#include "common.h"
#include "main.h"
#include "FileMgr.h"
#include "FileLoader.h"
#include "TxdStore.h"
#include "Timer.h"
#include "Weather.h"
#include "Camera.h"
#include "Vehicle.h"
#include "PlayerPed.h"
#include "Boat.h"
#include "World.h"
#include "General.h"
#include "Timecycle.h"
#include "ZoneCull.h"
#include "Clock.h"
#include "Particle.h"
#include "ParticleMgr.h"
#include "RwHelper.h"
#include "Streaming.h"
#include "ColStore.h"
#include "CdStream.h"
#include "Pad.h"
#include "RenderBuffer.h"
#include <rwcore.h>
#include <rpworld.h>
#include <rpmatfx.h>
#include "Occlusion.h"
#include "Replay.h"
#include "WaterLevel.h"
#include "SurfaceTable.h"
#include "WaterCreatures.h"

#define RwIm3DVertexSet_RGBA(vert, rgba) RwIm3DVertexSetRGBA(vert, rgba.red, rgba.green, rgba.blue, rgba.alpha)

float TEXTURE_ADDU;
float TEXTURE_ADDV;
float _TEXTURE_MASK_ADDU;
float _TEXTURE_MASK_ADDV;
float _TEXTURE_WAKE_ADDU;
float _TEXTURE_WAKE_ADDV;

int32 CWaterLevel::ms_nNoOfWaterLevels;
float CWaterLevel::ms_aWaterZs[48];
CRect CWaterLevel::ms_aWaterRects[48];
int8 CWaterLevel::aWaterBlockList[MAX_LARGE_SECTORS][MAX_LARGE_SECTORS];
int8 CWaterLevel::aWaterFineBlockList[MAX_SMALL_SECTORS][MAX_SMALL_SECTORS];
bool CWaterLevel::WavesCalculatedThisFrame;
bool CWaterLevel::RequireWavySector;
bool CWaterLevel::MaskCalculatedThisFrame;
CVector CWaterLevel::PreCalculatedMaskPosn;
bool CWaterLevel::m_bRenderSeaBed;
int32 CWaterLevel::m_nRenderWaterLayers;

// Punteros de geometrías animadas (Se mantienen para no romper headers, pero no se usan)
RpAtomic *CWaterLevel::ms_pWavyAtomic;
RpAtomic *CWaterLevel::ms_pMaskAtomic;
bool gbDontRenderWater;

RwTexture *gpWaterTex;
RwTexture *gpWaterEnvTex;
RwTexture *gpWaterEnvBaseTex;
RwTexture *gpWaterWakeTex;

RwRaster *gpWaterRaster;
RwRaster *gpWaterEnvRaster;
RwRaster *gpWaterEnvBaseRaster;
RwRaster *gpWaterWakeRaster;

bool _bSeaLife;
float _fWaterZOffset = WATER_Z_OFFSET;

// Variables globales de agua mantenidas para el linker
#ifdef PC_WATER
float fEnvScale = 0.25f;
#else
float fEnvScale = 0.5f;
#endif
float fWave2InvLength = 0.03f;
float fWave2NormScale = 0.5f;
float fWave2Ampl = 0.1f;
uint8 nWaterAlpha = 192;
uint8 nWakeAlpha = 192;
float fUnder1 = 4.0;
float fUnder2 = 2.5;
float fUnder3 = 1.5;
int nMaskAlpha = 230;
float fAdd1 = 180.0f;
float fAdd2 = 80.0;
float fRedMult = 0.6f;
float fGreenMult = 1.0f;
float fBlueMult = 1.4f;
float fAlphaMult = 500.0f;
float fAlphaBase = 30.0f;
float fRandomMoveDiv = 8.0f;
float fRandomDamp = 0.99f;
float fNormMult = 2.0f;
float fNormMultB = 1.0f;
float fBumpScale = 1.5;
float fBumpTexRepeat = 2.0;
float fNormalDirectionScalar1 = 2.0f;
float fNormalDirectionScalar2 = 1.0f;
bool bTestDoNormals = true;
float fSeaBedZ = 25.0f;
float aAlphaFade[5] = { 0.4f, 1.0f, 0.2f, 1.0f, 0.4f };
float fFlatWaterBlendRange = 0.05f;
float fStartBlendDistanceAdd = 64.0f;
float fMinWaterAlphaMult = -30.0f;

void
CWaterLevel::Initialise(Const char *pWaterDat)
{
	ms_nNoOfWaterLevels = 0;

#ifdef MASTER
	int32 hFile = -1;
	do { hFile = CFileMgr::OpenFile("DATA\\waterpro.dat", "rb"); } while (hFile < 0);
#else
	int32 hFile = CFileMgr::OpenFile("DATA\\waterpro.dat", "rb");
#endif

	if (hFile > 0) {
		CFileMgr::Read(hFile, (char *)&ms_nNoOfWaterLevels, sizeof(ms_nNoOfWaterLevels));
		CFileMgr::Read(hFile, (char *)ms_aWaterZs, sizeof(ms_aWaterZs));
		CFileMgr::Read(hFile, (char *)ms_aWaterRects, sizeof(ms_aWaterRects));
		CFileMgr::Read(hFile, (char *)aWaterBlockList, sizeof(aWaterBlockList));
		CFileMgr::Read(hFile, (char *)aWaterFineBlockList, sizeof(aWaterFineBlockList));
		CFileMgr::CloseFile(hFile);
	}

	CTxdStore::PushCurrentTxd();
	int32 slot = CTxdStore::FindTxdSlot("particle");
	CTxdStore::SetCurrentTxd(slot);

	if (gpWaterTex == nil) gpWaterTex = RwTextureRead("waterclear256", nil);
	gpWaterRaster = RwTextureGetRaster(gpWaterTex);

	if (gpWaterEnvTex == nil) gpWaterEnvTex = RwTextureRead("waterreflection2", nil);
	gpWaterEnvRaster = RwTextureGetRaster(gpWaterEnvTex);

#ifdef PC_WATER
	if (gpWaterEnvBaseTex == nil) gpWaterEnvBaseTex = RwTextureRead("sandywater", nil);
	gpWaterEnvBaseRaster = RwTextureGetRaster(gpWaterEnvBaseTex);
#endif

	// Mantenemos la textura de wake para evitar crashes si otra parte la pide, aunque no la dibujemos
	if (gpWaterWakeTex == nil) gpWaterWakeTex = RwTextureRead("waterwake", nil);
	gpWaterWakeRaster = RwTextureGetRaster(gpWaterWakeTex);

	CTxdStore::PopCurrentTxd();

	// OPTIMIZACIÓN EXTREMA: Ya no creamos geometría de morphing (ondas animadas)
	ms_pWavyAtomic = nil;
	ms_pMaskAtomic = nil;
}

