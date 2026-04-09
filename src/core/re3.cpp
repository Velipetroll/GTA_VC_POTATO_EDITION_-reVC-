#include <csignal>
#define WITHWINDOWS
#include "common.h"
#if defined DETECT_JOYSTICK_MENU && defined XINPUT
#include <xinput.h>
#if !defined(PSAPI_VERSION) || (PSAPI_VERSION > 1)
#pragma comment( lib, "Xinput9_1_0.lib" )
#else
#pragma comment( lib, "Xinput.lib" )
#endif
#endif
#include "Renderer.h"
#include "Occlusion.h"
#include "Credits.h"
#include "Camera.h"
#include "Weather.h"
#include "Timecycle.h"
#include "Clock.h"
#include "World.h"
#include "Vehicle.h"
#include "ModelIndices.h"
#include "Streaming.h"
#include "Boat.h"
#include "Heli.h"
#include "Automobile.h"
#include "Bike.h"
#include "Console.h"
#include "Debug.h"
#include "Hud.h"
#include "SceneEdit.h"
#include "Pad.h"
#include "PlayerPed.h"
#include "Radar.h"
#include "debugmenu.h"
#include "Frontend.h"
#include "WaterLevel.h"
#include "main.h"
#include "Script.h"
#include "MBlur.h"
#include "postfx.h"
#include "custompipes.h"
#include "MemoryHeap.h"
#include "FileMgr.h"
#include "Camera.h"
#include "MBlur.h"
#include "ControllerConfig.h"
#include "CarCtrl.h"
#include "Population.h"
#include "IniFile.h"
#include "Zones.h"

#include "crossplatform.h"

#ifndef _WIN32
#include "assert.h"
#include <stdarg.h>
#endif

#include <list>

#ifdef RWLIBS
extern "C" int vsprintf(char* const _Buffer, char const* const _Format, va_list  _ArgList);
#endif


#ifdef USE_PS2_RAND
unsigned long long myrand_seed = 1;
#else
unsigned long int myrand_seed = 1;
#endif

int
myrand(void)
{
#ifdef USE_PS2_RAND
	myrand_seed = 0x5851F42D4C957F2D * myrand_seed + 1;
	return ((myrand_seed >> 32) & 0x7FFFFFFF);
#else
	myrand_seed = myrand_seed * 1103515245 + 12345;
	return((myrand_seed >> 16) & 0x7FFF);
#endif
}

void
mysrand(unsigned int seed)
{
	myrand_seed = seed;
}

#ifdef CUSTOM_FRONTEND_OPTIONS
#include "frontendoption.h"

#ifdef MORE_LANGUAGES
void LangPolSelect(int8 action)
{
	if (action == FEOPTION_ACTION_SELECT) {
		FrontEndMenuManager.m_PrefsLanguage = CMenuManager::LANGUAGE_POLISH;
		FrontEndMenuManager.m_bFrontEnd_ReloadObrTxtGxt = true;
		FrontEndMenuManager.InitialiseChangedLanguageSettings();
		FrontEndMenuManager.SaveSettings();
	}
}

void LangRusSelect(int8 action)
{
	if (action == FEOPTION_ACTION_SELECT) {
		FrontEndMenuManager.m_PrefsLanguage = CMenuManager::LANGUAGE_RUSSIAN;
		FrontEndMenuManager.m_bFrontEnd_ReloadObrTxtGxt = true;
		FrontEndMenuManager.InitialiseChangedLanguageSettings();
		FrontEndMenuManager.SaveSettings();
	}
}

void LangJapSelect(int8 action)
{
	if (action == FEOPTION_ACTION_SELECT) {
		FrontEndMenuManager.m_PrefsLanguage = CMenuManager::LANGUAGE_JAPANESE;
		FrontEndMenuManager.m_bFrontEnd_ReloadObrTxtGxt = true;
		FrontEndMenuManager.InitialiseChangedLanguageSettings();
		FrontEndMenuManager.SaveSettings();
	}
}
#endif

