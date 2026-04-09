#ifdef _WIN32
#include <windows.h>
#endif
#include "common.h"
#include "platform.h"
#include "Game.h"
#include "main.h"
#include "RwHelper.h"
#include "Accident.h"
#include "Antennas.h"
#include "Bridge.h"
#include "CarCtrl.h"
#include "CarGen.h"
#include "CdStream.h"
#include "Clock.h"
#include "Clouds.h"
#include "Collision.h"
#include "Console.h"
#include "Coronas.h"
#include "Cranes.h"
#include "Credits.h"
#include "CutsceneMgr.h"
#include "DMAudio.h"
#include "Darkel.h"
#include "Debug.h"
#include "EventList.h"
#include "FileLoader.h"
#include "FileMgr.h"
#include "Fire.h"
#include "Fluff.h"
#include "Font.h"
#include "Frontend.h"
#include "frontendoption.h"
#include "GameLogic.h"
#include "Garages.h"
#include "GenericGameStorage.h"
#include "Glass.h"
#include "HandlingMgr.h"
#include "Heli.h"
#include "Hud.h"
#include "IniFile.h"
#include "Lights.h"
#include "MBlur.h"
#include "Messages.h"
#include "MemoryCard.h"
#include "MemoryHeap.h"
#include "Pad.h"
#include "Particle.h"
#include "ParticleObject.h"
#include "PedRoutes.h"
#include "Phones.h"
#include "Pickups.h"
#include "Plane.h"
#include "PlayerSkin.h"
#include "Population.h"
#include "Radar.h"
#include "Record.h"
#include "References.h"
#include "Renderer.h"
#include "Replay.h"
#include "Restart.h"
#include "RoadBlocks.h"
#include "Rubbish.h"
#include "SceneEdit.h"
#include "Script.h"
#include "Shadows.h"
#include "Skidmarks.h"
#include "SetPieces.h"
#include "SpecialFX.h"
#include "Stats.h"
#include "Streaming.h"
#include "SurfaceTable.h"
#include "TempColModels.h"
#include "Timecycle.h"
#include "TrafficLights.h"
#include "Train.h"
#include "TxdStore.h"
#include "User.h"
#include "VisibilityPlugins.h"
#include "WaterCannon.h"
#include "WaterLevel.h"
#include "Weapon.h"
#include "WeaponEffects.h"
#include "Weather.h"
#include "World.h"
#include "ZoneCull.h"
#include "Zones.h"
#include "Occlusion.h"
#include "debugmenu.h"
#include "Ropes.h"
#include "WindModifiers.h"
#include "WaterCreatures.h"
#include "postfx.h"
#include "custompipes.h"
#include "screendroplets.h"
#include "VarConsole.h"
#ifdef USE_TEXTURE_POOL
#include "TexturePools.h"
#endif

eLevelName CGame::currLevel;
int32 CGame::currArea;
bool CGame::bDemoMode = true;
bool CGame::nastyGame = true;
bool CGame::frenchGame;
bool CGame::germanGame;
bool CGame::noProstitutes;
bool CGame::playingIntro;
char CGame::aDatFile[32];
#ifdef MORE_LANGUAGES
bool CGame::russianGame = false;
bool CGame::japaneseGame = false;
#endif
#ifndef MASTER
CVector CGame::PlayerCoords;
bool8 CGame::VarUpdatePlayerCoords;
#endif

int gameTxdSlot;

#ifdef SECUROM
uint8 gameProcessPirateCheck = 0;
#endif

bool DoRWStuffStartOfFrame(int16 TopRed, int16 TopGreen, int16 TopBlue, int16 BottomRed, int16 BottomGreen, int16 BottomBlue, int16 Alpha);
void DoRWStuffEndOfFrame(void);
#ifdef PS2_MENU
void MessageScreen(char *msg)
{
	CRect rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	CRGBA color(255, 255, 255, 255);

	DoRWStuffStartOfFrame(50, 50, 50, 0, 0, 0, 255);

	CSprite2d::InitPerFrame();
	CFont::InitPerFrame();
	DefinedState();

	CSprite2d *splash = LoadSplash(NULL);
	splash->Draw(rect, color, color, color, color);
	splash->DrawRect(CRect(SCREEN_SCALE_X(20.0f), SCREEN_SCALE_Y(110.0f), SCREEN_SCALE_X(620.0f), SCREEN_SCALE_Y(300.0f)), CRGBA(50, 50, 50, 192));

	CFont::SetFontStyle(FONT_BANK);
	CFont::SetBackgroundOff();
	CFont::SetWrapx(SCREEN_SCALE_FROM_RIGHT(190.0f));
	CFont::SetScale(SCREEN_SCALE_X(1.0f), SCREEN_SCALE_Y(1.0f));
	CFont::SetCentreOn();
	CFont::SetCentreSize(SCREEN_SCALE_X(450.0f));
	CFont::SetJustifyOff();
	CFont::SetColor(CRGBA(255, 255, 255, 255));
	CFont::SetDropColor(CRGBA(32, 32, 32, 255));
	CFont::SetDropShadowPosition(3);
	CFont::SetPropOn();
	CFont::PrintString(SCREEN_SCALE_X(320.0f), SCREEN_SCALE_Y(130.0f), TheText.Get(msg));
	CFont::DrawFonts();

	DoRWStuffEndOfFrame();
}
#endif

