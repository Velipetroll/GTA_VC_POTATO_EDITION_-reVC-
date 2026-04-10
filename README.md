<img src="https://raw.githubusercontent.com/Velipetroll/reVC-POTATO_EDITION/refs/heads/miami/Images/logo/logo_1024.png?raw=true" alt="reVC logo" width="200">

[![Build Status](https://img.shields.io/endpoint.svg?url=https%3A%2F%2Factions-badge.atrox.dev%2Fmrxenginner%2FreVC%2Fbadge%3Fref%3Dmiami&style=flat)](https://actions-badge.atrox.dev/mrxenginner/reVC/goto?ref=miami)
<a href="https://discord.gg/RFNbjsUMGg"><img src="https://img.shields.io/badge/discord-join-7289DA.svg?logo=discord&longCache=true&style=flat" /></a>

## GTA Vice City Potato Edition 🌴

> ⚠️ **REVOLUTIONARY APPROACH: 100% AI-GENERATED CODE** ⚠️
> **Every single engine optimization, C++ logic rewrite, and new feature in this fork was exclusively designed, analyzed, and programmed by Artificial Intelligence (Gemini).** The human author acted strictly as a project director, tester, and compiler. This project stands as a proof-of-concept demonstrating how AI can deep-dive into complex, legacy reverse-engineered C++ game engines to surgically extract maximum performance.

## Intro & Objectives

In this repository you'll find a heavily modified and optimized version of the fully reversed source code for GTA VC ([miami](https://github.com/mrxenginner/reVC/tree/miami/) branch).

This specific fork was born with a clear goal: **to make GTA Vice City run smoothly and stably on extremely low-end hardware, specifically targeting single-core processors like the Intel Atom N450 and integrated graphics like the GMA 3150**. To achieve this, the AI analyzed the RenderWare engine and successfully eliminated massive bottlenecks, disabled unnecessary background mathematical calculations, and introduced aggressive Level of Detail (LOD) mechanics that the original developers left on the table.

It has been tested and works only on Windows\
Rendering is handled either by original RenderWare (D3D8)
or the reimplementation [librw](https://github.com/aap/librw) (D3D9, OpenGL 2.1 or above, OpenGL ES 2.0 or above).\
Audio is done with MSS (using dlls from original GTA) or OpenAL.

We cannot build for PS2 or Xbox yet. If you're interested in doing so, get in touch with us.

## ⚡ 100% AI-Driven Performance Optimizations

* **The Heat & CPU Patch (End of "Busy-Wait"):** The AI discovered through profiling that the engine spent nearly 60% of the CPU simply "waiting" (`rsIDLE`) for the next frame to draw. By injecting a `Sleep(1)` command into the frame limiter, the engine now yields control to Windows for 1 millisecond during idle times. This dramatically improved performance and plummeted hardware temperatures from 90°C to safe levels.
* **Distance-Based Entity Time-Slicing:** Entity updates are now scaled by distance. The further an NPC or vehicle is from the camera, the fewer frames it takes to update its AI, movement, position relative to the player, and animation frames.
* **Aggressive Island Culling:** If the player is on one island, the engine is strictly prohibited from rendering *anything* from the other island under any circumstances.
* **Flat Lighting & VFX Annihilation:** `Light.cpp` was completely emptied. Weather no longer visually affects any surface in the game, resulting in a completely flat, ultra-fast rendering pipeline. Heavy water math, air modifiers on 3D models, and all particle systems have been entirely removed.
* **Total Glass Annihilation:** All vehicle windows (both intact and damaged) have been forced to 0% opacity, completely removing their polygons from the rendering pipeline to save massive amounts of fillrate on older GPUs.
* **Mathematical Optimizations:** Replaced heavy CPU functions (`Sqrt` and `Atan2`) with "Manhattan Distance" approximations for entity visibility checks, saving thousands of clock cycles.

## 🎮 AI-Coded New Features Added

* **Quick Save System (F6):** A seamless quick-save feature was injected into the main game loop (`Game.cpp`). Pressing F6 intercepts the keyboard input and safely triggers the official save menu (`m_bActivateSaveMenu`), pausing the game and allowing you to save anywhere without corrupting your file.
* **True First-Person Camera (Vehicles):** Drive from Tommy's perspective! A first-person view has been fully programmed by AI, adjusting perfectly depending on the vehicle.
* **Crash-Proof Shielding:** AI patched texture destruction functions (Garbage Collection) to prevent unexpected engine crashes when loading new save files.

## Installation

- reVC requires game assets to work, so you **must** own [a copy of GTA Vice City](https://store.steampowered.com/app/12110/Grand_Theft_Auto_Vice_City/).
- Build reVC or download the latest build:
  - [Windows D3D9 MSS 32bit (ONLY POTATO BUILD)](https://www.mediafire.com/file/ikbxczynuvuo7uy/win-x86-librw_d3d9-mss.zip/file)
  - [Windows D3D9 64bit](https://nightly.link/mrxenginner/reVC/workflows/reVC_msvc_amd64/miami/reVC_Release_win-amd64-librw_d3d9-oal.zip)
  - [Windows OpenGL 64bit](https://nightly.link/mrxenginner/reVC/workflows/reVC_msvc_amd64/miami/reVC_Release_win-amd64-librw_gl3_glfw-oal.zip)
  - [Linux 64bit](https://nightly.link/mrxenginner/reVC/workflows/build-cmake-conan/miami/ubuntu-18.04-gl3.zip)
  - [MacOS 64bit x86-64](https://nightly.link/mrxenginner/reVC/workflows/build-cmake-conan/miami/macos-latest-gl3.zip)
  - [Android armeabi-v7a and arm64-v8a](https://nightly.link/mrxenginner/reVC/workflows/build-android/miami/revc-release.zip)
  
- Extract the downloaded zip over your GTA VC directory and run reVC. The zip includes the binary, updated and additional gamefiles and in case of OpenAL the required dlls.

## Screenshots

![screen_ 1613087332](https://raw.githubusercontent.com/Velipetroll/reVC-POTATO_EDITION/refs/heads/miami/Images/Image_1.png)
![screen_ 1613086989](https://raw.githubusercontent.com/Velipetroll/reVC-POTATO_EDITION/refs/heads/miami/Images/Image_3.png)
![screen_ 1613087193](https://raw.githubusercontent.com/Velipetroll/reVC-POTATO_EDITION/refs/heads/miami/Images/Image_4.png)
![screen_ 1613087123](https://raw.githubusercontent.com/Velipetroll/reVC-POTATO_EDITION/refs/heads/miami/Images/Image_5.png)
![screen_ 161301193](https://raw.githubusercontent.com/Velipetroll/reVC-POTATO_EDITION/refs/heads/miami/Images/Image_6.png)

## Standard reVC Improvements

We have implemented a number of changes and improvements to the original game.
They can be configured in `core/config.h`.
Some of them can be toggled at runtime, some cannot.

* Fixed a lot of smaller and bigger bugs
* User files (saves and settings) stored in GTA root directory
* Settings stored in reVC.ini file instead of gta_vc.set
* Debug menu to do and change various things (Ctrl-M to open)
* Debug camera (Ctrl-B to toggle)
* Rotatable camera
* XInput controller support (Windows)
* No loading screens between islands ("map memory usage" in menu)
* Rendering
  * Widescreen support (properly scaled HUD, Menu and FOV)
  * PS2 MatFX (vehicle reflections)
  * PS2 alpha test (better rendering of transparency)
  * Xbox vehicle rendering
  * Xbox world lightmap rendering (needs Xbox map)
  * Xbox ped rim light
  * Xbox screen rain droplets
  * More customizable colourfilter
* Menu
  * More options
  * Controller configuration menu
  * ...
* Can load DFFs and TXDs from other platforms, possibly with a performance penalty
* ...

## To-Do

The following things would be nice to have/do:

* Fix physics for high FPS
* Improve performance on lower end devices, especially the OpenGL layer on the Raspberry Pi (if you have experience with this, please get in touch)
* [PS2 port](https://github.com/mrxenginner/reVC/wiki/PS2-port)
* Xbox port (not quite as important)
* reverse remaining unused/debug functions
* compare CodeWarrior build with original binary for more accurate code (very tedious)

## Modding

Asset modifications (models, texture, handling, script, ...) should work the same way as with original GTA for the most part.

Mods that make changes to the code (dll/asi, CLEO, limit adjusters) will *not* work.
Some things these mods do are already implemented in re3 (much of SkyGFX, GInput, SilentPatch, Widescreen fix),
others can easily be achieved (increasing limis, see `config.h`),
others will simply have to be rewritten and integrated into the code directly.
Sorry for the inconvenience.

## Building from Source  

When using premake, you may want to point GTA_VC_RE_DIR environment variable to GTA Vice City root folder if you want the executable to be moved there via post-build script.

Clone the repository with `git clone --recursive -b miami https://github.com/Velipetroll/reVC-POTATO_EDITION.git reVC`. Then `cd reVC` into the cloned repository.

<details><summary>Android</summary>

For Android using Android Studio, proceed: [Building on Android](https://github.com/mrxenginner/reVC/wiki/Building-on-Android)

</details>

<details><summary>Linux Premake</summary>

For Linux using premake, proceed: [Building on Linux](https://github.com/mrxenginner/reVC/wiki/Building-on-Linux)

</details>

<details><summary>Linux Conan</summary>

Install python and conan, and then run build.

conan export vendor/librw librw/master@
mkdir build
cd build
conan install .. reVC/miami@ -if build -o reVC:audio=openal -o librw:platform=gl3 -o librw:gl3_gfxlib=glfw --build missing -s reVC:build_type=RelWithDebInfo -s librw:build_type=RelWithDebInfo
conan build .. -if build -bf build -pf package

</details>

<details><summary>MacOS Premake</summary>

For MacOS using premake, proceed: [Building on MacOS](https://github.com/mrxenginner/reVC/wiki/Building-on-MacOS)

</details>

<details><summary>FreeBSD</summary>

For FreeBSD using premake, proceed: [Building on FreeBSD](https://github.com/mrxenginner/reVC/wiki/Building-on-FreeBSD)

</details>

<details><summary>Windows</summary>

Assuming you have Visual Studio 2015/2017/2019:
- Run one of the `premake-vsXXXX.cmd` variants on root folder.
- Open build/reVC.sln with Visual Studio and compile the solution.
    
Microsoft recently discontinued its downloads of the DX9 SDK. You can download an archived version here: https://archive.org/details/dxsdk_jun10

**If you choose OpenAL on Windows** You must read [Running OpenAL build on Windows](https://github.com/mrxenginner/reVC/wiki/Running-OpenAL-build-on-Windows).
</details>

> :information_source: premake has an `--with-lto` option if you want the project to be compiled with Link Time Optimization.

> :information_source: There are various settings in [config.h](https://github.com/mrxenginner/reVC/tree/miami/src/core/config.h), you may want to take a look there.

> :information_source: reVC uses completely homebrew RenderWare-replacement rendering engine; [librw](https://github.com/aap/librw/). librw comes as submodule of re3, but you also can use LIBRW enviorenment variable to specify path to your own librw.

If you feel the need, you can also use CodeWarrior 7 to compile reVC using the supplied codewarrior/reVC.mcp project - this requires the original RW34 libraries, and the DX8 SDK. The build is unstable compared to the MSVC builds though, and is mostly meant to serve as a reference.

## Contributing
As long as it's not linux/cross-platform skeleton/compatibility layer, all of the code on the repo that's not behind a preprocessor condition(like FIX_BUGS) are **completely** reversed code from original binaries.  

We **don't** accept custom codes, as long as it's not wrapped via preprocessor conditions, or it's linux/cross-platform skeleton/compatibility layer.

We accept only these kinds of PRs;

- A new feature that exists in at least one of the GTAs (if it wasn't in III/VC then it doesn't have to be decompilation)  
- Game, UI or UX bug fixes (if it's a fix to original code, it should be behind FIX_BUGS)
- Platform-specific and/or unused code that's not been reversed yet
- Makes reversed code more understandable/accurate, as in "which code would produce this assembly".
- A new cross-platform skeleton/compatibility layer, or improvements to them
- Translation fixes, for languages original game supported
- Code that increase maintainability  

We have a [Coding Style](https://github.com/mrxenginner/reVC/blob/miami/CODING_STYLE.md) document that isn't followed or enforced very well.

Do not use features from C++11 or later.


## History

re3 was started sometime in the spring of 2018,
initially as a way to test reversed collision and physics code
inside the game.
This was done by replacing single functions of the game
with their reversed counterparts using a dll.

After a bit of work the project lay dormant for about a year
and was picked up again and pushed to github in May 2019.
At the time I (aap) had reversed around 10k lines of code and estimated
the final game to have around 200-250k.
Others quickly joined the effort (Fire_Head, shfil, erorcun and Nick007J
in time order, and Serge a bit later) and we made very quick progress
throughout the summer of 2019
after which the pace slowed down a bit.

Due to everyone staying home during the start of the Corona pandemic
everybody had a lot of time to work on re3 again and
we finally got a standalone exe in April 2020 (around 180k lines by then).

After the initial excitement and fixing and polishing the code further,
reVC was started in early May 2020 by starting from re3 code,
not by starting from scratch replacing functions with a dll.
After a few months of mostly steady progress we considered reVC
finished in December.

Since then we have started reLCS, which is currently work in progress.


## License

We don't feel like we're in a position to give this code a license.\
The code should only be used for educational, documentation and modding purposes.\
We do not encourage piracy or commercial use.\
Please keep derivate work open source and give proper credit