void
CustomFrontendOptionsPopulate(void)
{
	// OPTIMIZACIÓN GMA 3150: Vaciado por completo para evitar accesos al disco duro 
	// lentos en el arranque (Lecturas innecesarias de texturas Neo, etc.)
}
#endif

#ifdef LOAD_INI_SETTINGS
#define MINI_CASE_SENSITIVE
#include "ini.h"

mINI::INIFile ini("reVC.ini");
mINI::INIStructure cfg;

bool ReadIniIfExists(const char *cat, const char *key, uint32 *out)
{
	mINI::INIMap<std::string> section = cfg.get(cat);
	if (section.has(key)) {
		char *endPtr;
		*out = strtoul(section.get(key).c_str(), &endPtr, 0);
		return true;
	}
	return false;
}

bool ReadIniIfExists(const char *cat, const char *key, uint8 *out)
{
	mINI::INIMap<std::string> section = cfg.get(cat);
	if (section.has(key)) {
		char *endPtr;
		*out = strtoul(section.get(key).c_str(), &endPtr, 0);
		return true;
	}
	return false;
}

bool ReadIniIfExists(const char *cat, const char *key, bool *out)
{
	mINI::INIMap<std::string> section = cfg.get(cat);
	if (section.has(key)) {
		char *endPtr;
		*out = strtoul(section.get(key).c_str(), &endPtr, 0);
		return true;
	}
	return false;
}

bool ReadIniIfExists(const char *cat, const char *key, int32 *out)
{
	mINI::INIMap<std::string> section = cfg.get(cat);
	if (section.has(key)) {
		char *endPtr;
		*out = strtol(section.get(key).c_str(), &endPtr, 0);
		return true;
	}
	return false;
}

bool ReadIniIfExists(const char *cat, const char *key, int8 *out)
{
	mINI::INIMap<std::string> section = cfg.get(cat);
	if (section.has(key)) {
		char *endPtr;
		*out = strtol(section.get(key).c_str(), &endPtr, 0);
		return true;
	}
	return false;
}

bool ReadIniIfExists(const char *cat, const char *key, float *out)
{
	mINI::INIMap<std::string> section = cfg.get(cat);
	if (section.has(key)) {
		char *endPtr;
		*out = strtof(section.get(key).c_str(), &endPtr);
		return true;
	}
	return false;
}

bool ReadIniIfExists(const char *cat, const char *key, char *out, int size)
{
	mINI::INIMap<std::string> section = cfg.get(cat);
	if (section.has(key)) {
		strncpy(out, section.get(key).c_str(), size - 1);
		out[size - 1] = '\0';
		return true;
	}
	return false;
}

void StoreIni(const char *cat, const char *key, uint32 val)
{
	char temp[11];
	sprintf(temp, "%u", val);
	cfg[cat][key] = temp;
}

void StoreIni(const char *cat, const char *key, uint8 val)
{
	char temp[11];
	sprintf(temp, "%u", val);
	cfg[cat][key] = temp;
}

void StoreIni(const char *cat, const char *key, int32 val)
{
	char temp[11];
	sprintf(temp, "%d", val);
	cfg[cat][key] = temp;
}

void StoreIni(const char *cat, const char *key, int8 val)
{
	char temp[11];
	sprintf(temp, "%d", val);
	cfg[cat][key] = temp;
}

void StoreIni(const char *cat, const char *key, float val)
{
	char temp[50];
	sprintf(temp, "%f", val);
	cfg[cat][key] = temp;
}

void StoreIni(const char *cat, const char *key, char *val, int size)
{
	cfg[cat][key] = val;
}