void
CWaterLevel::Shutdown()
{
	// OPTIMIZACIÓN EXTREMA: Ya no destruimos atómicos porque nunca los creamos
#define _DELETE_TEXTURE(t) if ( t ) { RwTextureDestroy(t); t = nil; }
	_DELETE_TEXTURE(gpWaterTex);
	_DELETE_TEXTURE(gpWaterEnvTex);
	_DELETE_TEXTURE(gpWaterWakeTex);
	_DELETE_TEXTURE(gpWaterEnvBaseTex);
#undef _DELETE_TEXTURE
}

// ZONA DE LOBOTOMÍA: Funciones de geometría dinámica vaciadas
void CWaterLevel::CreateWavyAtomic() {}
void CWaterLevel::DestroyWavyAtomic() {}

bool
CWaterLevel::GetWaterLevel(float fX, float fY, float fZ, float *pfOutLevel, bool bDontCheckZ)
{
	int32 x = WATER_TO_SMALL_SECTOR_X(fX + WATER_X_OFFSET);
	int32 y = WATER_TO_SMALL_SECTOR_Y(fY);

#ifdef FIX_BUGS
	if (x < 0 || x >= MAX_SMALL_SECTORS) return false;
	if (y < 0 || y >= MAX_SMALL_SECTORS) return false;
#endif

	int8 nBlock = aWaterFineBlockList[x][y];
	if (nBlock == NO_WATER) return false;

	ASSERT(pfOutLevel != nil);

	// OPTIMIZACIÓN FÍSICA EXTREMA: Anulamos los cálculos trigonométricos (Sin/Cos) de las olas.
	// El agua ahora es completamente plana a nivel físico. ¡Los botes y el jugador consumen 0 CPU aquí!
	*pfOutLevel = ms_aWaterZs[nBlock];

	if (bDontCheckZ == false && (*pfOutLevel - fZ) > 3.0f) {
		*pfOutLevel = 0.0f;
		return false;
	}
	return true;
}

bool
CWaterLevel::GetWaterLevelNoWaves(float fX, float fY, float fZ, float *pfOutLevel)
{
	int32 x = WATER_TO_SMALL_SECTOR_X(fX + WATER_X_OFFSET);
	int32 y = WATER_TO_SMALL_SECTOR_Y(fY);

#ifdef FIX_BUGS
	if (x < 0 || x >= MAX_SMALL_SECTORS) return false;
	if (y < 0 || y >= MAX_SMALL_SECTORS) return false;
#endif

	int8 nBlock = aWaterFineBlockList[x][y];
	if (nBlock == NO_WATER) return false;

	ASSERT(pfOutLevel != nil);
	*pfOutLevel = ms_aWaterZs[nBlock];
	return true;
}

float
CWaterLevel::GetWaterWavesOnly(short x, short y)
{
	// OPTIMIZACIÓN: Cero olas.
	return 0.0f;
}

CVector
CWaterLevel::GetWaterNormal(float fX, float fY)
{
	// OPTIMIZACIÓN: Al no haber olas, la normal del agua siempre mira hacia arriba.
	// Nos ahorramos el CrossProduct y la trigonometría.
	return CVector(0.0f, 0.0f, 1.0f);
}

inline float _GetWaterDrawDist() {
	if (TheCamera.GetPosition().z < 15.0f) return 1200.0f;
	if (TheCamera.GetPosition().z > 60.0f) return 2000.0f;
	return (TheCamera.GetPosition().z + -15.0f) * 800.0f / 45.0f + 1200.0f;
}

inline float _GetWavyDrawDist() {
	if (FindPlayerVehicle() && FindPlayerVehicle()->IsBoat()) return 120.0f;
	else return 70.0f;
}

inline void _GetCamBounds(bool *bUseCamStartY, bool *bUseCamEndY, bool *bUseCamStartX, bool *bUseCamEndX) {
	if (TheCamera.GetForward().z > -0.8f) {
		if (Abs(TheCamera.GetForward().x) > Abs(TheCamera.GetForward().y)) {
			if (TheCamera.GetForward().x > 0.0f) *bUseCamStartX = true;
			else *bUseCamEndX = true;
		}
		else {
			if (TheCamera.GetForward().y > 0.0f) *bUseCamStartY = true;
			else *bUseCamEndY = true;
		}
	}
}

inline bool _IsColideWithBlock(int32 x, int32 y, int32 &block) {
	block = CWaterLevel::aWaterFineBlockList[x + 0][y + 0];
	if (block >= 0) return true;
	block = CWaterLevel::aWaterFineBlockList[x + 0][y + 1];
	if (block >= 0) {
		block = CWaterLevel::aWaterFineBlockList[x + 0][y + 2];
		if (block >= 0) return true;
	}
	block = CWaterLevel::aWaterFineBlockList[x + 1][y + 0];
	if (block >= 0) return true;
	block = CWaterLevel::aWaterFineBlockList[x + 1][y + 1];
	if (block >= 0) {
		block = CWaterLevel::aWaterFineBlockList[x + 1][y + 2];
		if (block >= 0) return true;
	}
	block = CWaterLevel::aWaterFineBlockList[x + 2][y + 0];
	if (block >= 0) return true;
	block = CWaterLevel::aWaterFineBlockList[x + 2][y + 1];
	if (block >= 0) {
		block = CWaterLevel::aWaterFineBlockList[x + 2][y + 2];
		if (block >= 0) return true;
	}
	return false;
}

inline float SectorRadius(float fSize) { return Sqrt(Pow(fSize, 2) + Pow(fSize, 2)); }