bool
CGame::InitialiseOnceBeforeRW(void)
{
	CFileMgr::Initialise();
	CdStreamInit(MAX_CDCHANNELS);
#ifdef EXTENDED_COLOURFILTER
	CPostFX::InitOnce();
#endif
#ifdef CUSTOM_FRONTEND_OPTIONS
	CustomFrontendOptionsPopulate();
#endif
	return true;
}

#ifndef LIBRW
#ifdef PS2_MATFX
void ReplaceMatFxCallback();
#endif // PS2_MATFX
#ifdef PS2_ALPHA_TEST
void ReplaceAtomicPipeCallback();
#endif // PS2_ALPHA_TEST
#endif // !LIBRW

bool
CGame::InitialiseRenderWare(void)
{
	ValidateVersion();
#ifdef USE_TEXTURE_POOL
	_TexturePoolsInitialise();
#endif

	CTxdStore::Initialise();
	CVisibilityPlugins::Initialise();

#ifdef GTA_PS2
	RpSkySelectTrueTSClipper(TRUE);
	RpSkySelectTrueTLClipper(TRUE);
	SetupPS2ManagerDefaultLightingCallback();
	PreAllocateRwObjects();
#endif

	Scene.camera = CameraCreate(SCREEN_WIDTH, SCREEN_HEIGHT, TRUE);
	ASSERT(Scene.camera != nil);
	if (!Scene.camera) return (false);

	RwCameraSetFarClipPlane(Scene.camera, 2000.0f);
	RwCameraSetNearClipPlane(Scene.camera, 0.9f);

	CameraSize(Scene.camera, nil, DEFAULT_VIEWWINDOW, DEFAULT_ASPECT_RATIO);

	RwBBox  bbox;
	bbox.sup.x = bbox.sup.y = bbox.sup.z = 10000.0f;
	bbox.inf.x = bbox.inf.y = bbox.inf.z = -10000.0f;

	Scene.world = RpWorldCreate(&bbox);
	ASSERT(Scene.world != nil);
	if (!Scene.world)
	{
		CameraDestroy(Scene.camera);
		Scene.camera = nil;
		return (false);
	}

	RpWorldAddCamera(Scene.world, Scene.camera);
	LightsCreate(Scene.world);

	CreateDebugFont();

#ifdef LIBRW
#ifdef PS2_MATFX
	rw::MatFX::envMapApplyLight = true;
	rw::MatFX::envMapUseMatColor = true;
	rw::MatFX::envMapFlipU = true;
#else
	rw::MatFX::envMapApplyLight = false;
	rw::MatFX::envMapUseMatColor = false;
	rw::MatFX::envMapFlipU = false;
#endif
	rw::RGBA envcol = { 64, 64, 64, 255 };
	rw::MatFX::envMapColor = envcol;
#else
#ifdef PS2_MATFX
	ReplaceMatFxCallback();
#endif // PS2_MATFX
#ifdef PS2_ALPHA_TEST
	ReplaceAtomicPipeCallback();
#endif // PS2_ALPHA_TEST
#endif // LIBRW

	PUSH_MEMID(MEMID_TEXTURES);
	CFont::Initialise();
	CHud::Initialise();
	CPlayerSkin::Initialise();
	POP_MEMID();

#ifdef EXTENDED_PIPELINES
	CustomPipes::CustomPipeInit();
#endif
#ifdef SCREEN_DROPLETS
	ScreenDroplets::InitDraw();
#endif

	return (true);
}