const char *iniControllerActions[] = { "PED_FIREWEAPON", "PED_CYCLE_WEAPON_RIGHT", "PED_CYCLE_WEAPON_LEFT", "GO_FORWARD", "GO_BACK", "GO_LEFT", "GO_RIGHT", "PED_SNIPER_ZOOM_IN",
	"PED_SNIPER_ZOOM_OUT", "VEHICLE_ENTER_EXIT", "CAMERA_CHANGE_VIEW_ALL_SITUATIONS", "PED_JUMPING", "PED_SPRINT", "PED_LOOKBEHIND", "PED_DUCK", "PED_ANSWER_PHONE", 
#ifdef BIND_VEHICLE_FIREWEAPON
	"VEHICLE_FIREWEAPON",
#endif
	"VEHICLE_ACCELERATE", "VEHICLE_BRAKE", "VEHICLE_CHANGE_RADIO_STATION", "VEHICLE_HORN", "TOGGLE_SUBMISSIONS", "VEHICLE_HANDBRAKE", "PED_1RST_PERSON_LOOK_LEFT",
	"PED_1RST_PERSON_LOOK_RIGHT", "VEHICLE_LOOKLEFT", "VEHICLE_LOOKRIGHT", "VEHICLE_LOOKBEHIND", "VEHICLE_TURRETLEFT", "VEHICLE_TURRETRIGHT", "VEHICLE_TURRETUP", "VEHICLE_TURRETDOWN",
	"PED_CYCLE_TARGET_LEFT", "PED_CYCLE_TARGET_RIGHT", "PED_CENTER_CAMERA_BEHIND_PLAYER", "PED_LOCK_TARGET", "NETWORK_TALK", "PED_1RST_PERSON_LOOK_UP", "PED_1RST_PERSON_LOOK_DOWN",
	"_CONTROLLERACTION_36", "TOGGLE_DPAD", "SWITCH_DEBUG_CAM_ON", "TAKE_SCREEN_SHOT", "SHOW_MOUSE_POINTER_TOGGLE", "UNKNOWN_ACTION" };

const char *iniControllerTypes[] = { "kbd:", "2ndKbd:", "mouse:", "joy:" };

const char *iniMouseButtons[] = {"LEFT","MIDDLE","RIGHT","WHLUP","WHLDOWN","X1","X2"};

const char *iniKeyboardButtons[] = {"ESC","F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12",
	"INS","DEL","HOME","END","PGUP","PGDN","UP","DOWN","LEFT","RIGHT","DIVIDE","TIMES","PLUS","MINUS","PADDEL",
	"PADEND","PADDOWN","PADPGDN","PADLEFT","PAD5","NUMLOCK","PADRIGHT","PADHOME","PADUP","PADPGUP","PADINS",
	"PADENTER", "SCROLL","PAUSE","BACKSP","TAB","CAPSLK","ENTER","LSHIFT","RSHIFT","SHIFT","LCTRL","RCTRL","LALT",
	"RALT", "LWIN", "RWIN", "APPS", "NULL"};