void
CWaterLevel::RenderWater()
{
#ifndef MASTER
	if (gbDontRenderWater) return;
#endif
	bool bUseCamEndX = false;
	bool bUseCamStartY = false;
	bool bUseCamStartX = false;
	bool bUseCamEndY = false;

	if (!CGame::CanSeeWaterFromCurrArea()) return;

	_GetCamBounds(&bUseCamStartY, &bUseCamEndY, &bUseCamStartX, &bUseCamEndX);

	float fHugeSectorMaxRenderDist = _GetWaterDrawDist();
	float fHugeSectorMaxRenderDistSqr = SQR(fHugeSectorMaxRenderDist);

	float windAddUV = CWeather::WindClipped * 0.0005f + 0.0006f;
	float fAngle = (CTimer::GetTimeInMilliseconds() & 4095) * (TWOPI / 4096.0f);

	if (!CTimer::GetIsPaused()) {
		TEXTURE_ADDU += windAddUV;
		TEXTURE_ADDV += windAddUV;
	}

	if (TEXTURE_ADDU >= 1.0f) TEXTURE_ADDU = 0.0f;
	if (TEXTURE_ADDV >= 1.0f) TEXTURE_ADDV = 0.0f;

#ifdef PC_WATER
	_fWaterZOffset = CWeather::WindClipped * 0.5f + 0.25f;
#endif
	RwRGBA color = { 15, 60, 100, 255 };

#ifndef PC_WATER
	RwRGBA colorUnderwater = { 15, 60, 100, 255 };
#endif

	TempBufferVerticesStored = 0;
	TempBufferIndicesStored = 0;

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void *)gpWaterRaster);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void *)TRUE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDZERO);

	CVector2D camPos(TheCamera.GetPosition().x, TheCamera.GetPosition().y);

	int32 nStartX = WATER_TO_HUGE_SECTOR_X(camPos.x - fHugeSectorMaxRenderDist + WATER_X_OFFSET);
	int32 nEndX = WATER_TO_HUGE_SECTOR_X(camPos.x + fHugeSectorMaxRenderDist + WATER_X_OFFSET) + 1;
	int32 nStartY = WATER_TO_HUGE_SECTOR_Y(camPos.y - fHugeSectorMaxRenderDist);
	int32 nEndY = WATER_TO_HUGE_SECTOR_Y(camPos.y + fHugeSectorMaxRenderDist) + 1;

	if (bUseCamStartX) nStartX = WATER_TO_HUGE_SECTOR_X(camPos.x + WATER_X_OFFSET);
	if (bUseCamEndX)   nEndX = WATER_TO_HUGE_SECTOR_X(camPos.x + WATER_X_OFFSET);
	if (bUseCamStartY) nStartY = WATER_TO_HUGE_SECTOR_Y(camPos.y);
	if (bUseCamEndY)   nEndY = WATER_TO_HUGE_SECTOR_Y(camPos.y);

	nStartX = Clamp(nStartX, 0, MAX_HUGE_SECTORS - 1);
	nEndX = Clamp(nEndX, 0, MAX_HUGE_SECTORS - 1);
	nStartY = Clamp(nStartY, 0, MAX_HUGE_SECTORS - 1);
	nEndY = Clamp(nEndY, 0, MAX_HUGE_SECTORS - 1);

	for (int32 x = nStartX; x <= nEndX; x++) {
		for (int32 y = nStartY; y <= nEndY; y++) {
			if (aWaterBlockList[2 * x + 0][2 * y + 0] >= 0
				|| aWaterBlockList[2 * x + 1][2 * y + 0] >= 0
				|| aWaterBlockList[2 * x + 0][2 * y + 1] >= 0
				|| aWaterBlockList[2 * x + 1][2 * y + 1] >= 0)
			{
				float fX = WATER_FROM_HUGE_SECTOR_X(x) - WATER_X_OFFSET;
				float fY = WATER_FROM_HUGE_SECTOR_Y(y);
				CVector2D vecHugeSectorCentre(fX + HUGE_SECTOR_SIZE / 2, fY + HUGE_SECTOR_SIZE / 2);
				float fHugeSectorDistToCamSqr = (camPos - vecHugeSectorCentre).MagnitudeSqr();

				if (fHugeSectorMaxRenderDistSqr > fHugeSectorDistToCamSqr) {
					if (TheCamera.IsSphereVisible(CVector(vecHugeSectorCentre.x, vecHugeSectorCentre.y, 0.0f), SectorRadius(HUGE_SECTOR_SIZE))) {
						float fZ;
						if (aWaterBlockList[2 * x + 0][2 * y + 0] >= 0) fZ = ms_aWaterZs[aWaterBlockList[2 * x + 0][2 * y + 0]];
						if (aWaterBlockList[2 * x + 1][2 * y + 0] >= 0) fZ = ms_aWaterZs[aWaterBlockList[2 * x + 1][2 * y + 0]];
						if (aWaterBlockList[2 * x + 0][2 * y + 1] >= 0) fZ = ms_aWaterZs[aWaterBlockList[2 * x + 0][2 * y + 1]];
						if (aWaterBlockList[2 * x + 1][2 * y + 1] >= 0) fZ = ms_aWaterZs[aWaterBlockList[2 * x + 1][2 * y + 1]];

						if (fHugeSectorDistToCamSqr >= SQR(500.0f)) {
							RenderOneFlatHugeWaterPoly(fX, fY, fZ, color);
						}
						else {
#ifndef PC_WATER
							if (m_bRenderSeaBed) RenderOneSlopedUnderWaterPoly(fX, fY, fZ, colorUnderwater);
#endif
						}
					}
				}
			}
		}
	}

	for (int32 x = 0; x < 26; x++) {
		for (int32 y = 0; y < 5; y++) {
			float fX = WATER_SIGN_X(float(x) * EXTRAHUGE_SECTOR_SIZE) - 1280.0f - WATER_X_OFFSET;
			float fY = WATER_SIGN_Y(float(y) * EXTRAHUGE_SECTOR_SIZE) - 1280.0f;
			if (!bUseCamStartY) {
				CVector2D vecExtraHugeSectorCentre(fX + EXTRAHUGE_SECTOR_SIZE / 2, fY + EXTRAHUGE_SECTOR_SIZE / 2);
				float fCamDistToSector = (vecExtraHugeSectorCentre - camPos).Magnitude();
				if (fCamDistToSector < fHugeSectorMaxRenderDistSqr) {
					if (TheCamera.IsSphereVisible(CVector(vecExtraHugeSectorCentre.x, vecExtraHugeSectorCentre.y, 0.0f), SectorRadius(EXTRAHUGE_SECTOR_SIZE))) {
						RenderOneFlatExtraHugeWaterPoly(vecExtraHugeSectorCentre.x - EXTRAHUGE_SECTOR_SIZE / 2, vecExtraHugeSectorCentre.y - EXTRAHUGE_SECTOR_SIZE / 2, 0.0f, color);
					}
				}
			}
			if (!bUseCamEndY) {
				CVector2D vecExtraHugeSectorCentre(fX + EXTRAHUGE_SECTOR_SIZE / 2, -(fY + EXTRAHUGE_SECTOR_SIZE / 2));
				float fCamDistToSector = (vecExtraHugeSectorCentre - camPos).Magnitude();
				if (fCamDistToSector < fHugeSectorMaxRenderDistSqr) {
					if (TheCamera.IsSphereVisible(CVector(vecExtraHugeSectorCentre.x, vecExtraHugeSectorCentre.y, 0.0f), SectorRadius(EXTRAHUGE_SECTOR_SIZE))) {
						RenderOneFlatExtraHugeWaterPoly(vecExtraHugeSectorCentre.x - EXTRAHUGE_SECTOR_SIZE / 2, vecExtraHugeSectorCentre.y - EXTRAHUGE_SECTOR_SIZE / 2, 0.0f, color);
					}
				}
			}
		}
	}

	for (int32 y = 5; y < 21; y++) {
		for (int32 x = 0; x < 5; x++) {
			float fX = WATER_SIGN_X(float(x) * EXTRAHUGE_SECTOR_SIZE) - 1280.0f - WATER_X_OFFSET;
			float fX2 = WATER_SIGN_X(float(x) * EXTRAHUGE_SECTOR_SIZE) - 1280.0f + WATER_X_OFFSET;
			float fY = WATER_SIGN_Y(float(y) * EXTRAHUGE_SECTOR_SIZE) - 1280.0f;
			if (!bUseCamStartX) {
				CVector2D vecExtraHugeSectorCentre(fX + EXTRAHUGE_SECTOR_SIZE / 2, fY + EXTRAHUGE_SECTOR_SIZE / 2);
				float fCamDistToSector = (vecExtraHugeSectorCentre - camPos).Magnitude();
				if (fCamDistToSector < fHugeSectorMaxRenderDistSqr) {
					if (TheCamera.IsSphereVisible(CVector(vecExtraHugeSectorCentre.x, vecExtraHugeSectorCentre.y, 0.0f), SectorRadius(EXTRAHUGE_SECTOR_SIZE))) {
						RenderOneFlatExtraHugeWaterPoly(vecExtraHugeSectorCentre.x - EXTRAHUGE_SECTOR_SIZE / 2, vecExtraHugeSectorCentre.y - EXTRAHUGE_SECTOR_SIZE / 2, 0.0f, color);
					}
				}
			}
			if (!bUseCamEndX) {
				CVector2D vecExtraHugeSectorCentre(-(fX2 + EXTRAHUGE_SECTOR_SIZE / 2), fY + EXTRAHUGE_SECTOR_SIZE / 2);
				float fCamDistToSector = (vecExtraHugeSectorCentre - camPos).Magnitude();
				if (fCamDistToSector < fHugeSectorMaxRenderDistSqr) {
					if (TheCamera.IsSphereVisible(CVector(vecExtraHugeSectorCentre.x, vecExtraHugeSectorCentre.x, 0.0f), SectorRadius(EXTRAHUGE_SECTOR_SIZE))) {
						RenderOneFlatExtraHugeWaterPoly(vecExtraHugeSectorCentre.x - EXTRAHUGE_SECTOR_SIZE / 2, vecExtraHugeSectorCentre.y - EXTRAHUGE_SECTOR_SIZE / 2, 0.0f, color);
					}
				}
			}
		}
	}

	RenderAndEmptyRenderBuffer();

	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA);

	// OPTIMIZACIÓN EXTREMA: Deshabilitamos el renderizado de pájaros, barcos fantasma a lo lejos y juguetes de playa

	DefinedState();
}