void CGame::ShutdownRenderWare(void)
{
#ifdef SCREEN_DROPLETS
	ScreenDroplets::Shutdown();
#endif
#ifdef EXTENDED_PIPELINES
	CustomPipes::CustomPipeShutdown();
#endif

	DestroySplashScreen();
	CHud::Shutdown();
	CFont::Shutdown();

	for (int32 i = 0; i < NUMPLAYERS; i++)
		CWorld::Players[i].DeletePlayerSkin();

	CPlayerSkin::Shutdown();
	DestroyDebugFont();

	LightsDestroy(Scene.world);
	RpWorldRemoveCamera(Scene.world, Scene.camera);
	RpWorldDestroy(Scene.world);
	CameraDestroy(Scene.camera);

	Scene.world = nil;
	Scene.camera = nil;

	CVisibilityPlugins::Shutdown();

#ifdef USE_TEXTURE_POOL
	_TexturePoolsShutdown();
#endif
}

bool CGame::InitialiseOnceAfterRW(void)
{
	TheText.Load();
	CTimer::Initialise();
	CTempColModels::Initialise();
	mod_HandlingManager.Initialise();
	CSurfaceTable::Initialise("DATA\\SURFACE.DAT");
	CPedStats::Initialise();
	CTimeCycle::Initialise();
#ifdef GTA_PS2
	LoadingScreen("Loading the Game", "Initialising audio", GetRandomSplashScreen());
#endif
	DMAudio.Initialise();

#ifndef GTA_PS2
#ifdef EXTERNAL_3D_SOUND
	if (DMAudio.GetNum3DProvidersAvailable() == 0)
		FrontEndMenuManager.m_nPrefsAudio3DProviderIndex = NO_AUDIO_PROVIDER;

	if (FrontEndMenuManager.m_nPrefsAudio3DProviderIndex == AUDIO_PROVIDER_NOT_DETERMINED || FrontEndMenuManager.m_nPrefsAudio3DProviderIndex == -2)
	{
		FrontEndMenuManager.m_PrefsSpeakers = 0;
		FrontEndMenuManager.m_nPrefsAudio3DProviderIndex = DMAudio.AutoDetect3DProviders();
	}

	DMAudio.SetCurrent3DProvider(FrontEndMenuManager.m_nPrefsAudio3DProviderIndex);
	DMAudio.SetSpeakerConfig(FrontEndMenuManager.m_PrefsSpeakers);
#endif
	DMAudio.SetDynamicAcousticModelingStatus(FrontEndMenuManager.m_PrefsDMA);
	DMAudio.SetMusicMasterVolume(FrontEndMenuManager.m_PrefsMusicVolume);
	DMAudio.SetEffectsMasterVolume(FrontEndMenuManager.m_PrefsSfxVolume);
	DMAudio.SetEffectsFadeVol(127);
	DMAudio.SetMusicFadeVol(127);
#endif
	return true;
}

void
CGame::FinalShutdown(void)
{
	CTxdStore::Shutdown();
	CPedStats::Shutdown();
	CdStreamShutdown();
}