void LoadINIControllerSettings()
{
#ifdef DETECT_JOYSTICK_MENU
#ifdef XINPUT
	int storedJoy1 = -1;
	if (ReadIniIfExists("Controller", "JoystickName", &storedJoy1)) {
		CPad::XInputJoy1 = -1;
		CPad::XInputJoy2 = -1;
		XINPUT_STATE xstate;
		memset(&xstate, 0, sizeof(XINPUT_STATE));

		if (XInputGetState(storedJoy1, &xstate) == ERROR_SUCCESS) {
			CPad::XInputJoy1 = storedJoy1;
		}

		for (int i = 0; i <= 3; i++) {
			if (XInputGetState(i, &xstate) == ERROR_SUCCESS) {
				if (CPad::XInputJoy1 == -1)
					CPad::XInputJoy1 = i;
				else if (CPad::XInputJoy2 == -1 && i != CPad::XInputJoy1)
					CPad::XInputJoy2 = i;
			}
		}

		if (CPad::XInputJoy1 == -1) {
			CPad::XInputJoy1 = 0;
			CPad::XInputJoy2 = 1;
		} else if (CPad::XInputJoy2 == -1) {
			CPad::XInputJoy2 = (CPad::XInputJoy1 + 1) % 4;
		}
	}
#else
	ReadIniIfExists("Controller", "JoystickName", gSelectedJoystickName, 128);
#endif
#endif
	if (!ReadIniIfExists("Controller", "PadButtonsInited", &ControlsManager.ms_padButtonsInited)) {
		ControlsManager.ms_padButtonsInited = cfg.get("Bindings").size() != 0 ? 16 : 0;
	}

	for (int32 i = 0; i < MAX_CONTROLLERACTIONS; i++) {
		char value[128];
		if (ReadIniIfExists("Bindings", iniControllerActions[i], value, 128)) {
			for (int32 j = 0; j < MAX_CONTROLLERTYPES; j++){
				ControlsManager.ClearSettingsAssociatedWithAction((e_ControllerAction)i, (eControllerType)j);
			}

			for (char *binding = strtok(value,", "); binding != nil; binding = strtok(nil, ", ")) {
				int contType = -1;
				for (int32 k = 0; k < ARRAY_SIZE(iniControllerTypes); k++) {
					int len = strlen(iniControllerTypes[k]);
					if (strncmp(binding, iniControllerTypes[k], len) == 0) {
						contType = k;
						binding += len;
						break;
					}
				}
				if (contType == -1)
					continue;

				int contKey;
				if (contType == JOYSTICK) {
					char *temp;
					contKey = strtol(binding, &temp, 0);

				} else if (contType == KEYBOARD || contType == OPTIONAL_EXTRA) {
					if (strlen(binding) == 1) {
						contKey = binding[0];
					} else if(strcmp(binding, "SPC") == 0) {
						contKey = ' ';
					} else {
						for (int32 k = 0; k < ARRAY_SIZE(iniKeyboardButtons); k++) {
							if(strcmp(binding, iniKeyboardButtons[k]) == 0) {
								contKey = 1000 + k;
								break;
							}
						}
					}
				} else if (contType == MOUSE) {
					for (int32 k = 0; k < ARRAY_SIZE(iniMouseButtons); k++) {
						if(strcmp(binding, iniMouseButtons[k]) == 0) {
							contKey = 1 + k;
							break;
						}
					}
				}

				ControlsManager.SetControllerKeyAssociatedWithAction((e_ControllerAction)i, contKey, (eControllerType)contType);
			}
		}
	}
}

void SaveINIControllerSettings()
{
	for (int32 i = 0; i < MAX_CONTROLLERACTIONS; i++) {
		char value[128] = { '\0' };

		for (int32 j = SETORDER_1; j < MAX_SETORDERS; j++){

			for (int32 k = 0; k < MAX_CONTROLLERTYPES; k++){
				if (ControlsManager.m_aSettings[i][k].m_ContSetOrder == j) {
					char next[32];
					if (k == JOYSTICK) {
						snprintf(next, 32, "%s%d,", iniControllerTypes[k], ControlsManager.m_aSettings[i][k].m_Key);

					} else if (k == KEYBOARD || k == OPTIONAL_EXTRA) {
						if (ControlsManager.m_aSettings[i][k].m_Key == ' ')
							snprintf(next, 32, "%sSPC,", iniControllerTypes[k]);
						else if (ControlsManager.m_aSettings[i][k].m_Key < 256)
							snprintf(next, 32, "%s%c,", iniControllerTypes[k], ControlsManager.m_aSettings[i][k].m_Key);
						else
							snprintf(next, 32, "%s%s,", iniControllerTypes[k], iniKeyboardButtons[ControlsManager.m_aSettings[i][k].m_Key - 1000]);

					} else if (k == MOUSE) {
						snprintf(next, 32, "%s%s,", iniControllerTypes[k], iniMouseButtons[ControlsManager.m_aSettings[i][k].m_Key - 1]);
					}
					strcat(value, next);
					break;
				}
			}
		}
		int len = strlen(value);
		if (len > 0)
			value[len - 1] = '\0'; 

		StoreIni("Bindings", iniControllerActions[i], value, 128);
	}

#ifdef DETECT_JOYSTICK_MENU
#ifdef XINPUT
	StoreIni("Controller", "JoystickName", CPad::XInputJoy1);
#else
	StoreIni("Controller", "JoystickName", gSelectedJoystickName, 128);
#endif
#endif
	StoreIni("Controller", "PadButtonsInited", ControlsManager.ms_padButtonsInited);

	ini.write(cfg);
}