void
CWaterLevel::RenderTransparentWater(void)
{
	bool bUseCamEndX = false;
	bool bUseCamStartY = false;
	bool bUseCamStartX = false;
	bool bUseCamEndY = false;

	_bSeaLife = false;
	if (!CGame::CanSeeWaterFromCurrArea()) return;

	PUSH_RENDERGROUP("CWaterLevel::RenderTransparentWater");

	float fWaterDrawDist = _GetWavyDrawDist();
	float fWaterDrawDistLarge = fWaterDrawDist + 90.0f;
	float fWavySectorMaxRenderDistSqr = SQR(fWaterDrawDist);

	_GetCamBounds(&bUseCamStartY, &bUseCamEndY, &bUseCamStartX, &bUseCamEndX);

	float fHugeSectorMaxRenderDist = _GetWaterDrawDist();
	float fHugeSectorMaxRenderDistSqr = SQR(fHugeSectorMaxRenderDist);

	RwRGBA color = { 15, 60, 100, 255 };

	RwRGBA colorTrans = { 15, 60, 100, 255 };

	TempBufferVerticesStored = 0;
	TempBufferIndicesStored = 0;

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void *)gpWaterRaster);

	// --- POTATO EDITION: Destruimos el Alpha Blending a nivel de hardware ---
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDZERO);

