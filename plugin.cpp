#include "pch.h"
#include "plugin.h"

#include "bakkesmod/wrappers/BallWrapper.h"
#include "bakkesmod/wrappers/CanvasWrapper.h"
#include "bakkesmod/wrappers/ControllerInput.h"
#include "bakkesmod/wrappers/GameEvent/GameEventWrapper.h"
#include "bakkesmod/wrappers/VehicleWrapper.h"
#include "bakkesmod/wrappers/ServerWrapper.h"
#include "bakkesmod/wrappers/Structs.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <format>
#include <string>

BAKKESMOD_PLUGIN(AirRollTrainer, "Directional air roll trainer", plugin_version, PLUGINTYPE_FREEPLAY | PLUGINTYPE_CUSTOM_TRAINING)

namespace
{
        constexpr float kMinSlowdownPercent = 5.0f;
        constexpr float kMaxSlowdownPercent = 100.0f;
        constexpr float kDefaultSlowdownPercent = 60.0f;
}

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void AirRollTrainer::onLoad()
{
        _globalCvarManager = cvarManager;

        RegisterCVars();
        RegisterRendering();
        ScheduleUpdate();
}

void AirRollTrainer::onUnload()
{
        ResetGameSpeed();

        if (gameWrapper)
        {
                gameWrapper->UnregisterDrawables(this);
        }
}

void AirRollTrainer::RegisterCVars()
{
        enabled_ = std::make_shared<bool>(true);

        auto enabledCvar = cvarManager->registerCvar(
                "art_enabled",
                "1",
                "Enable or disable the air roll trainer",
                true,
                true,
                0.f,
                true,
                1.f);
        enabledCvar.bindTo(enabled_);

        slowdownCvar_ = cvarManager->registerCvar(
                "art_slowdown_pct",
                std::format("{:.0f}", kDefaultSlowdownPercent),
                "Slowdown percentage applied to the game speed",
                true,
                true,
                kMinSlowdownPercent,
                true,
                kMaxSlowdownPercent);

        overlayCvar_ = cvarManager->registerCvar(
                "art_overlay_enabled",
                "1",
                "Show the input and guidance overlay",
                true,
                true,
                0.f,
                true,
                1.f);

        hintSensitivityCvar_ = cvarManager->registerCvar(
                "art_hint_sensitivity",
                "0.25",
                "Minimum recommendation strength before drawing the arrow",
                true,
                true,
                0.f,
                true,
                1.f);

        enabledCvar.addOnValueChanged([this](std::string, CVarWrapper wrapper) {
                if (!wrapper.getBoolValue())
                {
                        ResetGameSpeed();
                }
        });

        slowdownCvar_.addOnValueChanged([this](std::string, CVarWrapper) {
                ApplyGameSpeed();
        });
}

void AirRollTrainer::RegisterRendering()
{
        if (!gameWrapper || drawRegistered_)
        {
                return;
        }

        gameWrapper->RegisterDrawable(std::bind(&AirRollTrainer::Render, this, std::placeholders::_1));
        drawRegistered_ = true;
}

void AirRollTrainer::ScheduleUpdate()
{
        if (!gameWrapper)
        {
                return;
        }

        gameWrapper->SetTimeout([this](GameWrapper*) {
                UpdateTrainer();
                ScheduleUpdate();
        }, 0.02f);
}

void AirRollTrainer::UpdateTrainer()
{
        if (!gameWrapper || !enabled_ || !*enabled_)
        {
                return;
        }

        if (!gameWrapper->IsInFreeplay() && !gameWrapper->IsInCustomTraining())
        {
                ResetGameSpeed();
                return;
        }

        CarWrapper car = gameWrapper->GetLocalCar();
        if (!car)
        {
                return;
        }

        CaptureInput(car);
        UpdateRecommendation(car);
        ApplyGameSpeed();
}

void AirRollTrainer::ApplyGameSpeed()
{
        if (!gameWrapper || !enabled_ || !*enabled_)
        {
                return;
        }

        if (!gameWrapper->IsInFreeplay() && !gameWrapper->IsInCustomTraining())
        {
                return;
        }

        ServerWrapper server = gameWrapper->GetGameEventAsServer();
        if (!server)
        {
                return;
        }

        const float slowdown = std::clamp(slowdownCvar_.getFloatValue(), kMinSlowdownPercent, kMaxSlowdownPercent);
        const float gameSpeed = slowdown / 100.0f;
        server.SetGameSpeed(gameSpeed);
}

void AirRollTrainer::ResetGameSpeed()
{
        if (!gameWrapper)
        {
                return;
        }

        ServerWrapper server = gameWrapper->GetGameEventAsServer();
        if (server)
        {
                server.SetGameSpeed(1.0f);
        }
}