bool CGame::Initialise(const char* datFile)
{
	ResetLoadingScreenBar();
	strcpy(aDatFile, datFile);

	CPools::Initialise();

#ifndef GTA_PS2
#ifdef PED_CAR_DENSITY_SLIDERS
	if (CIniFile::PedNumberMultiplier == 0.6f && CIniFile::CarNumberMultiplier == 0.6f)
#endif
		CIniFile::LoadIniFile();
#endif
#ifdef USE_TEXTURE_POOL
	_TexturePoolsUnknown(false);
#endif
	currLevel = LEVEL_BEACH;
	currArea = AREA_MAIN_MAP;

	PUSH_MEMID(MEMID_TEXTURES);
	LoadingScreen("Loading the Game", "Loading generic textures", GetRandomSplashScreen());
	gameTxdSlot = CTxdStore::AddTxdSlot("generic");
	CTxdStore::Create(gameTxdSlot);
	CTxdStore::AddRef(gameTxdSlot);

#ifdef EXTENDED_PIPELINES
	CustomPipes::SetTxdFindCallback();
#endif

	LoadingScreen("Loading the Game", "Loading particles", nil);
	int particleTxdSlot = CTxdStore::AddTxdSlot("particle");
	CTxdStore::LoadTxd(particleTxdSlot, "MODELS/PARTICLE.TXD");
	CTxdStore::AddRef(particleTxdSlot);
	CTxdStore::SetCurrentTxd(gameTxdSlot);
	LoadingScreen("Loading the Game", "Setup game variables", nil);
	POP_MEMID();

	CGameLogic::InitAtStartOfGame();
	CReferences::Init();
	TheCamera.Init();
	TheCamera.SetRwCamera(Scene.camera);
	CDebug::DebugInitTextBuffer();
	ThePaths.Init();
	ThePaths.AllocatePathFindInfoMem(4500);
	CScriptPaths::Init();
	CWeather::Init();
	CCullZones::Init();
	COcclusion::Init();
	CCollision::Init();
	CSetPieces::Init();
	CTheZones::Init();
	CUserDisplay::Init();
	CMessages::Init();
	CMessages::ClearAllMessagesDisplayedByGame();
	CRecordDataForGame::Init();
	CRestart::Initialise();

	PUSH_MEMID(MEMID_WORLD);
	CWorld::Initialise();
	POP_MEMID();

	PUSH_MEMID(MEMID_TEXTURES);
	CParticle::Initialise();
	POP_MEMID();

	PUSH_MEMID(MEMID_ANIMATION);
	CAnimManager::Initialise();
	CCutsceneMgr::Initialise();
	POP_MEMID();

	PUSH_MEMID(MEMID_CARS);
	CCarCtrl::Init();
	POP_MEMID();

	PUSH_MEMID(MEMID_DEF_MODELS);
	InitModelIndices();
	CModelInfo::Initialise();
	CPickups::Init();
	CTheCarGenerators::Init();

	CdStreamAddImage("MODELS\\GTA3.IMG");

	CFileLoader::LoadLevel("DATA\\DEFAULT.DAT");
	CFileLoader::LoadLevel(datFile);

	CWorld::AddParticles();
	CVehicleModelInfo::LoadVehicleColours();
	CVehicleModelInfo::LoadEnvironmentMaps();
	CTheZones::PostZoneCreation();
	POP_MEMID();

	ThePaths.PreparePathData();
	for (int i = 0; i < NUMPLAYERS; i++)
		CWorld::Players[i].Clear();
	CWorld::Players[0].LoadPlayerSkin();
	TestModelIndices();

	CWaterLevel::Initialise("DATA\\WATER.DAT");
	TheConsole.Init();
	CDraw::SetFOV(120.0f);
	CDraw::ms_fLODDistance = 500.0f;

	CStreaming::LoadInitialVehicles();
	CStreaming::LoadInitialPeds();
	CStreaming::RequestBigBuildings(LEVEL_GENERIC);
	CStreaming::LoadAllRequestedModels(false);
	CStreaming::RemoveIslandsNotUsed(currLevel);

	PUSH_MEMID(MEMID_ANIMATION);
	CAnimManager::LoadAnimFiles();
	POP_MEMID();

	CStreaming::LoadInitialWeapons();
	CStreaming::LoadAllRequestedModels(0);
	CPed::Initialise();
	CRouteNode::Initialise();
	CEventList::Initialise();
#ifdef SCREEN_DROPLETS
	ScreenDroplets::Initialise();
#endif
	CRenderer::Init();

	CRadar::Initialise();
	CRadar::LoadTextures();
	CWeapon::InitialiseWeapons();

	CTrafficLights::ScanForLightsOnMap();
	CRoadBlocks::Init();

	CPopulation::Initialise();
	CWorld::PlayerInFocus = 0;
	CCoronas::Init();
	CShadows::Init();
	CWeaponEffects::Init();
	CSkidmarks::Init();
	CAntennas::Init();
	CGlass::Init();
	gPhoneInfo.Initialise();
#ifdef GTA_SCENE_EDIT
	CSceneEdit::Initialise();
#endif

	PUSH_MEMID(MEMID_SCRIPT);
	CTheScripts::Init();
	CGangs::Initialise();
	POP_MEMID();

	CClock::Initialise(1000);
	CHeli::InitHelis();
	CCranes::InitCranes();
	CMovingThings::Init();
	CDarkel::Init();
	CStats::Init();
	CPacManPickups::Init();
	CRubbish::Init();
	CClouds::Init();
	CSpecialFX::Init();
	CRopes::Init();
	CWaterCannons::Init();
	CBridge::Init();
	CGarages::Init();

	CTrain::InitTrains();
	CPlane::InitPlanes();
	CCredits::Init();
	CRecordDataForChase::Init();
	CReplay::Init();

#ifdef PS2_MENU
	if (!TheMemoryCard.m_bWantToLoad)
#endif
	{
		CTheScripts::StartTestScript();
		CTheScripts::Process();
		TheCamera.Process();
	}

	CCollision::ms_collisionInMemory = currLevel;
	for (int i = 0; i < MAX_PADS; i++)
		CPad::GetPad(i)->Clear(true);
#ifdef USE_TEXTURE_POOL
	_TexturePoolsUnknown(true);
#endif

#ifndef MASTER
	PlayerCoords = FindPlayerCoors();
	VarConsole.Add("X PLAYER COORD", &PlayerCoords.x, 10.0f, -10000.0f, 10000.0f, true);
	VarConsole.Add("Y PLAYER COORD", &PlayerCoords.y, 10.0f, -10000.0f, 10000.0f, true);
	VarConsole.Add("Z PLAYER COORD", &PlayerCoords.z, 10.0f, -10000.0f, 10000.0f, true);
	VarConsole.Add("UPDATE PLAYER COORD", &VarUpdatePlayerCoords, true);
#endif

	DMAudio.SetStartingTrackPositions(TRUE);
	DMAudio.ChangeMusicMode(MUSICMODE_GAME);
	return true;
}