#ifndef PC_WATER
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void *)TRUE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA);
#endif

	CVector2D camPos(TheCamera.GetPosition().x, TheCamera.GetPosition().y);

	int32 nStartX = WATER_TO_HUGE_SECTOR_X(camPos.x - fHugeSectorMaxRenderDist + WATER_X_OFFSET);
	int32 nEndX = WATER_TO_HUGE_SECTOR_X(camPos.x + fHugeSectorMaxRenderDist + WATER_X_OFFSET) + 1;
	int32 nStartY = WATER_TO_HUGE_SECTOR_Y(camPos.y - fHugeSectorMaxRenderDist);
	int32 nEndY = WATER_TO_HUGE_SECTOR_Y(camPos.y + fHugeSectorMaxRenderDist) + 1;

	if (bUseCamStartX) nStartX = WATER_TO_HUGE_SECTOR_X(camPos.x + WATER_X_OFFSET);
	if (bUseCamEndX)   nEndX = WATER_TO_HUGE_SECTOR_X(camPos.x + WATER_X_OFFSET);
	if (bUseCamStartY) nStartY = WATER_TO_HUGE_SECTOR_Y(camPos.y);
	if (bUseCamEndY)   nEndY = WATER_TO_HUGE_SECTOR_Y(camPos.y);

	nStartX = Clamp(nStartX, 0, MAX_HUGE_SECTORS - 1);
	nEndX = Clamp(nEndX, 0, MAX_HUGE_SECTORS - 1);
	nStartY = Clamp(nStartY, 0, MAX_HUGE_SECTORS - 1);
	nEndY = Clamp(nEndY, 0, MAX_HUGE_SECTORS - 1);

	for (int32 x = nStartX; x <= nEndX; x++) {
		for (int32 y = nStartY; y <= nEndY; y++) {
			if (aWaterBlockList[2 * x + 0][2 * y + 0] >= 0
				|| aWaterBlockList[2 * x + 1][2 * y + 0] >= 0
				|| aWaterBlockList[2 * x + 0][2 * y + 1] >= 0
				|| aWaterBlockList[2 * x + 1][2 * y + 1] >= 0)
			{
				float fX = WATER_FROM_HUGE_SECTOR_X(x) - WATER_X_OFFSET;
				float fY = WATER_FROM_HUGE_SECTOR_Y(y);
				CVector2D vecHugeSectorCentre(fX + HUGE_SECTOR_SIZE / 2, fY + HUGE_SECTOR_SIZE / 2);
				float fHugeSectorDistToCamSqr = (camPos - vecHugeSectorCentre).MagnitudeSqr();

				if (fHugeSectorMaxRenderDistSqr > fHugeSectorDistToCamSqr) {
					if (TheCamera.IsSphereVisible(CVector(vecHugeSectorCentre.x, vecHugeSectorCentre.y, 0.0f), SectorRadius(HUGE_SECTOR_SIZE))) {
						if (fHugeSectorDistToCamSqr < SQR(500.0f)) {
							for (int32 x2 = 2 * x; x2 <= 2 * x + 1; x2++) {
								for (int32 y2 = 2 * y; y2 <= 2 * y + 1; y2++) {
									if (aWaterBlockList[x2][y2] >= 0) {
										float fLargeX = WATER_FROM_LARGE_SECTOR_X(x2) - WATER_X_OFFSET;
										float fLargeY = WATER_FROM_LARGE_SECTOR_Y(y2);
										CVector2D vecLargeSectorCentre(fLargeX + LARGE_SECTOR_SIZE / 2, fLargeY + LARGE_SECTOR_SIZE / 2);
										float fLargeSectorDistToCamSqr = (camPos - vecLargeSectorCentre).MagnitudeSqr();

										if (fLargeSectorDistToCamSqr < fHugeSectorMaxRenderDistSqr) {
											if (TheCamera.IsSphereVisible(CVector(vecLargeSectorCentre.x, vecLargeSectorCentre.y, 0.0f), SectorRadius(LARGE_SECTOR_SIZE))) {
												float fLargeSectorDrawDistSqr = SQR((fWaterDrawDistLarge + 16.0f));
												if (fLargeSectorDistToCamSqr < fLargeSectorDrawDistSqr) {
													float fZ;
													// WS
													if (aWaterFineBlockList[2 * x2 + 0][2 * y2 + 0] >= 0) {
														float fSmallX = fLargeX; float fSmallY = fLargeY;
														CVector2D vecSmallSectorCentre(fSmallX + SMALL_SECTOR_SIZE / 2, fSmallY + SMALL_SECTOR_SIZE / 2);
														float fSmallSectorDistToCamSqr = (camPos - vecSmallSectorCentre).MagnitudeSqr();
														fZ = ms_aWaterZs[aWaterFineBlockList[2 * x2 + 0][2 * y2 + 0]];
														if (fSmallSectorDistToCamSqr < fWavySectorMaxRenderDistSqr)
															RenderOneWavySector(fSmallX, fSmallY, fZ, colorTrans);
														else RenderOneFlatSmallWaterPolyBlended(fSmallX, fSmallY, fZ, camPos.x, camPos.y, color, colorTrans, fWaterDrawDist);
													}
													// SE
													if (aWaterFineBlockList[2 * x2 + 1][2 * y2 + 0] >= 0) {
														float fSmallX = fLargeX + (LARGE_SECTOR_SIZE / 2); float fSmallY = fLargeY;
														CVector2D vecSmallSectorCentre(fSmallX + SMALL_SECTOR_SIZE / 2, fSmallY + SMALL_SECTOR_SIZE / 2);
														float fSmallSectorDistToCamSqr = (camPos - vecSmallSectorCentre).MagnitudeSqr();
														fZ = ms_aWaterZs[aWaterFineBlockList[2 * x2 + 1][2 * y2 + 0]];
														if (fSmallSectorDistToCamSqr < fWavySectorMaxRenderDistSqr)
															RenderOneWavySector(fSmallX, fSmallY, fZ, colorTrans);
														else RenderOneFlatSmallWaterPolyBlended(fSmallX, fSmallY, fZ, camPos.x, camPos.y, color, colorTrans, fWaterDrawDist);
													}
													// WN
													if (aWaterFineBlockList[2 * x2 + 0][2 * y2 + 1] >= 0) {
														float fSmallX = fLargeX; float fSmallY = fLargeY + (LARGE_SECTOR_SIZE / 2);
														CVector2D vecSmallSectorCentre(fSmallX + SMALL_SECTOR_SIZE / 2, fSmallY + SMALL_SECTOR_SIZE / 2);
														float fSmallSectorDistToCamSqr = (camPos - vecSmallSectorCentre).MagnitudeSqr();
														fZ = ms_aWaterZs[aWaterFineBlockList[2 * x2 + 0][2 * y2 + 1]];
														if (fSmallSectorDistToCamSqr < fWavySectorMaxRenderDistSqr)
															RenderOneWavySector(fSmallX, fSmallY, fZ, colorTrans);
														else RenderOneFlatSmallWaterPolyBlended(fSmallX, fSmallY, fZ, camPos.x, camPos.y, color, colorTrans, fWaterDrawDist);
													}
													//NE
													if (aWaterFineBlockList[2 * x2 + 1][2 * y2 + 1] >= 0) {
														float fSmallX = fLargeX + (LARGE_SECTOR_SIZE / 2); float fSmallY = fLargeY + (LARGE_SECTOR_SIZE / 2);
														CVector2D vecSmallSectorCentre(fSmallX + SMALL_SECTOR_SIZE / 2, fSmallY + SMALL_SECTOR_SIZE / 2);
														float fSmallSectorDistToCamSqr = (camPos - vecSmallSectorCentre).MagnitudeSqr();
														fZ = ms_aWaterZs[aWaterFineBlockList[2 * x2 + 1][2 * y2 + 1]];
														if (fSmallSectorDistToCamSqr < fWavySectorMaxRenderDistSqr)
															RenderOneWavySector(fSmallX, fSmallY, fZ, colorTrans);
														else RenderOneFlatSmallWaterPolyBlended(fSmallX, fSmallY, fZ, camPos.x, camPos.y, color, colorTrans, fWaterDrawDist);
													}
												}
												else {
													float fZ = ms_aWaterZs[aWaterBlockList[x2][y2]];
													RenderOneFlatLargeWaterPoly(fLargeX, fLargeY, fZ, color);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	RenderAndEmptyRenderBuffer();
	DefinedState();
	POP_RENDERGROUP();
}

void CWaterLevel::RenderOneFlatSmallWaterPoly(float fX, float fY, float fZ, RwRGBA const &color)
{
	if (TempBufferIndicesStored >= TEMPBUFFERINDEXSIZE - 6 || TempBufferVerticesStored >= TEMPBUFFERVERTSIZE - 4)
		RenderAndEmptyRenderBuffer();

	int32 vidx = TempBufferVerticesStored;

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 0], fX, fY, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 0], TEXTURE_ADDU);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 0], TEXTURE_ADDV);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 0], color);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 1], fX, fY + SMALL_SECTOR_SIZE, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 1], TEXTURE_ADDU);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 1], TEXTURE_ADDV + 1.0f);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 1], color);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 2], fX + SMALL_SECTOR_SIZE, fY + SMALL_SECTOR_SIZE, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 2], TEXTURE_ADDU + 1.0f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 2], TEXTURE_ADDV + 1.0f);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 2], color);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 3], fX + SMALL_SECTOR_SIZE, fY, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 3], TEXTURE_ADDU + 1.0f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 3], TEXTURE_ADDV);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 3], color);

	int32 iidx = TempBufferIndicesStored;
	TempBufferRenderIndexList[iidx + 0] = TempBufferVerticesStored + 0;
	TempBufferRenderIndexList[iidx + 1] = TempBufferVerticesStored + 2;
	TempBufferRenderIndexList[iidx + 2] = TempBufferVerticesStored + 1;
	TempBufferRenderIndexList[iidx + 3] = TempBufferVerticesStored + 0;
	TempBufferRenderIndexList[iidx + 4] = TempBufferVerticesStored + 3;
	TempBufferRenderIndexList[iidx + 5] = TempBufferVerticesStored + 2;

	TempBufferVerticesStored += 4;
	TempBufferIndicesStored += 6;
}