void AirRollTrainer::CaptureInput(const CarWrapper& car)
{
        ControllerInput input = car.GetInput();
        lastInput_ = input;
}

void AirRollTrainer::UpdateRecommendation(const CarWrapper& car)
{
        BallWrapper ball = gameWrapper->GetBall();
        if (!ball)
        {
                recommendedStick_ = {};
                recommendedRoll_ = 0.f;
                return;
        }

        const Vector carLocation = car.GetLocation();
        const Vector ballLocation = ball.GetLocation();
        Vector toBall = ballLocation - carLocation;

        if (toBall.magnitude() < 1.f)
        {
                recommendedStick_ = {};
                recommendedRoll_ = 0.f;
                return;
        }

        toBall.normalize();

        const Vector carForward = car.GetForwardVector();
        const Vector carRight = car.GetRightVector();
        const Vector carUp = car.GetUpVector();

        const Vector rotationAxis = carForward.cross(toBall);

        const float yawError = rotationAxis.dot(carUp);
        const float pitchError = rotationAxis.dot(carRight);
        const float rollError = rotationAxis.dot(carForward);

        const float maxComponent = std::max({ std::abs(yawError), std::abs(pitchError), 1e-3f });

        recommendedStick_.x = std::clamp(yawError / maxComponent, -1.f, 1.f);
        recommendedStick_.y = std::clamp(-pitchError / maxComponent, -1.f, 1.f);
        recommendedRoll_ = std::clamp(rollError, -1.f, 1.f);
}

void AirRollTrainer::Render(CanvasWrapper canvas)
{
        if (overlayCvar_.IsNull() || hintSensitivityCvar_.IsNull())
        {
                return;
        }

        if (!enabled_ || !*enabled_)
        {
                return;
        }

        if (!overlayCvar_.getBoolValue())
        {
                return;
        }

        if (!gameWrapper->IsInFreeplay() && !gameWrapper->IsInCustomTraining())
        {
                return;
        }

        const Vector2 canvasSize = canvas.GetSize();
        const float boxSize = 200.f;
        const Vector2 origin{
                static_cast<int>(canvasSize.X - boxSize - 40.f),
                static_cast<int>(canvasSize.Y - boxSize - 160.f)
        };
        const Vector2 boxDimensions{ static_cast<int>(boxSize), static_cast<int>(boxSize) };
        const Vector2 center{
                origin.X + boxDimensions.X / 2,
                origin.Y + boxDimensions.Y / 2
        };

        canvas.SetColor(0, 0, 0, 140);
        canvas.FillBox(origin, boxDimensions);

        canvas.SetColor(255, 255, 255, 220);
        canvas.DrawBox(origin, boxDimensions);

        const float half = static_cast<float>(boxDimensions.X) * 0.5f - 10.f;
        const Vector2 yawPitchIndicator{
                static_cast<int>(center.X + lastInput_.Yaw * half),
                static_cast<int>(center.Y - lastInput_.Pitch * half)
        };

        canvas.SetColor(120, 200, 255, 255);
        canvas.DrawLine(center, yawPitchIndicator);
        canvas.FillBox({ yawPitchIndicator.X - 3, yawPitchIndicator.Y - 3 }, { 6, 6 });

        const float recommendationStrength = std::sqrt(recommendedStick_.x * recommendedStick_.x + recommendedStick_.y * recommendedStick_.y);
        if (recommendationStrength >= hintSensitivityCvar_.getFloatValue())
        {
                const Vector2 recommendedTarget{
                        static_cast<int>(center.X + recommendedStick_.x * half),
                        static_cast<int>(center.Y - recommendedStick_.y * half)
                };

                canvas.SetColor(255, 140, 0, 255);
                canvas.DrawLine(center, recommendedTarget);
                canvas.FillBox({ recommendedTarget.X - 4, recommendedTarget.Y - 4 }, { 8, 8 });
        }

        const int textX = origin.X;
        int textY = origin.Y + boxDimensions.Y + 10;
        const auto drawStat = [&](const std::string& label, float value) {
                const std::string line = fmt::format("{}: {:.2f}", label, value);
                canvas.DrawString({ textX, textY }, line, 1.0f, 1.0f);
                textY += 18;
        };

        canvas.SetColor(255, 255, 255, 255);
        drawStat("Throttle", lastInput_.Throttle);
        drawStat("Steer", lastInput_.Steer);
        drawStat("Pitch", lastInput_.Pitch);
        drawStat("Yaw", lastInput_.Yaw);
        drawStat("Roll", lastInput_.Roll);
        drawStat("Roll hint", recommendedRoll_);
}