bool CGame::ShutDown(void)
{
#ifdef USE_TEXTURE_POOL
	_TexturePoolsUnknown(false);
#endif
	CReplay::FinishPlayback();
	CReplay::EmptyReplayBuffer();
	CPlane::Shutdown();
	CTrain::Shutdown();
	CScriptPaths::Shutdown();
	CWaterCreatures::RemoveAll();
	CSpecialFX::Shutdown();
	CGarages::Shutdown();
	CMovingThings::Shutdown();
	gPhoneInfo.Shutdown();
	CWeapon::ShutdownWeapons();
	CPedType::Shutdown();

	for (int32 i = 0; i < NUMPLAYERS; i++)
	{
		if (CWorld::Players[i].m_pPed)
		{
			CWorld::Remove(CWorld::Players[i].m_pPed);
			delete CWorld::Players[i].m_pPed;
			CWorld::Players[i].m_pPed = nil;
		}

		CWorld::Players[i].Clear();
	}

	CRenderer::Shutdown();
	CWorld::ShutDown();
	DMAudio.DestroyAllGameCreatedEntities();
	CModelInfo::ShutDown();
	CAnimManager::Shutdown();
	CCutsceneMgr::Shutdown();
	CVehicleModelInfo::DeleteVehicleColourTextures();
	CVehicleModelInfo::ShutdownEnvironmentMaps();
	CRadar::Shutdown();
	CStreaming::Shutdown();
	CTxdStore::GameShutdown();
	CCollision::Shutdown();
	CWaterLevel::Shutdown();
	CRubbish::Shutdown();
	CClouds::Shutdown();
	CShadows::Shutdown();
	CCoronas::Shutdown();
	CSkidmarks::Shutdown();
	CWeaponEffects::Shutdown();
	CParticle::Shutdown();
	CPools::ShutDown();
	CHud::ReInitialise();
	CTxdStore::RemoveTxdSlot(gameTxdSlot);
	CMBlur::MotionBlurClose();
	CdStreamRemoveImages();
#ifdef USE_TEXTURE_POOL
	_TexturePoolsFinalShutdown();
#endif
	return true;
}

void CGame::ReInitGameObjectVariables(void)
{
	CGameLogic::InitAtStartOfGame();
#ifdef PS2_MENU
	if (!TheMemoryCard.m_bWantToLoad)
#endif
	{
		TheCamera.Init();
		TheCamera.SetRwCamera(Scene.camera);
	}
	CDebug::DebugInitTextBuffer();
	CWeather::Init();
	CUserDisplay::Init();
	CMessages::Init();
	CRestart::Initialise();
	CWorld::bDoingCarCollisions = false;
	CHud::ReInitialise();
	CRadar::Initialise();
	CCarCtrl::ReInit();
	CTimeCycle::Initialise();
	CDraw::SetFOV(120.0f);
	CDraw::ms_fLODDistance = 500.0f;
	CStreaming::RequestBigBuildings(LEVEL_GENERIC);
	CStreaming::RemoveIslandsNotUsed(LEVEL_BEACH);
	CStreaming::RemoveIslandsNotUsed(LEVEL_MAINLAND);
	CStreaming::LoadAllRequestedModels(false);
	currArea = AREA_MAIN_MAP;
	CPed::Initialise();
	CEventList::Initialise();
#ifdef SCREEN_DROPLETS
	ScreenDroplets::Initialise();
#endif
	CWeapon::InitialiseWeapons();
	CPopulation::Initialise();

	for (int i = 0; i < NUMPLAYERS; i++)
		CWorld::Players[i].Clear();

	CWorld::PlayerInFocus = 0;
	CAntennas::Init();
	CGlass::Init();
	gPhoneInfo.Initialise();

	PUSH_MEMID(MEMID_SCRIPT);
	CTheScripts::Init();
	CGangs::Initialise();
	POP_MEMID();

	CTimer::Initialise();
	CClock::Initialise(1000);
	CTheCarGenerators::Init();
	CHeli::InitHelis();
	CMovingThings::Init();
	CDarkel::Init();
	CStats::Init();
	CPickups::Init();
	CPacManPickups::Init();
	CGarages::Init();
	CSpecialFX::Init();
	CRopes::Init();
	CWaterCannons::Init();
	CScriptPaths::Init();
	CParticle::ReloadConfig();

#ifdef PS2_MENU
	if (!TheMemoryCard.m_bWantToLoad)
#else
	if (!FrontEndMenuManager.m_bWantToLoad)
#endif
	{
		CCranes::InitCranes();
		CTheScripts::StartTestScript();
		CTheScripts::Process();
		TheCamera.Process();
		CTrain::InitTrains();
		CPlane::InitPlanes();
	}

	for (int32 i = 0; i < MAX_PADS; i++)
		CPad::GetPad(i)->Clear(true);
}