void
CWaterLevel::RenderOneFlatLargeWaterPoly(float fX, float fY, float fZ, RwRGBA const &color)
{
	if (TempBufferIndicesStored >= TEMPBUFFERINDEXSIZE - 6 || TempBufferVerticesStored >= TEMPBUFFERVERTSIZE - 4)
		RenderAndEmptyRenderBuffer();

	int32 vidx = TempBufferVerticesStored;

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 0], fX, fY, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 0], TEXTURE_ADDU);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 0], TEXTURE_ADDV);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 0], color);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 1], fX, fY + LARGE_SECTOR_SIZE, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 1], TEXTURE_ADDU);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 1], TEXTURE_ADDV + 2.0f);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 1], color);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 2], fX + LARGE_SECTOR_SIZE, fY + LARGE_SECTOR_SIZE, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 2], TEXTURE_ADDU + 2.0f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 2], TEXTURE_ADDV + 2.0f);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 2], color);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 3], fX + LARGE_SECTOR_SIZE, fY, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 3], TEXTURE_ADDU + 2.0f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 3], TEXTURE_ADDV);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 3], color);

	int32 iidx = TempBufferIndicesStored;
	TempBufferRenderIndexList[iidx + 0] = TempBufferVerticesStored + 0;
	TempBufferRenderIndexList[iidx + 1] = TempBufferVerticesStored + 2;
	TempBufferRenderIndexList[iidx + 2] = TempBufferVerticesStored + 1;
	TempBufferRenderIndexList[iidx + 3] = TempBufferVerticesStored + 0;
	TempBufferRenderIndexList[iidx + 4] = TempBufferVerticesStored + 3;
	TempBufferRenderIndexList[iidx + 5] = TempBufferVerticesStored + 2;

	TempBufferVerticesStored += 4;
	TempBufferIndicesStored += 6;
}

void
CWaterLevel::RenderOneFlatHugeWaterPoly(float fX, float fY, float fZ, RwRGBA const &color)
{
	if (TempBufferIndicesStored >= TEMPBUFFERINDEXSIZE - 6 || TempBufferVerticesStored >= TEMPBUFFERVERTSIZE - 4)
		RenderAndEmptyRenderBuffer();

	int32 vidx = TempBufferVerticesStored;
	RwRGBA c;
	c.red = color.red; c.green = color.green; c.blue = color.blue; c.alpha = 255;

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 0], fX, fY, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 0], TEXTURE_ADDU);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 0], TEXTURE_ADDV);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 0], c);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 1], fX, fY + HUGE_SECTOR_SIZE, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 1], TEXTURE_ADDU);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 1], TEXTURE_ADDV + 4.0f);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 1], c);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 2], fX + HUGE_SECTOR_SIZE, fY + HUGE_SECTOR_SIZE, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 2], TEXTURE_ADDU + 4.0f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 2], TEXTURE_ADDV + 4.0f);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 2], c);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 3], fX + HUGE_SECTOR_SIZE, fY, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 3], TEXTURE_ADDU + 4.0f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 3], TEXTURE_ADDV);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 3], c);

	int32 iidx = TempBufferIndicesStored;
	TempBufferRenderIndexList[iidx + 0] = TempBufferVerticesStored + 0;
	TempBufferRenderIndexList[iidx + 1] = TempBufferVerticesStored + 2;
	TempBufferRenderIndexList[iidx + 2] = TempBufferVerticesStored + 1;
	TempBufferRenderIndexList[iidx + 3] = TempBufferVerticesStored + 0;
	TempBufferRenderIndexList[iidx + 4] = TempBufferVerticesStored + 3;
	TempBufferRenderIndexList[iidx + 5] = TempBufferVerticesStored + 2;

	TempBufferVerticesStored += 4;
	TempBufferIndicesStored += 6;
}

void
CWaterLevel::RenderOneFlatExtraHugeWaterPoly(float fX, float fY, float fZ, RwRGBA const &color)
{
	if (TempBufferIndicesStored >= TEMPBUFFERINDEXSIZE - 6 || TempBufferVerticesStored >= TEMPBUFFERVERTSIZE - 4)
		RenderAndEmptyRenderBuffer();

	int32 vidx = TempBufferVerticesStored;
	RwRGBA c;
	c.red = color.red; c.green = color.green; c.blue = color.blue; c.alpha = 255;

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 0], fX, fY, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 0], TEXTURE_ADDU);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 0], TEXTURE_ADDV);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 0], c);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 1], fX, fY + EXTRAHUGE_SECTOR_SIZE, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 1], TEXTURE_ADDU);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 1], TEXTURE_ADDV + 8.0f);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 1], c);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 2], fX + EXTRAHUGE_SECTOR_SIZE, fY + EXTRAHUGE_SECTOR_SIZE, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 2], TEXTURE_ADDU + 8.0f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 2], TEXTURE_ADDV + 8.0f);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 2], c);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 3], fX + EXTRAHUGE_SECTOR_SIZE, fY, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 3], TEXTURE_ADDU + 8.0f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 3], TEXTURE_ADDV);
	RwIm3DVertexSet_RGBA(&TempBufferRenderVertices[vidx + 3], c);

	int32 iidx = TempBufferIndicesStored;
	TempBufferRenderIndexList[iidx + 0] = TempBufferVerticesStored + 0;
	TempBufferRenderIndexList[iidx + 1] = TempBufferVerticesStored + 2;
	TempBufferRenderIndexList[iidx + 2] = TempBufferVerticesStored + 1;
	TempBufferRenderIndexList[iidx + 3] = TempBufferVerticesStored + 0;
	TempBufferRenderIndexList[iidx + 4] = TempBufferVerticesStored + 3;
	TempBufferRenderIndexList[iidx + 5] = TempBufferVerticesStored + 2;

	TempBufferVerticesStored += 4;
	TempBufferIndicesStored += 6;
}

void
CWaterLevel::RenderOneWavySector(float fX, float fY, float fZ, RwRGBA const &color, bool bDontRender)
{
	// OPTIMIZACIÓN EXTREMA: 
	// En lugar de renderizar la malla ondulada por la CPU (Morph Target)
	// Forzamos el renderizado de un polígono 100% plano (0 coste de CPU y 4 vértices en lugar de 289 vértices)
	if (!bDontRender) {
		RenderOneFlatSmallWaterPoly(fX, fY, fZ, color);
	}
}

void
CWaterLevel::RenderWavyMask(float fX, float fY, float fZ, float fSectorX, float fSectorY,
#ifdef PC_WATER
	float fCamPosX, float fCamPosY, float fCamDirX, float fCamDirY, RwRGBA const&color)