bool LoadINISettings()
{
	if (!ini.read(cfg))
		return false;

#ifdef IMPROVED_VIDEOMODE
	ReadIniIfExists("VideoMode", "Width", &FrontEndMenuManager.m_nPrefsWidth);
	ReadIniIfExists("VideoMode", "Height", &FrontEndMenuManager.m_nPrefsHeight);
	ReadIniIfExists("VideoMode", "Depth", &FrontEndMenuManager.m_nPrefsDepth);
	ReadIniIfExists("VideoMode", "Subsystem", &FrontEndMenuManager.m_nPrefsSubsystem);
#else
	ReadIniIfExists("Graphics", "VideoMode", &FrontEndMenuManager.m_nDisplayVideoMode);
#endif
	ReadIniIfExists("Controller", "HeadBob1stPerson", &TheCamera.m_bHeadBob);
	ReadIniIfExists("Controller", "HorizantalMouseSens", &TheCamera.m_fMouseAccelHorzntl);
	ReadIniIfExists("Controller", "InvertMouseVertically", &MousePointerStateHelper.bInvertVertically);
	ReadIniIfExists("Controller", "DisableMouseSteering", &CVehicle::m_bDisableMouseSteering);
	ReadIniIfExists("Controller", "Vibration", &FrontEndMenuManager.m_PrefsUseVibration);
	ReadIniIfExists("Audio", "SfxVolume", &FrontEndMenuManager.m_PrefsSfxVolume);
	ReadIniIfExists("Audio", "MusicVolume", &FrontEndMenuManager.m_PrefsMusicVolume);
	ReadIniIfExists("Audio", "MP3BoostVolume", &FrontEndMenuManager.m_PrefsMP3BoostVolume);
	ReadIniIfExists("Audio", "Radio", &FrontEndMenuManager.m_PrefsRadioStation);
#ifdef EXTERNAL_3D_SOUND
	ReadIniIfExists("Audio", "SpeakerType", &FrontEndMenuManager.m_PrefsSpeakers);
	ReadIniIfExists("Audio", "Provider", &FrontEndMenuManager.m_nPrefsAudio3DProviderIndex);
#endif
	ReadIniIfExists("Audio", "DynamicAcoustics", &FrontEndMenuManager.m_PrefsDMA);
	ReadIniIfExists("Display", "Brightness", &FrontEndMenuManager.m_PrefsBrightness);
	ReadIniIfExists("Display", "DrawDistance", &FrontEndMenuManager.m_PrefsLOD);
	ReadIniIfExists("Display", "Subtitles", &FrontEndMenuManager.m_PrefsShowSubtitles);
	ReadIniIfExists("Graphics", "AspectRatio", &FrontEndMenuManager.m_PrefsUseWideScreen);
	ReadIniIfExists("Graphics", "FrameLimiter", &FrontEndMenuManager.m_PrefsFrameLimiter);
#ifdef LEGACY_MENU_OPTIONS
	ReadIniIfExists("Graphics", "VSync", &FrontEndMenuManager.m_PrefsVsyncDisp);
	ReadIniIfExists("Graphics", "Trails", &CMBlur::BlurOn);
#endif
	ReadIniIfExists("General", "SkinFile", FrontEndMenuManager.m_PrefsSkinFile, 256);
	ReadIniIfExists("Controller", "Method", &FrontEndMenuManager.m_ControlMethod);
	ReadIniIfExists("General", "Language", &FrontEndMenuManager.m_PrefsLanguage);
	ReadIniIfExists("Display", "ShowHud", &FrontEndMenuManager.m_PrefsShowHud);
	ReadIniIfExists("Display", "RadarMode", &FrontEndMenuManager.m_PrefsRadarMode);
	ReadIniIfExists("Display", "ShowLegends", &FrontEndMenuManager.m_PrefsShowLegends);

#ifdef EXTENDED_COLOURFILTER
	ReadIniIfExists("CustomPipesValues", "PostFXIntensity", &CPostFX::Intensity);
#endif
#ifdef EXTENDED_PIPELINES
	ReadIniIfExists("CustomPipesValues", "NeoVehicleShininess", &CustomPipes::VehicleShininess);
	ReadIniIfExists("CustomPipesValues", "NeoVehicleSpecularity", &CustomPipes::VehicleSpecularity);
	ReadIniIfExists("CustomPipesValues", "RimlightMult", &CustomPipes::RimlightMult);
	ReadIniIfExists("CustomPipesValues", "LightmapMult", &CustomPipes::LightmapMult);
	ReadIniIfExists("CustomPipesValues", "GlossMult", &CustomPipes::GlossMult);
#endif
	ReadIniIfExists("Rendering", "BackfaceCulling", &gBackfaceCulling);
#ifdef NEW_RENDERER
	ReadIniIfExists("Rendering", "NewRenderer", &gbNewRenderer);
#endif

#ifdef PROPER_SCALING
	ReadIniIfExists("Draw", "ProperScaling", &CDraw::ms_bProperScaling);	
#endif
#ifdef FIX_RADAR
	ReadIniIfExists("Draw", "FixRadar", &CDraw::ms_bFixRadar);	
#endif
#ifdef FIX_SPRITES
	ReadIniIfExists("Draw", "FixSprites", &CDraw::ms_bFixSprites);	
#endif
#ifdef DRAW_GAME_VERSION_TEXT
	ReadIniIfExists("General", "DrawVersionText", &gbDrawVersionText);
#endif
#ifdef NO_MOVIES
	ReadIniIfExists("General", "NoMovies", &gbNoMovies);
#endif

#ifdef CUSTOM_FRONTEND_OPTIONS
	bool migrate = cfg.get("FrontendOptions").size() != 0;
	for (int i = 0; i < MENUPAGES; i++) {
		for (int j = 0; j < NUM_MENUROWS; j++) {
			CMenuScreenCustom::CMenuEntry &option = aScreens[i].m_aEntries[j];
			if (option.m_Action == MENUACTION_NOTHING)
				break;
				
			if (option.m_Action < MENUACTION_NOTHING && option.m_CFO->save) {
				if (migrate && ReadIniIfExists("FrontendOptions", option.m_CFO->save, (int8*)option.m_CFO->value))
					cfg["FrontendOptions"].remove(option.m_CFO->save);
				else if (option.m_Action == MENUACTION_CFO_SLIDER)
					ReadIniIfExists(option.m_CFO->saveCat, option.m_CFO->save, (float*)option.m_CFO->value);
				else
					ReadIniIfExists(option.m_CFO->saveCat, option.m_CFO->save, (int8*)option.m_CFO->value);

				if (option.m_Action == MENUACTION_CFO_SELECT) {
					option.m_CFOSelect->lastSavedValue = option.m_CFOSelect->displayedValue = *(int8*)option.m_CFO->value;
				}
			}
		}
	}
#endif

#ifdef PED_CAR_DENSITY_SLIDERS
	CPopulation::MaxNumberOfPedsInUse = DEFAULT_MAX_NUMBER_OF_PEDS * CIniFile::PedNumberMultiplier;
	CPopulation::MaxNumberOfPedsInUseInterior = DEFAULT_MAX_NUMBER_OF_PEDS_INTERIOR * CIniFile::PedNumberMultiplier;
	CCarCtrl::MaxNumberOfCarsInUse = DEFAULT_MAX_NUMBER_OF_CARS * CIniFile::CarNumberMultiplier;
#endif

	// OPTIMIZACIÓN GMA 3150: Forzar opciones gráficas pesadas a falso
	CMBlur::BlurOn = false;
#ifdef EXTENDED_COLOURFILTER
	CPostFX::BlurOn = false;
	CPostFX::MotionBlurOn = false;
#endif

	return true;
}

