# Air Roll Trainer

Directional air roll practice helper for Rocket League. The plugin slows down the game in Freeplay and Custom Training, draws a live controller overlay, and suggests how you should move your stick to align the car with the ball.

## Features

* **Configurable slowdown:** `art_slowdown_pct` lets you choose how much to slow the simulation (default 60%).
* **Overlay toggle:** `art_overlay_enabled` hides or shows the controller overlay.
* **Hint sensitivity:** `art_hint_sensitivity` controls how strong the recommendation must be before the arrow appears.
* **Enable switch:** `art_enabled` turns the trainer on or off and restores normal game speed when disabled.

## Building

Build the Visual Studio project in Release mode. Copy the produced `AirRollTrainer.dll` to `BakkesMod\plugins` and add `AirRollTrainer` to `plugins.cfg`.