#else
int32 nCamDirX, int32 nCamDirY, RwRGBA const&color)
#endif
{
	// Vaciado Extremo: No más máscaras reflectantes
}

#ifdef PC_WATER
void CWaterLevel::PreCalcWaterGeometry(void) {}
bool CWaterLevel::PreCalcWavySector(RwRGBA const &color) { return true; }
bool CWaterLevel::PreCalcWavyMask(float fX, float fY, float fZ, float fSectorX, float fSectorY, float fCamPosX, float fCamPosY, float fCamDirX, float fCamDirY, RwRGBA const&color) { return false; }
#endif

// LOBOTOMÍA A LAS ESTELAS Y PARTÍCULAS
void CWaterLevel::RenderBoatWakes(void) {}
void CWaterLevel::RenderWakeSegment(CVector2D &vecA, CVector2D &vecB, CVector2D &vecC, CVector2D &vecD, float &fSizeA, float &fSizeB, float &fAlphaA, float &fAlphaB, float &fWakeZ) {}

void
CWaterLevel::RenderOneSlopedUnderWaterPoly(float fX, float fY, float fZ, RwRGBA const&color)
{
	CVector2D camPos(TheCamera.GetPosition().x, TheCamera.GetPosition().y);

	float fDistA = (CVector2D(fX, fY) - camPos).Magnitude() + -140.0f;
	float fDistB = (CVector2D(fX, fY + HUGE_SECTOR_SIZE) - camPos).Magnitude() + -140.0f;
	float fDistC = (CVector2D(fX + HUGE_SECTOR_SIZE, fY + HUGE_SECTOR_SIZE) - camPos).Magnitude() + -140.0f;
	float fDistD = (CVector2D(fX + HUGE_SECTOR_SIZE, fY) - camPos).Magnitude() + -140.0f;

	float fSeaBedA, fSeaBedB, fSeaBedC, fSeaBedD;

#define CALCSEABED(v, d) { v = 0.1f; if ( d < 0.0f ) v += fSeaBedZ; else if ( d <= 240.0f ) v += (fSeaBedZ / 240.0f) * (240.0f - d); }
	CALCSEABED(fSeaBedA, fDistA);
	CALCSEABED(fSeaBedB, fDistB);
	CALCSEABED(fSeaBedC, fDistC);
	CALCSEABED(fSeaBedD, fDistD);
#undef CALCSEABED

	if (TempBufferIndicesStored >= TEMPBUFFERINDEXSIZE - 6 || TempBufferVerticesStored >= TEMPBUFFERVERTSIZE - 4)
		RenderAndEmptyRenderBuffer();

	int32 vidx = TempBufferVerticesStored;

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 0], fX, fY, fZ - _fWaterZOffset - fSeaBedA);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 0], 0.0f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 0], 0.0f);
	RwIm3DVertexSetRGBA(&TempBufferRenderVertices[vidx + 0], color.red, color.green, color.blue, 255);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 1], fX, fY + HUGE_SECTOR_SIZE, fZ - _fWaterZOffset - fSeaBedB);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 1], 0.0f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 1], 4.0f);
	RwIm3DVertexSetRGBA(&TempBufferRenderVertices[vidx + 1], color.red, color.green, color.blue, 255);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 2], fX + HUGE_SECTOR_SIZE, fY + HUGE_SECTOR_SIZE, fZ - _fWaterZOffset - fSeaBedC);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 2], 4.0f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 2], 4.0f);
	RwIm3DVertexSetRGBA(&TempBufferRenderVertices[vidx + 2], color.red, color.green, color.blue, 255);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 3], fX + HUGE_SECTOR_SIZE, fY, fZ - _fWaterZOffset - fSeaBedD);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 3], 4.0f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 3], 0.0f);
	RwIm3DVertexSetRGBA(&TempBufferRenderVertices[vidx + 3], color.red, color.green, color.blue, 255);

	int32 iidx = TempBufferIndicesStored;
	TempBufferRenderIndexList[iidx + 0] = TempBufferVerticesStored + 0;
	TempBufferRenderIndexList[iidx + 1] = TempBufferVerticesStored + 2;
	TempBufferRenderIndexList[iidx + 2] = TempBufferVerticesStored + 1;
	TempBufferRenderIndexList[iidx + 3] = TempBufferVerticesStored + 0;
	TempBufferRenderIndexList[iidx + 4] = TempBufferVerticesStored + 3;
	TempBufferRenderIndexList[iidx + 5] = TempBufferVerticesStored + 2;

	TempBufferVerticesStored += 4;
	TempBufferIndicesStored += 6;
}

void
CWaterLevel::RenderOneFlatSmallWaterPolyBlended(float fX, float fY, float fZ, float fCamX, float fCamY,
	RwRGBA const &color, RwRGBA const &colorTrans, float fDrawDist)
{
	if (TempBufferIndicesStored >= TEMPBUFFERINDEXSIZE - 6 || TempBufferVerticesStored >= TEMPBUFFERVERTSIZE - 4)
		RenderAndEmptyRenderBuffer();

	int32 vidx = TempBufferVerticesStored;

	float fBlendDrawDist = fDrawDist + fStartBlendDistanceAdd;
	float fDistStartX = SQR(fX - fCamX);
	float fDistStartY = SQR(fY - fCamY);
	float fDistEndX = SQR((fX + SMALL_SECTOR_SIZE) - fCamX);
	float fDistEndY = SQR((fY + SMALL_SECTOR_SIZE) - fCamY);

	float fAlphaBlendMulA = Min(fFlatWaterBlendRange * Max(sqrt(fDistStartX + fDistStartY) - fBlendDrawDist, fMinWaterAlphaMult), 1.0f);
	float fAlphaBlendMulB = Min(fFlatWaterBlendRange * Max(sqrt(fDistStartX + fDistEndY) - fBlendDrawDist, fMinWaterAlphaMult), 1.0f);
	float fAlphaBlendMulC = Min(fFlatWaterBlendRange * Max(sqrt(fDistEndX + fDistEndY) - fBlendDrawDist, fMinWaterAlphaMult), 1.0f);
	float fAlphaBlendMulD = Min(fFlatWaterBlendRange * Max(sqrt(fDistEndX + fDistStartY) - fBlendDrawDist, fMinWaterAlphaMult), 1.0f);

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 0], fX, fY, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 0], TEXTURE_ADDU);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 0], TEXTURE_ADDV);
	RwIm3DVertexSetRGBA(&TempBufferRenderVertices[vidx + 0], color.red, color.green, color.blue, (colorTrans.alpha + (color.alpha - colorTrans.alpha) * (uint8)(int32)fAlphaBlendMulA));

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 1], fX, fY + SMALL_SECTOR_SIZE, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 1], TEXTURE_ADDU);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 1], TEXTURE_ADDV + 1.0f);
	RwIm3DVertexSetRGBA(&TempBufferRenderVertices[vidx + 1], color.red, color.green, color.blue, (colorTrans.alpha + (color.alpha - colorTrans.alpha) * (uint8)(int32)fAlphaBlendMulB));

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 2], fX + SMALL_SECTOR_SIZE, fY + SMALL_SECTOR_SIZE, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 2], TEXTURE_ADDU + 1.0f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 2], TEXTURE_ADDV + 1.0f);
	RwIm3DVertexSetRGBA(&TempBufferRenderVertices[vidx + 2], color.red, color.green, color.blue, (colorTrans.alpha + (color.alpha - colorTrans.alpha) * (uint8)(int32)fAlphaBlendMulC));

	RwIm3DVertexSetPos(&TempBufferRenderVertices[vidx + 3], fX + SMALL_SECTOR_SIZE, fY, fZ - _fWaterZOffset);
	RwIm3DVertexSetU(&TempBufferRenderVertices[vidx + 3], TEXTURE_ADDU + 1.0f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[vidx + 3], TEXTURE_ADDV);
	RwIm3DVertexSetRGBA(&TempBufferRenderVertices[vidx + 3], color.red, color.green, color.blue, (colorTrans.alpha + (color.alpha - colorTrans.alpha) * (uint8)(int32)fAlphaBlendMulD));

	int32 iidx = TempBufferIndicesStored;
	TempBufferRenderIndexList[iidx + 0] = TempBufferVerticesStored + 0;
	TempBufferRenderIndexList[iidx + 1] = TempBufferVerticesStored + 2;
	TempBufferRenderIndexList[iidx + 2] = TempBufferVerticesStored + 1;
	TempBufferRenderIndexList[iidx + 3] = TempBufferVerticesStored + 0;
	TempBufferRenderIndexList[iidx + 4] = TempBufferVerticesStored + 3;
	TempBufferRenderIndexList[iidx + 5] = TempBufferVerticesStored + 2;

	TempBufferVerticesStored += 4;
	TempBufferIndicesStored += 6;
}