void SaveINISettings()
{
#ifdef IMPROVED_VIDEOMODE
	StoreIni("VideoMode", "Width", FrontEndMenuManager.m_nPrefsWidth);
	StoreIni("VideoMode", "Height", FrontEndMenuManager.m_nPrefsHeight);
	StoreIni("VideoMode", "Depth", FrontEndMenuManager.m_nPrefsDepth);
	StoreIni("VideoMode", "Subsystem", FrontEndMenuManager.m_nPrefsSubsystem);
#else
	StoreIni("Graphics", "VideoMode", FrontEndMenuManager.m_nDisplayVideoMode);
#endif
	StoreIni("Controller", "HeadBob1stPerson", TheCamera.m_bHeadBob);
	StoreIni("Controller", "HorizantalMouseSens", TheCamera.m_fMouseAccelHorzntl);
	StoreIni("Controller", "InvertMouseVertically", MousePointerStateHelper.bInvertVertically);
	StoreIni("Controller", "DisableMouseSteering", CVehicle::m_bDisableMouseSteering);
	StoreIni("Controller", "Vibration", FrontEndMenuManager.m_PrefsUseVibration);
	StoreIni("Audio", "SfxVolume", FrontEndMenuManager.m_PrefsSfxVolume);
	StoreIni("Audio", "MusicVolume", FrontEndMenuManager.m_PrefsMusicVolume);
	StoreIni("Audio", "MP3BoostVolume", FrontEndMenuManager.m_PrefsMP3BoostVolume);
	StoreIni("Audio", "Radio", FrontEndMenuManager.m_PrefsRadioStation);
#ifdef EXTERNAL_3D_SOUND
	StoreIni("Audio", "SpeakerType", FrontEndMenuManager.m_PrefsSpeakers);
	StoreIni("Audio", "Provider", FrontEndMenuManager.m_nPrefsAudio3DProviderIndex);
#endif
	StoreIni("Audio", "DynamicAcoustics", FrontEndMenuManager.m_PrefsDMA);
	StoreIni("Display", "Brightness", FrontEndMenuManager.m_PrefsBrightness);
	StoreIni("Display", "DrawDistance", FrontEndMenuManager.m_PrefsLOD);
	StoreIni("Display", "Subtitles", FrontEndMenuManager.m_PrefsShowSubtitles);
	StoreIni("Graphics", "AspectRatio", FrontEndMenuManager.m_PrefsUseWideScreen);
#ifdef LEGACY_MENU_OPTIONS
	StoreIni("Graphics", "VSync", FrontEndMenuManager.m_PrefsVsyncDisp);
	StoreIni("Graphics", "Trails", CMBlur::BlurOn);
#endif
	StoreIni("Graphics", "FrameLimiter", FrontEndMenuManager.m_PrefsFrameLimiter);
	StoreIni("General", "SkinFile", FrontEndMenuManager.m_PrefsSkinFile, 256);
	StoreIni("Controller", "Method", FrontEndMenuManager.m_ControlMethod);
	StoreIni("General", "Language", FrontEndMenuManager.m_PrefsLanguage);
	StoreIni("Display", "ShowHud", FrontEndMenuManager.m_PrefsShowHud);
	StoreIni("Display", "RadarMode", FrontEndMenuManager.m_PrefsRadarMode);
	StoreIni("Display", "ShowLegends", FrontEndMenuManager.m_PrefsShowLegends);

#ifdef EXTENDED_COLOURFILTER
	StoreIni("CustomPipesValues", "PostFXIntensity", CPostFX::Intensity);
#endif
#ifdef EXTENDED_PIPELINES
	StoreIni("CustomPipesValues", "NeoVehicleShininess", CustomPipes::VehicleShininess);
	StoreIni("CustomPipesValues", "NeoVehicleSpecularity", CustomPipes::VehicleSpecularity);
	StoreIni("CustomPipesValues", "RimlightMult", CustomPipes::RimlightMult);
	StoreIni("CustomPipesValues", "LightmapMult", CustomPipes::LightmapMult);
	StoreIni("CustomPipesValues", "GlossMult", CustomPipes::GlossMult);
#endif
	StoreIni("Rendering", "BackfaceCulling", gBackfaceCulling);
#ifdef NEW_RENDERER
	StoreIni("Rendering", "NewRenderer", gbNewRenderer);
#endif

#ifdef PROPER_SCALING	
	StoreIni("Draw", "ProperScaling", CDraw::ms_bProperScaling);	
#endif
#ifdef FIX_RADAR
	StoreIni("Draw", "FixRadar", CDraw::ms_bFixRadar);
#endif
#ifdef FIX_SPRITES
	StoreIni("Draw", "FixSprites", CDraw::ms_bFixSprites);	
#endif
#ifdef DRAW_GAME_VERSION_TEXT
	StoreIni("General", "DrawVersionText", gbDrawVersionText);
#endif
#ifdef NO_MOVIES
	StoreIni("General", "NoMovies", gbNoMovies);
#endif
#ifdef CUSTOM_FRONTEND_OPTIONS
	for (int i = 0; i < MENUPAGES; i++) {
		for (int j = 0; j < NUM_MENUROWS; j++) {
			CMenuScreenCustom::CMenuEntry &option = aScreens[i].m_aEntries[j];
			if (option.m_Action == MENUACTION_NOTHING)
				break;
				
			if (option.m_Action < MENUACTION_NOTHING && option.m_CFO->save) {
				if (option.m_Action == MENUACTION_CFO_SLIDER)
					StoreIni(option.m_CFO->saveCat, option.m_CFO->save, *(float*)option.m_CFO->value);
				else
					StoreIni(option.m_CFO->saveCat, option.m_CFO->save, *(int8*)option.m_CFO->value);
			}
		}
	}
#endif

	ini.write(cfg);
}

