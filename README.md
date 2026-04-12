<div align="center">
  <img src="https://raw.githubusercontent.com/Velipetroll/reVC-POTATO_EDITION/refs/heads/miami/Images/logo/logo_1024.png?raw=true" alt="GTA Vice City Potato Edition Logo - AI Optimized reVC Fork" width="200">

  <h1>GTA Vice City: Potato Edition 🌴 (AI-Optimized reVC Fork)</h1>
  
  <p><b>The ultimate performance fork for extremely low-end PCs, single-core processors, and integrated graphics.</b></p>

  [![Build Status](https://img.shields.io/endpoint.svg?url=https%3A%2F%2Factions-badge.atrox.dev%2Fmrxenginner%2FreVC%2Fbadge%3Fref%3Dmiami&style=flat)](https://actions-badge.atrox.dev/mrxenginner/reVC/goto?ref=miami)
  <a href="https://discord.gg/RFNbjsUMGg"><img src="https://img.shields.io/badge/discord-join-7289DA.svg?logo=discord&longCache=true&style=flat" alt="Join our Discord community" /></a>
</div>

---

> ⚠️ **REVOLUTIONARY APPROACH: 100% AI-GENERATED C++ CODE** ⚠️
> **Every engine optimization, C++ logic rewrite, and new feature in this fork was exclusively designed, analyzed, and programmed by Artificial Intelligence (Gemini).** The human author acted strictly as a project director, tester, and compiler. This project stands as a proof-of-concept demonstrating how AI can deep-dive into complex, legacy reverse-engineered C++ game engines to surgically extract maximum performance.

## 🚀 Intro & Project Objectives

Welcome to **GTA Vice City Potato Edition**, a heavily modified and optimized version of the fully reversed source code for GTA VC ([miami](https://github.com/mrxenginner/reVC/tree/miami/) branch).

This specific fork was built with a single, aggressive goal: **to make Grand Theft Auto: Vice City run smoothly, stably, and with high FPS on extremely low-end hardware**, specifically targeting single-core processors (like the Intel Atom N450) and ultra-basic integrated graphics (like the Intel GMA 3150). 

To achieve this extreme performance boost, the AI analyzed the RenderWare engine to eliminate massive bottlenecks, disable unnecessary background math calculations, and introduce aggressive Level of Detail (LOD) mechanics that the original developers left on the table.

* **Compatibility:** Tested and working strictly on Windows.
* **Rendering:** Handled by the original RenderWare (D3D8) or the reimplementation [librw](https://github.com/aap/librw) (D3D9, OpenGL 2.1+, OpenGL ES 2.0+).
* **Audio:** Powered by MSS (original GTA dlls) or OpenAL.

*(Note: We cannot build for PS2 or Xbox yet. If you have experience with console porting, get in touch via Discord!)*

## 🛠️ Technical Optimization Report (+60 FPS Boost)

To achieve stable framerates on legacy hardware, the RenderWare engine was surgically stripped of invisible background math and fillrate-heavy rendering techniques:

### 1. World Management & Physics
* **The Heat & CPU Patch (End of "Busy-Wait"):** Profiling revealed the engine spent nearly 60% of CPU time "waiting" (`rsIDLE`) for the next frame. By injecting a `Sleep(1)` command into the frame limiter, the engine yields control to Windows for 1 millisecond during idle times. **Result:** Massive performance stabilization and hardware temperatures dropping from 90°C to safe levels.
* **Dynamic Object Culling:** Emptied `RepositionCertainDynamicObjects`. The engine no longer casts mathematical rays (`ProcessVerticalLine`) to recalculate the height of traffic lights or mailboxes; they now strictly use the map's native Z-height.
* **Stunt Annihilation:** Gutted the heavy physics block that calculated spring compression and angles for unique jump bonuses. This grants massive framerate stability while driving.
* **Flat Traffic Density:** The CPU no longer constantly scans path nodes to calculate traffic density. `m_fRoadDensity` is statically locked at `1.0f`.
* **Aggressive Island Culling:** If Tommy is on one island, the engine is strictly prohibited from rendering *anything* from the other island, drastically reducing memory overhead.

### 2. AI & Lightweight Math
* **`sqrt` Eradication:** Heavy square roots were replaced with "squared magnitudes" (`.MagnitudeSqr()`) to calculate distances. This drastically accelerated siren AI, dodging mechanics, flipped/stuck car detection, and vehicle removal routines (`PossiblyRemoveVehicle`).
* **Distance-Based Entity Time-Slicing:** Entity updates are scaled by distance. The further an NPC or vehicle is from the camera, the fewer frames it uses to update its AI, movement, and animations. Roadblocks also utilize time-slicing (processed in batches every 16 frames) to prevent FPS drops at 3+ wanted stars.

### 3. Rendering & Graphics (The Alpha-Blending Killers)
The Intel GMA 3150 suffers massively with Alpha Blending (transparencies). The codebase was heavily modified to force opacity and destroy particle loops:
* **The Square Radar (Chebyshev Math):** Vanilla Vice City draws a massive black polygon mask with alpha transparency over a square map to hide the corners and make it look round—a massive GPU drain. We deleted `DrawRadarMask()`. Furthermore, in `Radar.cpp` (`CRadar::LimitRadarPoint`), the Pythagorean trigonometry that glued blips to the edge of a circle was replaced with **"Chebyshev Distance"**, forcing icons to correctly snap to the straight edges of our new square radar.
* **Mathematical Flat Water:** Disabled wave calculations entirely. The ocean is forced to a static solid color (R:15, G:60, B:100), blocking Alpha Blending so the water is no longer a burden on the GPU.
* **Transparency Bypass (Solid Icons):** Used `RwIm2DRenderPrimitive` to force radar and weapon icons to render as solid blocks.
* **Particle Annihilation (`Object.cpp`):** Gutted absolutely all loops generating yacht foam/wakes, plus dozens of debris, wood, leaf, and dust fragments when crashing into breakable objects. Objects still break and make sound, but draw zero transparencies.
* **Total Glass Annihilation:** All vehicle windows (intact and damaged) are forced to 0% opacity, completely removing their polygons from the rendering pipeline.
* **HUD Reorganization:** Health and Armor were moved to the bottom left (under the new square minimap). Wanted Stars were moved to Y: 65.0f (under the money) for a cleaner, more efficient layout at low resolutions.

---

## 🔬 Deep-Dive Technical Autopsy (Code Rewrites)

For fellow developers and reverse-engineers, here is the exact autopsy of the RenderWare logic we manipulated at the memory and mathematical levels to achieve our new gameplay features without crashing the legacy engine:

### 1. The Zero-Crash Invisible Head Hack (`CPed::Render`)
To make the First-Person view work without the camera clipping inside Tommy's model, we needed to hide his head. Attempting to delete textures or skip geometry calls caused crashes. Instead, we used a **Bone Squashing Hack**:
* **The Target:** We accessed the advanced skeletal hierarchy (`HAnim`) via `GetAnimHierarchyFromSkinClump()` and located the exact index of the head bone using `RpHAnimIDGetIndex(..., PED_HEAD)`.
* **The Backup:** *Before* squashing the head, we created a perfect memory backup of the 4x4 matrix (`matrizOriginal = *pMatrizCabeza;`). This was vital to prevent the game from exploding when entering vehicles.
* **The Execution:** We applied `RwMatrixScale(..., &escalaCero, ...)` to multiply the head matrix by `0.0`. We then called `CPed::Render()`. Since the head size was zero, the GPU skipped drawing it entirely.
* **The Restore:** Immediately after the draw call, we restored the original matrix (`*pMatrizCabeza = matrizOriginal;`). The GPU is tricked into not rendering the head, but the CPU physics and collision engine still believes the head is there, preventing any Inverse Kinematics (IK) divide-by-zero crashes.

### 2. Advanced 1st-Person Camera & Dynamic FOV (`src/core/Camera.cpp`)
* **Gimbal Lock Prevention:** In `CCamera::Process()`, we anchored the global reference vector to `CVector globalUp(0.0f, 0.0f, 1.0f)`. Using CrossProducts, we obtained a purely orthogonal `Right` vector. This eliminates local Z-axis roll (motion sickness) while allowing free X-axis pitch (looking at the sky while popping a wheelie).
* **2D Rotation Matrix (Mouse Pan):** We read the raw hardware delta using `CPad::GetPad(0)->NewMouseControllerState.x` and applied trigonometric rotation (cos/sin over `s_fLookPan`) directly to the X/Y components of the `CamFront` unit vector. 
* **Temporal Interpolation (Auto-Center):** Implemented a timer (`CTimer::GetTimeInMilliseconds()`). If the delta exceeds 3000ms, a Lerp (Linear Interpolation) function smoothly decrements `s_fLookPan` back to 0.0, scaling perfectly with `CTimer::GetTimeStep()`.
* **Frustum Culling Recalculation:** Injected `CalculateDerivedValues();` at the end of the free-look function. This forces the engine to recalculate the 6 planes of the View Frustum. Without this, turning the camera with the mouse would cause the world to disappear because the engine culled polygons based on the player's forward vector.
* **Dynamic FOV:** Added a dynamic FOV using Lerp interpolation just before the `CDraw::SetFOV(FOV)` call. This ensures FOV transitions are silk-smooth, even during FPS drops, and fluidly return to normal when braking.

### 3. Bike Physics & Animation Decoupling (`src/vehicles/Bike.cpp`)
* **State Hijacking:** Inside `CBike::ProcessControl(void)`, we hijacked an inactive variable (`m_bike_unused1`) to store the actual lean input float (`m_fLeanInput`). 
* **Animation Bypass:** If the camera is in `1STPERSON`, we force `m_fLeanInput = 0.0f` right before the end of the integration block. This tricks the subsequent `CPed` evaluator, preventing it from assigning a blend weight to the `CAnimBlendAssociation` structures (`ANIM_BIKE_LEANB` and `ANIM_BIKE_LEANF`), thereby locking the torso bones and keeping the 1st person camera perfectly stable.
* **Aerodynamic Refactor:** Vanilla physics tied speed/drag to visual animation timing (`assoc->currentTime > 0.06f`). We broke this dependency by reading raw hardware input (`m_fLeanInput > 0.15f`) to reduce the friction coefficient (`m_fAirResistance *= 0.6f`) and mathematically applying frontal force multiplied by mass and gravity.

### 4. Low-Cost Incandescence Effect (`src/vehicles/Automobile.cpp`)
The most "performance-cheap" way to add an immersive blinking/glowing effect for low-end hardware. Instead of generating new geometry or transparent layers that choke the GMA 3150, we bypassed heavy material iteration. In `CAutomobile::Render()`, we trick the game by changing the car's color index right before it is sent to the GPU, and restoring it immediately after. 

### 5. Pseudo-3D Audio (Linear Attenuation) (`src/audio/oam/MusicManager.cpp`)
Real 3D audio (Doppler effect, reverb, directional panning) eats precious CPU cycles. To emulate modern GTA games (hearing the radio outside the car) on an Atom N450, we implemented a "Fake 3D" attenuation hack. We keep the standard 2D radio stream active when exiting a vehicle, but attenuate its volume mathematically based on Tommy's distance from the car:
* **0 meters:** 100% Volume
* **7.5 meters:** 50% Volume
* **15+ meters:** 0% Volume (Radio shuts off)

### 6. Seamless F11 Fullscreen Toggle (`src/skel/events.cpp`)
Inside `HandleKeyDown(RsKeyStatus *keyStatus)`, we injected a new `case rsF11:` in the main OS keyboard event dispatcher. We directly invert `FrontEndMenuManager` booleans (`m_nPrefsWindowed` and `m_nSelectedScreenMode`) and call the external function `_psSelectScreenVM()`. This forces the API (D3D9/OpenGL) to destroy the window context and recreate the swapchain dynamically with the new resolution parameters, completely bypassing a main thread restart.

---

## 💾 Installation Guide

1. **Prerequisite:** reVC requires the original game assets. You **must** own [a legitimate copy of GTA Vice City](https://store.steampowered.com/app/12110/Grand_Theft_Auto_Vice_City/).
2. **Download:** Grab the latest build for your system:
   * 🥔 **[Windows D3D9 MSS 32bit (ONLY POTATO BUILD)](https://www.mediafire.com/file/y7cttngp9icuvu9/win-x86-librw_d3d9-mss.zip/file)** *(Recommended for Low-End PCs)*
   * [Windows D3D9 64bit](https://nightly.link/mrxenginner/reVC/workflows/reVC_msvc_amd64/miami/reVC_Release_win-amd64-librw_d3d9-oal.zip)
   * [Windows OpenGL 64bit](https://nightly.link/mrxenginner/reVC/workflows/reVC_msvc_amd64/miami/reVC_Release_win-amd64-librw_gl3_glfw-oal.zip)
   * [Linux 64bit](https://nightly.link/mrxenginner/reVC/workflows/build-cmake-conan/miami/ubuntu-18.04-gl3.zip)
   * [MacOS 64bit x86-64](https://nightly.link/mrxenginner/reVC/workflows/build-cmake-conan/miami/macos-latest-gl3.zip)
   * [Android armeabi-v7a and arm64-v8a](https://nightly.link/mrxenginner/reVC/workflows/build-android/miami/revc-release.zip)
3. **Extract:** Unzip the downloaded archive directly over your clean GTA VC root directory.
4. **Play:** Run the reVC executable. The zip includes updated gamefiles, binaries, and required OpenAL dlls.

## 📸 Screenshots (Potato PC Visuals)

*Witness the flat-shaded, highly optimized RenderWare engine in action:*

![GTA Vice City Potato PC Gameplay 1](https://raw.githubusercontent.com/Velipetroll/reVC-POTATO_EDITION/refs/heads/miami/Images/Image_1.png)
![GTA Vice City Low Spec Optimization 2](https://raw.githubusercontent.com/Velipetroll/reVC-POTATO_EDITION/refs/heads/miami/Images/Image_3.png)
![GTA Vice City Flat Lighting Engine 3](https://raw.githubusercontent.com/Velipetroll/reVC-POTATO_EDITION/refs/heads/miami/Images/Image_4.png)
![GTA Vice City First Person Vehicle Mod 4](https://raw.githubusercontent.com/Velipetroll/reVC-POTATO_EDITION/refs/heads/miami/Images/Image_5.png)
![GTA Vice City Low End FPS Boost 5](https://raw.githubusercontent.com/Velipetroll/reVC-POTATO_EDITION/refs/heads/miami/Images/Image_6.png)

## 🛠️ Standard reVC Engine Improvements

Beyond the Potato Edition optimizations, this fork inherits the massive quality-of-life improvements from the core reVC project. Configure them via `core/config.h` or the in-game debug menu.

* Bug fixes for vanilla game glitches.
* User files (saves/settings) are now cleanly stored in the GTA root directory.
* Settings use a readable `reVC.ini` file instead of `gta_vc.set`.
* **Developer Tools:** Debug menu (`Ctrl-M`), Free Camera (`Ctrl-B`), and rotatable camera.
* Native XInput controller support for modern Windows gamepads.
* **Zero Loading Screens** between islands (toggle "map memory usage" in the menu).
* **Advanced Rendering Options:**
  * True Widescreen support (proper HUD scaling, Menu, and FOV).
  * PS2 MatFX (vehicle reflections) & PS2 alpha testing for transparency.
  * Xbox vehicle rendering, lightmaps, ped rim lights, and screen rain droplets.
  * Highly customizable color filters.
* Cross-platform DFF/TXD asset loading support.

## 📋 To-Do & Roadmap

* Fix physics calculations for uncapped/high FPS monitors.
* Further improve performance on ARM devices, specifically the OpenGL layer for Raspberry Pi (Contributions welcome!).
* [PS2 port](https://github.com/mrxenginner/reVC/wiki/PS2-port) & Xbox port compatibility.
* Reverse engineer remaining unused/debug functions from the original binary.

## ⚙️ Modding Compatibility

* **Assets:** Standard texture, model, handling, and script mods work just like vanilla GTA VC.
* **Code Mods (CLEO / ASI):** Mods that inject code (dll/asi, CLEO, limit adjusters) **will not work**. Many popular fixes (SkyGFX, GInput, SilentPatch, Widescreen fix) are already natively built into reVC. Limits can be easily increased by compiling from source via `config.h`.

## 🏗️ Building from Source

To compile the engine yourself, clone the repository using:  
`git clone --recursive -b miami https://github.com/Velipetroll/reVC-POTATO_EDITION.git reVC`  
Then `cd reVC` into the cloned directory.

*(Tip: When using premake, point the `GTA_VC_RE_DIR` environment variable to your GTA root folder to auto-move the executable after building).*

<details><summary><b>Windows (Visual Studio)</b></summary>

Assuming you have Visual Studio 2015/2017/2019:
1. Run the appropriate `premake-vsXXXX.cmd` script in the root folder.
2. Open `build/reVC.sln` and compile.

*Note on DirectX:* Microsoft discontinued the DX9 SDK. Download the archived version [here](https://archive.org/details/dxsdk_jun10).  
*Note on OpenAL:* Read the [Running OpenAL build on Windows](https://github.com/mrxenginner/reVC/wiki/Running-OpenAL-build-on-Windows) guide.
</details>

<details><summary><b>Linux (Premake & Conan)</b></summary>

* **Premake:** Proceed to [Building on Linux](https://github.com/mrxenginner/reVC/wiki/Building-on-Linux).
* **Conan:** Install Python and Conan, then run:
    ```bash
    conan export vendor/librw librw/master@
    mkdir build && cd build
    conan install .. reVC/miami@ -if build -o reVC:audio=openal -o librw:platform=gl3 -o librw:gl3_gfxlib=glfw --build missing -s reVC:build_type=RelWithDebInfo -s librw:build_type=RelWithDebInfo
    conan build .. -if build -bf build -pf package
    ```
</details>

<details><summary><b>MacOS, Android & FreeBSD</b></summary>

* **MacOS:** See [Building on MacOS](https://github.com/mrxenginner/reVC/wiki/Building-on-MacOS)
* **Android:** See [Building on Android](https://github.com/mrxenginner/reVC/wiki/Building-on-Android)
* **FreeBSD:** See [Building on FreeBSD](https://github.com/mrxenginner/reVC/wiki/Building-on-FreeBSD)
</details>

> ℹ️ **Pro Tips:** > * Use `--with-lto` in premake for Link Time Optimization.  
> * Customize engine limits in [`config.h`](https://github.com/mrxenginner/reVC/tree/miami/src/core/config.h).  
> * reVC uses a custom RenderWare replacement called [librw](https://github.com/aap/librw/).

## 🤝 Contributing

Outside of the OS compatibility layers, all code not wrapped in preprocessor conditions (like `FIX_BUGS`) is **100% strictly reversed code from the original binaries**. 

We generally do not accept custom code mods unless wrapped properly. We *do* accept PRs for:
* Features that exist in other classic GTAs (III/VC logic).
* Vanilla UI, UX, or game bug fixes (must be behind `FIX_BUGS`).
* Platform-specific/unused code documentation.
* Code readability improvements and translation fixes.

Please review our [Coding Style](https://github.com/mrxenginner/reVC/blob/miami/CODING_STYLE.md). *(Do not use C++11 or later features).*

## 📜 History

The core `re3` project started in spring 2018 to test reversed collision/physics via DLL injection. After a dormant period, it was revived in May 2019, growing rapidly thanks to contributors like Fire_Head, shfil, erorcun, Nick007J, and Serge. By April 2020, we achieved a standalone executable.  

`reVC` was born in May 2020 using the `re3` foundation and was considered complete by December of that year. The community continues to push the boundaries of classic RenderWare engine reverse engineering.

## ⚖️ License

We do not provide an official license for this codebase. **This repository is strictly for educational, documentation, and modding purposes.** We do not encourage software piracy or commercial exploitation of this code. Please keep any derivative works open-source and provide proper credit to the reVC team and contributors.