void CGame::ReloadIPLs(void) {}

void CGame::ShutDownForRestart(void)
{
#ifdef USE_TEXTURE_POOL
	_TexturePoolsUnknown(false);
#endif
	CReplay::FinishPlayback();
	CReplay::EmptyReplayBuffer();
	DMAudio.DestroyAllGameCreatedEntities();
	CMovingThings::Shutdown();

	for (int i = 0; i < NUMPLAYERS; i++)
		CWorld::Players[i].Clear();

	CGarages::SetAllDoorsBackToOriginalHeight();
	CTheScripts::UndoBuildingSwaps();
	CTheScripts::UndoEntityInvisibilitySettings();
	CWorld::ClearForRestart();
	CGameLogic::ClearShortCut();
	CTimer::Shutdown();
	CStreaming::ReInit();
	CRadar::RemoveRadarSections();
	FrontEndMenuManager.UnloadTextures();
	CParticleObject::RemoveAllExpireableParticleObjects();
	CWaterCreatures::RemoveAll();
	CSetPieces::Init();
	CPedType::Shutdown();
	CSpecialFX::Shutdown();
}

void CGame::InitialiseWhenRestarting(void)
{
	CRect rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	CRGBA color(255, 255, 255, 255);

	CTimer::Initialise();
	CSprite2d::SetRecipNearClip();

	if (b_FoundRecentSavedGameWantToLoad || FrontEndMenuManager.m_bWantToLoad)
	{
		LoadSplash("splash1");
#ifndef XBOX_MESSAGE_SCREEN
		if (FrontEndMenuManager.m_bWantToLoad)
			FrontEndMenuManager.MessageScreen("FELD_WR", true);
#endif
	}

	b_FoundRecentSavedGameWantToLoad = false;
	TheCamera.Init();

	if (FrontEndMenuManager.m_bWantToLoad == true)
	{
#ifdef XBOX_MESSAGE_SCREEN
		FrontEndMenuManager.SetDialogTimer(1000);
		DoRWStuffStartOfFrame(0, 0, 0, 0, 0, 0, 0);
		CSprite2d::InitPerFrame();
		CFont::InitPerFrame();
		FrontEndMenuManager.DrawOverlays();
		DoRWStuffEndOfFrame();
#endif
		RestoreForStartLoad();
	}

	ReInitGameObjectVariables();

	if (FrontEndMenuManager.m_bWantToLoad == true)
	{
		FrontEndMenuManager.m_bWantToLoad = false;
		InitRadioStationPositionList();
		if (GenericLoad() == true)
		{
			DMAudio.ResetTimers(CTimer::GetTimeInMilliseconds());
			CTrain::InitTrains();
			CPlane::InitPlanes();
		}
		else
		{
			for (int32 i = 0; i < 50; i++)
			{
				HandleExit();
				FrontEndMenuManager.MessageScreen("FED_LFL", true);
			}

			TheCamera.SetFadeColour(0, 0, 0);
			ShutDownForRestart();
			CTimer::Stop();
			CTimer::Initialise();
			FrontEndMenuManager.m_bWantToLoad = false;
			ReInitGameObjectVariables();
			currLevel = LEVEL_GENERIC;
			CCollision::SortOutCollisionAfterLoad();
		}
#ifdef XBOX_MESSAGE_SCREEN
		FrontEndMenuManager.ProcessDialogTimer();
#endif
	}

	CTimer::Update();
	DMAudio.ChangeMusicMode(MUSICMODE_GAME);
#ifdef USE_TEXTURE_POOL
	_TexturePoolsUnknown(true);
#endif
}