float
CWaterLevel::CalcDistanceToWater(float fX, float fY)
{
	const float fSectorMaxRenderDist = 250.0f;

	int32 nStartX = WATER_TO_SMALL_SECTOR_X(fX - fSectorMaxRenderDist + WATER_X_OFFSET) - 1;
	int32 nEndX = WATER_TO_SMALL_SECTOR_X(fX + fSectorMaxRenderDist + WATER_X_OFFSET) + 1;
	int32 nStartY = WATER_TO_SMALL_SECTOR_Y(fY - fSectorMaxRenderDist) - 1;
	int32 nEndY = WATER_TO_SMALL_SECTOR_Y(fY + fSectorMaxRenderDist) + 1;

	nStartX = Clamp(nStartX, 0, MAX_SMALL_SECTORS - 1);
	nEndX = Clamp(nEndX, 0, MAX_SMALL_SECTORS - 1);
	nStartY = Clamp(nStartY, 0, MAX_SMALL_SECTORS - 1);
	nEndY = Clamp(nEndY, 0, MAX_SMALL_SECTORS - 1);

	float fDistSqr = 1.0e10f;

	for (int32 x = nStartX; x <= nEndX; x++) {
		for (int32 y = nStartY; y <= nEndY; y++) {
			if (aWaterFineBlockList[x][y] >= 0) {
				float fSectorX = WATER_FROM_SMALL_SECTOR_X(x) - WATER_X_OFFSET;
				float fSectorY = WATER_FROM_SMALL_SECTOR_Y(y);
				CVector2D vecDist(fSectorX + SMALL_SECTOR_SIZE - fX, fSectorY + SMALL_SECTOR_SIZE - fY);
				fDistSqr = Min(vecDist.MagnitudeSqr(), fDistSqr);
			}
		}
	}
	return Clamp(Sqrt(fDistSqr) - 23.0f, 0.0f, fSectorMaxRenderDist);
}

void
CWaterLevel::RenderAndEmptyRenderBuffer()
{
	if (TempBufferVerticesStored) {
		LittleTest();
		if (RwIm3DTransform(TempBufferRenderVertices, TempBufferVerticesStored, nil, rwIM3D_VERTEXUV)) {
			RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, TempBufferRenderIndexList, TempBufferIndicesStored);
			RwIm3DEnd();
		}
	}
	TempBufferIndicesStored = 0;
	TempBufferVerticesStored = 0;
}

bool
CWaterLevel::GetGroundLevel(CVector const &vecPosn, float *pfOutLevel, ColData *pData, float fDistance)
{
	CColPoint point;
	CEntity *entity;
	if (!CWorld::ProcessVerticalLine(vecPosn + CVector(0.0f, 0.0f, fDistance), -fDistance, point, entity, true, false, false, false, true, false, nil))
		return false;

	*pfOutLevel = point.point.z;
	if (pData != nil) {
		pData->SurfaceType = point.surfaceB;
		pData->PieceType = point.pieceB;
	}
	return true;
}

bool
CWaterLevel::IsLocationOutOfWorldBounds_WS(CVector const &vecPosn, int nOffset)
{
	int32 x = int32((vecPosn.x / 50.0f) + 48.0f);
	int32 y = int32((vecPosn.y / 50.0f) + 40.0f);
	return x < nOffset || x >= 80 - nOffset || y < nOffset || y >= 80 - nOffset;
}

bool
CWaterLevel::GetGroundLevel_WS(CVector const &vecPosn, float *pfOutLevel, ColData *pData, float fDistance)
{
	if (IsLocationOutOfWorldBounds_WS(vecPosn, 0)) return false;
	else return GetGroundLevel(vecPosn, pfOutLevel, pData, fDistance);
}

bool
CWaterLevel::GetWaterDepth(CVector const &vecPosn, float *pfDepth, float *pfLevelNoWaves, float *pfGroundLevel)
{
	float fLevelNoWaves;
	float fGroundLevel;
	if (!GetWaterLevelNoWaves(vecPosn.x, vecPosn.y, vecPosn.z, &fLevelNoWaves)) return false;
	if (!GetGroundLevel(vecPosn, &fGroundLevel, nil, 30.0f)) fGroundLevel = -100.0;

	if (pfDepth != nil) *pfDepth = fLevelNoWaves - fGroundLevel;
	if (pfLevelNoWaves != nil) *pfLevelNoWaves = fLevelNoWaves;
	if (pfGroundLevel != nil) *pfGroundLevel = fGroundLevel;

	return true;
}

// ZONA DE LOBOTOMÍA: No más distracciones inútiles
void CWaterLevel::RenderSeaBirds() {}
void CWaterLevel::RenderShipsOnHorizon() {}
void CWaterLevel::HandleSeaLifeForms() {}
void CWaterLevel::HandleBeachToysStuff(void) {}
CEntity * CWaterLevel::CreateBeachToy(CVector const &vec, eBeachToy beachtoy) { return nil; }

