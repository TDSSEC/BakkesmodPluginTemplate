#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/wrappers/CanvasWrapper.h"
#include "bakkesmod/wrappers/GameWrapper.h"
#include "bakkesmod/wrappers/Structs.h"
#include "bakkesmod/wrappers/cvarwrapper.h"

#include "version.h"

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class CarWrapper;

class AirRollTrainer final : public BakkesMod::Plugin::BakkesModPlugin
{
public:
        void onLoad() override;
        void onUnload() override;

private:
        struct StickVector
        {
                float x{ 0.f };
                float y{ 0.f };
        };

        void RegisterCVars();
        void RegisterRendering();
        void ScheduleUpdate();
        void UpdateTrainer();
        void ApplyGameSpeed();
        void ResetGameSpeed();
        void CaptureInput(const CarWrapper& car);
        void UpdateRecommendation(const CarWrapper& car);
        void Render(CanvasWrapper canvas);

        std::shared_ptr<bool> enabled_;
        CVarWrapper slowdownCvar_;
        CVarWrapper overlayCvar_;
        CVarWrapper hintSensitivityCvar_;

        ControllerInput lastInput_{};
        StickVector recommendedStick_{};
        float recommendedRoll_{ 0.f };
        bool drawRegistered_{ false };
};