void CGame::Process(void)
{
	// --- OPTIMIZACIN ATOM N450: TIME-SLICING ---
	// Usamos un contador esttico para alternar qu sistema se actualiza en cada frame.
	// Esto evita que la CPU calcule todo al mismo tiempo.
	static uint32 frameCounter = 0;
	frameCounter++;

	CPad::UpdatePads();
#ifdef USE_CUSTOM_ALLOCATOR
	ProcessTidyUpMemory();
#endif
#ifdef DEBUGMENU
	DebugMenuProcess();
#endif
	CCutsceneMgr::Update();

	if (!CCutsceneMgr::IsCutsceneProcessing() && !CTimer::GetIsCodePaused())
		FrontEndMenuManager.Process();

	CTheZones::Update();
#ifdef SECUROM
	if (CTimer::GetTimeInMilliseconds() >= (35 * 60 * 1000) && gameProcessPirateCheck == 0) {
		gameProcessPirateCheck = 2;
	}
#endif
	CStreaming::Update();
	CWindModifiers::Number = 0;

	if (!CTimer::GetIsPaused())
	{
#ifndef MASTER
		if (VarUpdatePlayerCoords) {
			FindPlayerPed()->Teleport(PlayerCoords);
			VarUpdatePlayerCoords = false;
		}
#endif
		CSprite2d::SetRecipNearClip();
		CSprite2d::InitPerFrame();
		CFont::InitPerFrame();

		// --- OPTIMIZACIN CPU: Reducir escritura en disco o memoria temporal
		if (frameCounter % 4 == 0) {
			CRecordDataForGame::SaveOrRetrieveDataForThisFrame();
			CRecordDataForChase::SaveOrRetrieveDataForThisFrame();
		}

		CPad::DoCheats();
		CClock::Update();
		CWeather::Update();

		PUSH_MEMID(MEMID_SCRIPT);
		CTheScripts::Process();
		POP_MEMID();

		CCollision::Update();
		CScriptPaths::Update();
		CTrain::UpdateTrains();
		CPlane::UpdatePlanes();
		CHeli::UpdateHelis();
		CDarkel::Update();
		CSkidmarks::Update();
		CAntennas::Update();
		CGlass::Update();
#ifdef GTA_SCENE_EDIT
		CSceneEdit::Update();
#endif
		CSetPieces::Update();
		CEventList::Update();
		CParticle::Update();
		gFireManager.Update();

		// --- OPTIMIZACIN ATOM N450: Staggering Poblacin ---
		// En vez de depender del tiempo exacto (que en Atom vara drsticamente),
		// forzamos el clculo fuerte de peatones solo 1 de cada 3 frames.
		if (frameCounter % 3 == 0) {
			CPopulation::Update(true);
		}
		else {
			CPopulation::Update(false);
		}

		CWeapon::UpdateWeapons();
		if (!CCutsceneMgr::IsRunning())
			CTheCarGenerators::Process();
		if (!CReplay::IsPlayingBack())
			CCranes::UpdateCranes();

		// --- OPTIMIZACIN ATOM N450: APAGADO DE SISTEMAS INTILES ---
		// Nubes y caones de agua consumen ciclos iterando matrices, afuera.
		// CClouds::Update(); 
		// CWaterCannons::Update(); 

		// =========================================================
		// --- SISTEMA DE GUARDADO: ABRIR MENÚ (TECLA F6) ---
		// =========================================================
#ifdef _WIN32
		static bool bF6Pressed = false;
		if (GetAsyncKeyState(VK_F6) & 0x8000) {
			// Solo abrimos el menú si la tecla acaba de ser presionada, 
			// no estamos en una cinemática, ni viendo una repetición.
			if (!bF6Pressed && !CCutsceneMgr::IsRunning() && !CReplay::IsPlayingBack()) {

				// Le decimos al motor que queremos guardar (como si pisáramos el disquete rosa del hotel)
				FrontEndMenuManager.m_bWantToLoad = false;
				FrontEndMenuManager.m_bActivateSaveMenu = true;

			}
			bF6Pressed = true;
		}
		else {
			bF6Pressed = false;
		}
#endif
		// =========================================================
		CMovingThings::Update();
		CUserDisplay::Process();
		CReplay::Update();

		PUSH_MEMID(MEMID_WORLD);
		CWorld::Process();
		POP_MEMID();

		gAccidentManager.Update();
		CPacManPickups::Update();
		CPickups::Update();
		CGarages::Update();
		CRubbish::Update();
		CSpecialFX::Update();
		CRopes::Update();
		CTimeCycle::Update();
		if (CReplay::ShouldStandardCameraBeProcessed())
			TheCamera.Process();
		CCullZones::Update();
		if (!CReplay::IsPlayingBack())
			CGameLogic::Update();
		CBridge::Update();

		// --- OPTIMIZACIN ATOM N450: CORONAS Y SOMBRAS BLOQUEADAS ---
		// Destruyen el fillrate y la CPU haciendo raycasts contra el suelo.
		// CCoronas::DoSunAndMoon(); // Comentado opcional, apaga el sol visible en el cielo
		// CCoronas::Update();       // Apaga luces de farolas y flares
		// CShadows::UpdateStaticShadows(); 
		// CShadows::UpdatePermanentShadows(); 

		gPhoneInfo.Update();

		if (!CReplay::IsPlayingBack())
		{
			PUSH_MEMID(MEMID_CARS);

			// --- OPTIMIZACIN ATOM N450: Staggering Trfico ---
			// Solo generamos autos nuevos cada 5 frames, y limpiamos cada 5 (pero desfasado)
			if (frameCounter % 5 == 0) {
				CCarCtrl::GenerateRandomCars();
				CRoadBlocks::GenerateRoadBlocks();
			}
			else if (frameCounter % 5 == 2) {
				CCarCtrl::RemoveDistantCars();
				CCarCtrl::RemoveCarsIfThePoolGetsFull();
			}

			POP_MEMID();
		}
	}
#ifdef GTA_PS2
	CMemCheck::DoTest();
#endif
}

