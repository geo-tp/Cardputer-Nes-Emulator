# Cardputer Game Station

![NES emulator screen captures on the M5Stack Cardputer](nes_emulator_s.jpg)
![SMS emulator screen captures on the M5Stack Cardputer](sms_emulator_s.jpg)
![NGP emulator screen captures on the M5Stack Cardputer](ngp_emulator_s.jpg)
![Megadrive emulator screen captures on the M5Stack Cardputer](megadrive_emulator_s.jpg)

Powered by [Nofrendo](https://github.com/moononournation/arduino-nofrendo), **Smsplus**, [Race](https://github.com/libretro/RACE), [Gwenesis](https://github.com/bzhxx/gwenesis), [Oswan](https://github.com/alekmaul/oswan) and **PCE-GO** 

**Running on the M5Stack Cardputer**, with sound, video, game save, and controls.

 Console           | Sound | Video | Save | Speed | All Games  | Notes |
|-------------------|--------|--------|---------------|-------------|-------------------|--------|
| **NES**           | ✅ | ✅ | ✅ | ✅ | ✅ | Few mappers issues in some games |
| **Master System** | ✅ | ✅ | ✅ | ✅ | ✅ | Fully compatible |
| **Game Gear**     | ✅ | ✅ | ✅ | ✅ | ✅ | Fully compatible |
| **PC Engine**     | ✅ | ✅ | ⚠️ | ✅ | ✅ | Fully compatible |
| **Mega Drive**    | ✅ | ✅ | ⚠️ | ✅ | ✅ | Some slowdown and not accurate sound in heavy titles |
| **Neo Geo Pocket**| ✅ | ✅ | ⚠️ | ✅ | ✅| Some slowdown in heavy titles |
| **WonderSwan**    | ✅ | ✅ | ⚠️ | ⚠️ | ⚠️ | No support for SRAM >32KB, not fullspeed (75FPS) in most games  |


It runs **`.nes` `.sms` `.gg`  `.md` `.ngc` `.ngp` `.ws` `.wsc` `.pce` ROM files directly from the SD**.

> **Make sure your ROMs are uncompressed** (not .zip, .7z, or .rar).

## Controls

The built-in **Cardputer keyboard** is used for all controls: 

| Function | Cardputer Key | Description |
|---------------|---------------|-------------|
| 🕹️ Up | **E** | Move up |
| 🕹️ Down | **S** | Move down |
| 🕹️ Left | **A** | Move left |
| 🕹️ Right | **D** | Move right |
| 🅰️ Button A | **K** | Primary action / confirm |
| 🅱️ Button B | **L** | Secondary action / cancel |
| ▶️ Start | **1** | Start / pause |
| ⏸️ Select | **2** | Select / menu |
| 💡 Brightness + | **]** | Increase LCD brightness |
| 💡 Brightness − | **[** | Decrease LCD brightness |
| 🔊 Volume + | **+** | Increase audio volume |
| 🔊 Volume − | **-** | Decrease audio volume |
| 🖥️ Screen Mode | **\\** | Toggle screen display mode |
| 🔘 Quit Game | **G0 (hold 1 s)** | Go back to menu |

**Note:** `fn` + `arrows` keys are also binded for zoom/sound controls. The `j` key is also bound as Button A to allow an alternative layout for player preference.

## Zoom Mode

The Zoom Mode allows you to **dynamically adjust the display scale of games** on the Cardputer’s screen.

By pressing `\` (above the `OK` key), you can toggle between **multiple zoom levels (100 to 150%), fullscreen or 4/3**. This flexibility ensures that each game looks its best on the Cardputer’s compact display.

You can precisely adjust the display zoom level with `fn` + `arrows left/right`.

✅ Why it matters:

- Enhances readability and visual comfort.
- Lets you adapt the screen to games.
- Greatly improves gameplay experience.

## About Games

You can place the **ROM uncompressed files** anywhere on your SD card and select them. **Avoid having folders with more than 512 items** to prevent loading times.

When browsing your game list, you can **type the first few letters of a game’s name** to jump directly to it. This makes it much faster to find a specific title, especially when your library contains dozens of entries. You should **avoid game titles longer than 64 characters**.

ROMs **up to 5MB can be played when flashing the firmware directly** (which covers 99.9% of all games), if you're using the [Launcher](https://github.com/bmorcelli/Launcher), it is limited to 1 MB.