#endif

#ifdef DEBUGMENU
void
DebugMenuPopulate(void)
{
	// OPTIMIZACIÓN GMA 3150: Menú de debug vaciado por completo para ahorrar RAM, 
	// evitar el registro de cientos de strings y acelerar el tiempo de inicialización de CPU.
}
#endif

#ifndef __MWERKS__
#ifndef MASTER
const int   re3_buffsize = 1024;
static char re3_buff[re3_buffsize];
#endif

#ifndef MASTER
void re3_assert(const char *expr, const char *filename, unsigned int lineno, const char *func)
{
	// OPTIMIZACIÓN: Vaciamos el string de error gigante. Cierra silenciosamente si hay error crítico.
#ifdef _WIN32
	_exit(3);
#else
	assert(false);
#endif
}
#endif

void re3_debug(const char *format, ...)
{
	// OPTIMIZACIÓN GMA 3150: Formateo de strings (vsprintf) bloqueado para no gastar CPU por frame.
}

#ifndef MASTER
void re3_trace(const char *filename, unsigned int lineno, const char *func, const char *format, ...)
{
	// OPTIMIZACIÓN GMA 3150: Sin traces.
}
#endif

#ifndef MASTER
void re3_usererror(const char *format, ...)
{
#ifdef _WIN32
	_exit(3);
#else
	assert(false);
#endif
}
#endif
#endif

#ifdef VALIDATE_SAVE_SIZE
int32 _saveBufCount;
#endif

// Stubs para el sistema de tweak (optimización GMA 3150)
// Implementaciones vacías para satisfacer al enlazador
void CTweakFunc::AddDBG(const char*) {}
void CTweakUInt32::AddDBG(const char*) {}
void CTweakBool::AddDBG(const char*) {}
void CTweakInt32::AddDBG(const char*) {}
void CTweakFloat::AddDBG(const char*) {}
void CTweakVars::Add(CTweakVar*) {}