# Arena Battles: A HoMM3 Fan Mod

**Draft a hero, build an army, fight one battle.** Arena Battles is a fan-made arena mode for *Heroes of Might and Magic III*, built on the open-source [VCMI](https://vcmi.eu/) engine. There is no map and no economy, and you won't wait 100 turns. You pick a hero, draft skills, spells, artifacts and creatures one screen at a time, and then everything comes down to one battle.

**⬇️ [Download the latest release (v0.7.0)](https://github.com/tishonator/arena-battles-hmm3-fan-mod/releases/latest)** for Windows, macOS, Linux and Android.

> [!CAUTION]
> **You need your own copy of *Heroes of Might and Magic III: Complete* or *Shadow of Death*.**
> **The Steam *HD Edition* on its own will NOT work.** Arena Battles contains no original game files.
> The easiest option is *HoMM3 Complete* from [GOG](https://www.gog.com/game/heroes_of_might_and_magic_3_complete_edition). See [how to use your game files](#1-before-you-start-the-original-game-data) (including on Android) **before you download**.

![Main menu](arena-battles/screenshots/main-menu.png)

> **Made by [Tihomir Ivanov](#about-the-author) – [TishGamer on YouTube](https://www.youtube.com/@TishGamer).**
> If you like the mod, **[subscribe to the channel](https://www.youtube.com/@TishGamer?sub_confirmation=1)**. It's the best way to support the work.

---

## Contents

- [Features](#features)
- [Game modes](#game-modes)
- [How a draft works](#how-a-draft-works)
  - [Choosing your skills](#choosing-your-skills)
  - [Fair picks: heroes, artifacts and armies](#fair-picks-heroes-artifacts-and-armies)
- [Screenshots](#screenshots)
- [Installation](#installation)
  - [Before you start: the original game data](#1-before-you-start-the-original-game-data)
  - [Download](#2-download)
  - [Windows](#windows)
  - [macOS](#macos)
  - [Linux](#linux)
  - [Android](#android)
  - [iOS / iPadOS](#ios--ipados)
  - [First launch](#3-first-launch)
  - [Updating and uninstalling](#4-updating-and-uninstalling)
  - [Troubleshooting](#troubleshooting)
- [Building from source](#building-from-source)
- [Repository layout](#repository-layout)
- [About the author](#about-the-author)
- [Support my work](#support-my-work)
- [Credits and license](#credits-and-license)

---

## Features

- **Three arena modes:**
  - **PvE** draft against the computer;
  - **PvP** hotseat for two players on one computer;
  - **Custom Arena**, where you set up both sides by hand.
- **A full draft in about 10 minutes.** Pick 1 of 4 heroes (always 2 might and 2 magic heroes) and fill all 8 skill slots with **battle skills only**. Your hero then jumps to level 25 with every skill at Expert. Then choose 9 spells, 11 artifacts (one for each slot) and 7 creature stacks.
- **No useless picks.** You'll never be offered Estates, Scouting or Navigation, heroes whose specialty is gold or Logistics, the Earthquake spell, or other things that do nothing in a single battle.
- **Big, balanced armies.** Armies are about the size you'd have at the end of a large map, and every creature offer has the same total strength: upgraded and stronger creatures come in smaller stacks, with no random amounts.
- **A smart AI opponent.** In PvE, the computer drafts the same way you do and always takes the strongest offer.
- **Fair fights.** Quick combat is off, both sides get the same number of picks, and the battlefield is random.
- **Original HoMM3 look and feel.** The mod runs on your own copy of the game data, so it has the classic creatures, spells, artifacts, music and battle screen.
- **Runs everywhere VCMI runs:** Windows, macOS, Linux, Android and iOS.
- **Free and open source.**

## Game modes

| Menu item | What it is |
| --- | --- |
| **New Arena PvE** | You draft a hero and army. The computer drafts its own, then you fight it. |
| **New Arena PvP** | Two players take turns drafting on the same computer (Player 1, then Player 2). Then they fight each other in hotseat. |
| **Custom Arena** | A sandbox. Pick both heroes, their skills, artifacts and **any creatures in any amounts**. Player 2 can be a **Human** or the **Computer**. It is good for testing formations, "creature vs creature" match-ups and theory-crafting. |
| **Credits** | Who made this, and the engine and game credits. |

## How a draft works

1. **Choose your hero.** You get 4 random heroes: 2 might and 2 magic heroes, each from a different faction. Every hero starts with a spell book.
2. **Choose your skills.** A hero has 8 skill slots, and you fill them one screen at a time, taking 1 of 3 skills each time.
   - Only skills that matter in a battle are offered: Offense, Armorer, Archery, Resistance, Tactics, Leadership, Luck, Intelligence, Sorcery, and Air, Earth, Fire and Water Magic.
   - Skills the hero starts with stay if they're battle skills; any others are removed, so every hero gets 6-8 skill picks.
3. **Level 25, automatically.** Your hero levels up to 25 right away. Attack, Defense, Power and Knowledge grow the way they do for the hero's class in the original game, and **all your skills become Expert**. A summary screen shows the result.
4. **Spells.** There are 9 screens, and on each you take 1 of 3 combat spells.
5. **Artifacts.** There are 11 screens, one for each slot: helmet, necklace, cloak, weapon, shield, armor, boots, two rings and two miscellaneous artifacts. On each you take 1 of 3 artifacts. Then you **arrange** them on your hero's paper doll.
6. **Creatures.** There are 7 screens, one for each creature level. Each offers 3 stacks from 3 different factions, and one is always from your hero's faction. A typical stack of a non-upgraded creature has:

   | Level         | 1     | 2    | 3    | 4    | 5    | 6   | 7   |
   | ------------- | ----- | ---- | ---- | ---- | ---- | --- | --- |
   | Typical stack | ~1000 | ~350 | ~225 | ~160 | ~120 | ~95 | ~75 |

7. **Confirm.** Review your hero and see your opponent's hero. Then press **Begin**.
8. **Fight!** One battle on a random battlefield decides the winner, and then the game ends.

### Choosing your skills

In the original game, level-ups offer random skills, so you could end up with Estates, Scouting or Navigation, which do nothing in a single battle. Arena Battles gives you **8 skill screens instead**. Each one offers **3 random battle skills**, and you pick the one you want:

![Choose a skill: three battle skills to pick from](arena-battles/screenshots/choose-skill.png)

- **The skill pool (13 skills):**
  - Offense, Armorer, Archery, Resistance, Tactics, Leadership and Luck;
  - Intelligence and Sorcery;
  - Air, Earth, Fire and Water Magic.
- **Each skill can only be picked once.** Skills you already have aren't offered again.
- **Starting skills:** if your hero starts with battle skills (for example Offense), they stay and fill a slot. Non-battle starting skills are removed. Most heroes get 6-8 picks.
- **Every skill becomes Expert.** The card shows what the skill does at Expert level, and right-clicking a card shows its full description.
- **Plan around your spells:** a magic school (Air, Earth, Fire, Water) makes its spells cheaper and stronger, Sorcery adds spell damage, and Intelligence doubles your spell points.

When all slots are full, your hero **levels up to 25 automatically**, and you see the result before the spell draft starts:

![Your hero at level 25 with 8 Expert battle skills](arena-battles/screenshots/hero-level-25.png)

The computer opponent drafts its skills the same way and picks the most useful skill for battle. In PvP, each player drafts their own skills.

### Fair picks: heroes, artifacts and armies

Version 0.7.0 makes the draft less about luck and more about your choices.

**Heroes.** You choose from **4 heroes: 2 might and 2 magic heroes, each from a different faction**. So whether you like might or magic, you always have a choice. The hero's name is shown at the top of each card. Heroes whose specialty does nothing in an arena battle are never offered:

- gold and resources (for example +350 gold or +1 crystal);
- Navigation, Logistics, Eagle Eye, Mysticism and Estates;
- First Aid and Ballista, since there are no war machines in the arena;
- Necromancy, which only raises skeletons after a battle.

Heroes with a creature, spell or battle-skill specialty stay, so a Galthran still makes your Skeletons stronger.

**Spells.** Only combat spells are offered. **Earthquake** is left out, since it only damages castle walls and the arena has none. Every other combat spell stays, even ones with a small effect like Remove Obstacle.

![Choose an artifact for one slot](arena-battles/screenshots/choose-artifact.png)

**Artifacts.** Every screen is for **one slot**, so your artifacts never compete for the same place: helmet, necklace, cloak, weapon, shield, armor, boots, two rings, then two miscellaneous artifacts.

- Only artifacts that do something in battle are offered.
- **Orb of Inhibition** is left out because it stops all spell casting, which would make a magic hero useless.
- **Combination artifacts** (for example Angelic Alliance) are not offered ready-made. If you draft all of their parts, they **combine automatically** after the last artifact screen, just like in the original game.

![Choose level 7 creatures: one from your hero's faction, two from other factions](arena-battles/screenshots/choose-creatures.png)

**Armies.** Armies are about **10 times larger** than in earlier versions, closer to the end of a large or extra-large map, so battles last longer.

- **Every offer on a screen has the same total strength.** The stack size is based on the creature's strength rating from the original game (its AI value). Upgraded and stronger creatures come in smaller stacks, weaker ones in bigger stacks, and amounts are never random. For example:
  - level 1: 740 Pikemen or 510 Halberdiers;
  - level 7: 70 Angels or 39 Archangels, and 100 Bone Dragons or 39 Black Dragons.
- **Each screen offers 3 different factions, and one creature is always from your hero's faction.** A Necropolis hero can build a full Necropolis army for the best morale, or mix factions for the strongest creatures.

The computer opponent follows the same rules. It takes the most valuable artifact for each slot and prefers creatures from its own faction.

## Screenshots

| | |
| --- | --- |
| ![Choose a hero](arena-battles/screenshots/choose-hero.png) **Choose 1 of 4 heroes** | ![Battle](arena-battles/screenshots/battle.png) **One battle decides it** |
| ![Choose a spell](arena-battles/screenshots/choose-spell.png) **Draft spells** | ![Choose an artifact](arena-battles/screenshots/choose-artifact.png) **Draft artifacts** |
| ![Arrange artifacts](arena-battles/screenshots/arrange-artifacts.png) **Arrange your artifacts** | ![Choose creatures](arena-battles/screenshots/choose-creatures.png) **Draft creatures** |
| ![Hero confirm](arena-battles/screenshots/hero-confirm.png) **Confirm and begin** | ![Custom Arena](arena-battles/screenshots/custom-arena.png) **Custom Arena sandbox** |

---

## Installation

Arena Battles is a **standalone app**. It ships as its own build of the VCMI engine, with the mod built in and turned on automatically. You don't need to copy the mod anywhere or enable it in a launcher.

### 1. Before you start: the original game data

> [!CAUTION]
> **Arena Battles does not include any Heroes of Might and Magic III files. You need your own legal copy of *HoMM3 Complete* or *Shadow of Death*.**
> **The Steam *HD Edition* on its own will NOT work**, because it doesn't contain the files Arena Battles needs. If you only own the HD Edition, get *HoMM3 Complete* (for example from GOG) first.

**These versions work:**

- ✅ *Heroes of Might and Magic III: Complete*, for example from [GOG](https://www.gog.com/game/heroes_of_might_and_magic_3_complete_edition) (recommended, the easiest on every platform);
- ✅ *Shadow of Death*;
- ✅ the original CD version with both expansions.
- ❌ **Steam *HD Edition* alone does not work.**

**What you need from it:** only three folders of your installed game: **`Data`**, **`Maps`** and **`Mp3`**. Here is a GOG install of *HoMM3 Complete* on Windows as an example:

![Example: the HoMM3 Complete (GOG) install folder with the Data, Maps and Mp3 folders highlighted](arena-battles/screenshots/homm3-install-folder.png)

**How to give Arena Battles your game files.** On first start, the setup wizard asks for them:

- **Windows, macOS, Linux:** point the wizard at your HoMM3 install folder (the one shown above). Or give it the GOG **offline installer** (see below) and it unpacks the files for you.
- **Android and iOS:** you can't install the GOG game on a phone, so use one of these:
  1. **GOG offline installer (no PC install needed).** In your GOG library on [gog.com](https://www.gog.com/), open *HoMM3 Complete*, go to the offline backup installers, and download the Windows installer files. That's `setup_heroes_of_might_and_magic_3_complete_*.exe`, plus a `.bin` file if there is one. Copy them to your phone into the same folder. In the Arena Battles setup wizard, choose to import from the GOG installer and select the `.exe`.
  2. **Copy the folders from a PC.** Copy `Data`, `Maps` and `Mp3` from a computer install to your phone (USB cable or cloud storage), then select that folder in the setup wizard.

Arena Battles uses the same game data as VCMI. **If you already play VCMI on a computer, Arena Battles will find your data automatically.**

### 2. Download

Open the **[latest release (v0.7.0)](https://github.com/tishonator/arena-battles-hmm3-fan-mod/releases/latest)** and download the file for your system (`<version>` is the release version, e.g. `v0.7.0`):

| System | File | Notes |
| --- | --- | --- |
| Windows 10/11 (64-bit) | `ArenaBattles-<version>-x64.exe` (installer) or `ArenaBattles-<version>-x64.zip` (portable) | Also `-x86` (32-bit) and `-arm64` versions |
| macOS (Apple Silicon) | `ArenaBattles-<version>-arm.dmg` | |
| macOS (Intel) | `ArenaBattles-<version>-intel.dmg` | When available for that release |
| Linux | `ArenaBattles-<version>-x86_64.AppImage` or `-arm64.AppImage` | Works on most distributions |
| Android 5.0+ | `ArenaBattles-<version>-arm64-v8a.apk` | Most phones. Older 32-bit phones: `-armeabi-v7a.apk`; x86-64 devices and emulators: `-x64.apk` |
| iOS / iPadOS | `ArenaBattles-<version>.ipa` | When available; needs sideloading, see below |

> The exact file names may differ a little. If you want to try the newest development build, open the **[Actions](https://github.com/tishonator/arena-battles-hmm3-fan-mod/actions)** tab and pick the latest green run of the `arena-battles` branch. Its **Artifacts** section has the same packages. You need to be signed in to GitHub to download them.

### Windows

1. Run the `.exe` installer. Windows SmartScreen may warn you because the installer is not code-signed. Click **More info → Run anyway**.
   - If you prefer the portable `.zip`, extract it to any folder, for example `C:\Games\ArenaBattles`.
2. The installer adds a **VCMI Arena Battles** shortcut. It installs **next to** any official VCMI install and does not replace it.
3. Start **VCMI Arena Battles** (`VCMI_launcher.exe` in the portable version) and continue with [First launch](#3-first-launch).

### macOS

1. Open the `.dmg` and drag **VCMI** (Arena Battles) into **Applications**. To keep an official VCMI install as well, rename the app first, for example to *Arena Battles*.
2. The app is not notarized by Apple. The first time you open it, **right-click → Open**, then confirm.
   - If macOS says the app is "damaged", run this in Terminal: `xattr -cr "/Applications/<app name>.app"`
3. Continue with [First launch](#3-first-launch).

### Linux

1. Make the AppImage executable and run it:
   ```sh
   chmod +x ArenaBattles-*.AppImage
   ./ArenaBattles-*.AppImage
   ```
2. On some distributions you also need FUSE 2 (`sudo apt install libfuse2` on Ubuntu/Debian).
3. Continue with [First launch](#3-first-launch).

### Android

1. Download the `.apk` on your phone or tablet and open it. Allow installing from this source when Android asks (*Settings → Install unknown apps*).
2. The app is called **Arena Battles**. It installs as a **separate app** next to the official VCMI app.
3. On first start, the launcher asks for your HoMM3 data. Import the **GOG offline installer** files, or copy the `Data`, `Maps` and `Mp3` folders from a PC. Both ways are explained step by step in [Before you start](#1-before-you-start-the-original-game-data).

### iOS / iPadOS

iOS only allows App Store apps unless you sideload them, so:

1. Install a sideloading tool such as [AltStore](https://altstore.io/) or [Sideloadly](https://sideloadly.io/) on your computer.
2. Sideload the `.ipa` with your Apple ID. With a free Apple ID you must refresh the app every 7 days.
3. Copy the `Data`, `Maps` and `Mp3` folders from your PC install into the app's folder using the **Files** app (or Finder/iTunes file sharing). You can also copy the GOG offline installer files there and import them in the setup wizard, see [Before you start](#1-before-you-start-the-original-game-data). Then start the app.

### 3. First launch

1. The **launcher** opens first. If it doesn't find your HoMM3 data, its setup wizard asks for it. You have two options:
   - point it at your HoMM3 install folder (the one containing `Data`, `Maps` and `Mp3`);
   - give it the **GOG offline installer** (`setup_heroes_of_might_and_magic_3_complete_*.exe`), and it unpacks the files for you.
2. Skip the wizard's mod suggestions if you like. **Arena Battles is already built in and enabled.**
3. Press **Start game**. The **Arena Battles** main menu opens.
4. Choose **New Arena PvE** and have fun!

For more help with the game data, see the official VCMI guides. Arena Battles uses the same steps:
[Windows](https://vcmi.eu/players/Installation_Windows/) ·
[macOS](https://vcmi.eu/players/Installation_macOS/) ·
[Linux](https://vcmi.eu/players/Installation_Linux/) ·
[Android](https://vcmi.eu/players/Installation_Android/) ·
[iOS](https://vcmi.eu/players/Installation_iOS/)

### 4. Updating and uninstalling

- **Updating:** download the new release and install it over the old one, or extract the new `.zip` or AppImage instead. Your settings and game data stay where they are.
- **Uninstalling:**
  - Windows: *Settings → Apps → VCMI Arena Battles → Uninstall*;
  - macOS: delete the app;
  - Linux: delete the AppImage;
  - Android and iOS: remove the app.

  The HoMM3 data and settings live in the VCMI user folder (for example `Documents\My Games\vcmi` on Windows). Leave that folder if you also use VCMI.

### Troubleshooting

| Problem | Fix |
| --- | --- |
| "Heroes III data files are missing" | The launcher can't find `Data`, `Maps` and `Mp3`. Run the launcher's setup wizard again (*Settings → Run setup wizard*) and point it at a *Complete* or *SoD* install. |
| I only have the Steam HD Edition | The HD Edition alone doesn't have the files Arena Battles needs. You need *HoMM3 Complete* (e.g. from GOG) or *Shadow of Death*. |
| The normal VCMI menu appears instead of Arena Battles | You started the official VCMI, not Arena Battles. Use the **VCMI Arena Battles** shortcut. |
| The game starts in a small window | Change the resolution and fullscreen settings in the launcher (*Settings*) or in-game (*Esc → System options*). |
| Something else went wrong | Open an [issue](https://github.com/tishonator/arena-battles-hmm3-fan-mod/issues) and attach `VCMI_Client_log.txt` from the `logs` folder inside the VCMI user folder. |

---

## Building from source

This repository is a fork of [VCMI](https://github.com/vcmi/vcmi), based on the **1.7.5** release. You build Arena Battles the same way you build VCMI. Follow the official guides:
[Windows](https://vcmi.eu/developers/Building_Windows/) ·
[macOS](https://vcmi.eu/developers/Building_macOS/) ·
[Linux](https://vcmi.eu/developers/Building_Linux/) ·
[Android](https://vcmi.eu/developers/Building_Android/) ·
[iOS](https://vcmi.eu/developers/Building_iOS/)

The `Mods/arenabattles` folder is installed with the engine automatically. On desktop it is copied next to the executables, and on Android and iOS it is bundled into the app.

**Automatic builds (GitHub Actions)**

- Every push to the `arena-battles` branch builds packages for **all platforms**: Windows x64/x86/ARM64, macOS Intel/ARM, Linux AppImage, Android and iOS. They appear under the run's **Artifacts**.
- **Making a release:** push the commit to `arena-battles`, then push a version tag for it. CI builds everything and **publishes the GitHub Release** with all the packages attached. The tag build replaces the branch build of the same commit.
  ```sh
  git push origin arena-battles
  git tag v0.7.0
  git push origin v0.7.0
  ```
- If a platform fails (for example when no macOS runner is free), the release is still published with the other packages. Use **Re-run failed jobs** on that run to add the missing files to the same release.
- Code signing is optional:
  - macOS and Windows signing are skipped when the secrets aren't set;
  - Android uses the public development keystore that VCMI ships for daily builds;
  - the iOS `.ipa` is unsigned and is meant for sideloading.

**The button art**

The menu buttons are original art, drawn by a script. They don't contain any pixels from the original game. To regenerate them (needs Python 3 and Pillow):

```sh
pip install pillow
python arena-battles/tools/build_buttons.py   # main menu buttons
python arena-battles/tools/build_assets.py    # title, background, icon
python arena-battles/tools/preview.py         # optional: renders a menu preview image
```

## Repository layout

| Path | What's inside |
| --- | --- |
| `Mods/arenabattles/` | **The mod:** `mod.json`, main menu layout, texts (`Content/config/arenabattles/english.json`) and graphics |
| `client/arena/` | The draft screens and logic (hero, skill, spell, artifact and creature picks; automatic level-ups; AI drafting) |
| `client/mainmenu/`, `client/lobby/`, … | Small engine hooks: menu commands, PvP hotseat, Custom Arena, credits |
| `lib/modding/ModManager.cpp` | Keeps the Arena Battles mod enabled in this build |
| `arena-battles/tools/` | Scripts that generate the mod's graphics, plus fonts (SIL Open Font License) |
| `arena-battles/screenshots/` | The images used in this README |
| `CI/`, `.github/workflows/` | Build and release pipeline for all platforms |
| everything else | The VCMI engine (see `AUTHORS.h`, `ChangeLog.md`) |

---

## About the author

Hi! I'm **Tihomir Ivanov**, a software developer and a lifelong *Heroes of Might and Magic* fan. I make gaming videos on my YouTube channel **[TishGamer](https://www.youtube.com/@TishGamer)**. I also build free, open-source projects that I share on GitHub as **[tishonator](https://github.com/tishonator)**, such as [TishCommerce](https://github.com/tishonator/TishCommerce), a free self-hosted eCommerce platform.

Arena Battles started from a simple wish. I wanted the best part of HoMM3 (the big hero-vs-hero battle) without having to play a whole map to get there. I hope you enjoy it as much as I enjoyed making it.

- **YouTube:** [youtube.com/@TishGamer](https://www.youtube.com/@TishGamer)
- **GitHub:** [github.com/tishonator](https://github.com/tishonator)
- **This mod:** [github.com/tishonator/arena-battles-hmm3-fan-mod](https://github.com/tishonator/arena-battles-hmm3-fan-mod)

## Support my work

Arena Battles is **free**, and it will stay free. Making and maintaining it takes a lot of time. If you enjoy it, here is how you can help:

1. **[Subscribe to TishGamer on YouTube](https://www.youtube.com/@TishGamer?sub_confirmation=1)** and turn on the 🔔. It's free, and it's the most solid way to support my work. Every subscriber, like and comment helps the channel grow, and that lets me keep making mods and videos.
2. **Share the mod.** Send it to a friend, challenge them to a PvP duel, or post it on your favourite HoMM3 forum or Discord.
3. **Star this repository** ⭐ on GitHub so more players can find it.
4. **Report bugs and ideas** in [Issues](https://github.com/tishonator/arena-battles-hmm3-fan-mod/issues).
5. **Donate.** If you'd like to buy me a coffee, any amount is greatly appreciated:

   [![Donate](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/donate/?hosted_button_id=S88TKD55YVK7S)

Your support helps me keep this project alive and improve it further. Thank you! ❤

---

## Credits and license

- **Mod design, code and art:** Tihomir Ivanov ([TishGamer](https://www.youtube.com/@TishGamer)).
- **Engine:** [VCMI](https://vcmi.eu/) by the VCMI Team and contributors, licensed under the **GNU GPL v2 or later**. This fork, including all Arena Battles changes, is released under the same license (see `license.txt`). The full source code is in this repository.
- **Fonts:** Cinzel, under the SIL Open Font License (the license is in `arena-battles/tools/fonts/`).
- **Heroes of Might and Magic** is a trademark of Ubisoft Entertainment. This is an **unofficial, non-commercial fan project**, and it is not made, endorsed or supported by Ubisoft or the VCMI Team. It doesn't include or distribute any of the original game's files. You need your own copy of the game.
