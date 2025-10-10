# NES Emulator for M5Stack Cardputer

![Nes Emulator screen captures on the M5Stack Cardputer](nes_emulator_s.jpg)

Powered by [Nofrendo](https://github.com/moononournation/arduino-nofrendo). **A full-speed NES emulator running on the M5Stack Cardputer**, with sound, video, save, and gamepad controls.

It runs **.nes ROM files directly from the SD**, simply browse and select the game you want to play.
**Make sure your ROMs are uncompressed** (.nes only — not .zip, .7z, or .rar).

**All games are supported**, save files are stored on the SD card in the `nes_saves` folder.

## Controls

The built-in **Cardputer keyboard** is used for all NES controls: 

| NES Function | Cardputer Key | Description |
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

**Note:** `fn` + `arrows` keys are also binded for zoom/sound controls. The `j` key is also bound as Button A to allow an alternative layout for player preference.

## Zoom Mode

The Zoom Mode allows you to **dynamically adjust the display scale of NES games** on the Cardputer’s screen.

By pressing `/`, you can toggle between **multiple zoom levels (100 to 150%),  fullscreen or 4/3**. This flexibility ensures that each game looks its best on the Cardputer’s compact display.

You can precisely adjust the display zoom level with `fn` + `arrows left/right`.

✅ Why it matters:

- Enhances readability and visual comfort.

- Lets you adapt the screen to games.

- Greatly improves gameplay experience.

## About Games

All games  `.nes` from the **official NES catalog** should be **fully supported** and run at **full speed**. You can place the ROM files anywhere on your SD card and select them.

✅ Expected behavior:
- Smooth 60 FPS gameplay  
- Accurate sound emulation  
- SD save/load functionality

You can pic any `.nes` file from your SD using the file browser. When browsing your game list, you can **type the first few letters of a game’s name** to jump directly to it. This makes it much faster to find a specific title, especially when your library contains dozens of entries. You should **avoid game titles longer than 64 characters**.

Games are loaded from the SD cards with **slow I/O performance**, it’s strongly recommended **not to put hundreds of game files in the same directory**.  
Large directories can drastically increase load time for rom selection, or even make the file browsers unresponsive (limit 512 files per folder).

To ensure smooth navigation and faster access, **organize your games into smaller folders**, each containing **fewer than 100 files** whenever possible.  