#ifdef USE_CUSTOM_ALLOCATOR
int32 gNumMemMoved;

bool MoveMem(void** ptr)
{
	if (*ptr) {
		gNumMemMoved++;
		void* newPtr = gMainHeap.MoveMemory(*ptr);
		if (*ptr != newPtr) {
			*ptr = newPtr;
			return true;
		}
	}
	return false;
}

struct SkyDataPrefix { uint32 pktSize1; uint32 data; uint32 pktSize2; uint32 unused; };
struct DMAGIFUpload { uint32 tag1_qwc, tag1_addr; uint32 nop1, vif_direct1; uint32 giftag[4]; uint32 gs_bitbltbuf[4]; uint32 tag2_qwc, tag2_addr; uint32 nop2, vif_direct2; };

RwTexture* MoveTextureMemoryCB(RwTexture* texture, void* pData) { return texture; }

bool MoveAtomicMemory(RpAtomic* atomic, bool onlyOne) { return false; }
bool MoveColModelMemory(CColModel& colModel, bool onlyOne) { return false; }

RpAtomic* MoveAtomicMemoryCB(RpAtomic* atomic, void* pData)
{
	bool* pRet = (bool*)pData;
	if (pRet == nil) MoveAtomicMemory(atomic, false);
	else if (MoveAtomicMemory(atomic, true)) { *pRet = true; return nil; }
	return atomic;
}

bool TidyUpModelInfo(CBaseModelInfo* modelInfo, bool onlyone) { return false; }
#endif

void CGame::DrasticTidyUpMemory(bool flushDraw) {}
void CGame::TidyUpMemory(bool moveTextures, bool flushDraw) {}
void CGame::ProcessTidyUpMemory(void) {}

void CGame::InitAfterFocusLoss()
{
	FrontEndMenuManager.m_nPrefsAudio3DProviderIndex = FrontEndMenuManager.m_lastWorking3DAudioProvider;
	DMAudio.SetCurrent3DProvider(FrontEndMenuManager.m_lastWorking3DAudioProvider);

	if (!FrontEndMenuManager.m_bGameNotLoaded && !FrontEndMenuManager.m_bMenuActive)
		FrontEndMenuManager.m_bStartUpFrontEndRequested = true;
}

bool CGame::CanSeeWaterFromCurrArea(void)
{
	return currArea == AREA_MAIN_MAP || currArea == AREA_MANSION || currArea == AREA_HOTEL;
}

bool CGame::CanSeeOutSideFromCurrArea(void)
{
	return currArea == AREA_MAIN_MAP || currArea == AREA_MALL || currArea == AREA_MANSION || currArea == AREA_HOTEL;
}
