#include "common.h"
#include "SpecialFX.h"
#include "RenderBuffer.h"
#include "Timer.h"
#include "Sprite.h"
#include "Font.h"
#include "Text.h"
#include "TxdStore.h"
#include "FileMgr.h"
#include "FileLoader.h"
#include "Timecycle.h"
#include "Lights.h"
#include "ModelIndices.h"
#include "VisibilityPlugins.h"
#include "World.h"
#include "PlayerPed.h"
#include "Particle.h"
#include "Shadows.h"
#include "General.h"
#include "Camera.h"
#include "Shadows.h"
#include "main.h"
#include "ColStore.h"
#include "Coronas.h"
#include "Script.h"
#include "DMAudio.h"

// Variables mantenidas para el Linker
RwIm3DVertex StreakVertices[4];
RwImVertexIndex StreakIndexList[12];
RwIm3DVertex TraceVertices[10];

bool CSpecialFX::bVideoCam;
bool CSpecialFX::bLiftCam;
bool CSpecialFX::bSnapShotActive;
int32 CSpecialFX::SnapShotFrames;

void
CSpecialFX::Init(void)
{
	C3dMarkers::Init();
	CMoneyMessages::Init();

	CSpecialFX::bSnapShotActive = false;
	CSpecialFX::bVideoCam = false;
	CSpecialFX::SnapShotFrames = 0;
	CSpecialFX::bLiftCam = false;

	// Optimización: No cargamos texturas de humo, ni iniciamos subsistemas de blur/trazadoras.
}

void
CSpecialFX::AddWeaponStreak(int type)
{
	// Vaciado extremo
}

RwObject*
LookForBatCB(RwObject *object, void *data)
{
	return nil;
}

void
CSpecialFX::Update(void)
{
	// Vaciado extremo
}

void
CSpecialFX::Shutdown(void)
{
	C3dMarkers::Shutdown();
}

void
CSpecialFX::Render(void)
{
	PUSH_RENDERGROUP("CSpecialFX::Render");
	CMoneyMessages::Render();
#ifdef NEW_RENDERER
	if (!(gbNewRenderer && FredIsInFirstPersonCam()))
#endif
		C3dMarkers::Render();
	POP_RENDERGROUP();
}

void
CSpecialFX::Render2DFXs(void)
{
	// Optimización extrema: Muerte al bucle de Scanlines. 
	// Solo mantenemos el texto y efecto base del Sniper/Cam.
	if (CSpecialFX::bVideoCam) {
		CFont::SetScale(SCREEN_SCALE_X(1.5f), SCREEN_SCALE_Y(1.5f));
		CFont::SetJustifyOff();
		CFont::SetBackgroundOff();
		CFont::SetCentreOff();
		CFont::SetPropOn();
		CFont::SetColor(CRGBA(0, 255, 0, 200));
		CFont::SetFontStyle(FONT_LOCALE(FONT_STANDARD));
		sprintf(gString, "%d", CTimer::GetFrameCounter() & 0x3F);
		AsciiToUnicode(gString, gUString);
		CFont::PrintString(SCREEN_WIDTH * 8 / 10, SCREEN_HEIGHT * 8 / 10, gUString);
	}
	if (CSpecialFX::bSnapShotActive) {
		if (++CSpecialFX::SnapShotFrames > 20) {
			CSpecialFX::bSnapShotActive = false;
			CTimer::SetTimeScale(1.0f);
		}
		else {
			CTimer::SetTimeScale(0.0f);
			if (CSpecialFX::SnapShotFrames < 10) {
				int32 tmp = (255 - 255 * CSpecialFX::SnapShotFrames / 10) * 0.65f;
				RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
				RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
				CSprite2d::Draw2DPolygon(0.0f, 0.0f, SCREEN_WIDTH, 0.0f, 0.0f, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT, CRGBA(tmp, tmp, tmp, tmp));
				RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
				RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
			}
		}
	}
}

// -------------------------------------------------------------------------------------------------
// Muerte a Motion Blur y Trazadoras de Balas
// -------------------------------------------------------------------------------------------------
CRegisteredMotionBlurStreak CMotionBlurStreaks::aStreaks[NUMMBLURSTREAKS];
void CRegisteredMotionBlurStreak::Update(void) { }
void CRegisteredMotionBlurStreak::Render(void) { }
void CMotionBlurStreaks::Init(void) { }
void CMotionBlurStreaks::Update(void) { }
void CMotionBlurStreaks::RegisterStreak(uintptr id, uint8 r, uint8 g, uint8 b, CVector p1, CVector p2) { }
void CMotionBlurStreaks::Render(void) { }

CBulletTrace CBulletTraces::aTraces[NUMBULLETTRACES];
void CBulletTraces::Init(void) { }
void CBulletTraces::AddTrace(CVector* start, CVector* end, float thickness, uint32 lifeTime, uint8 visibility) { }
void CBulletTraces::AddTrace(CVector* start, CVector* end, int32 weaponType, class CEntity* shooter) { }
void CBulletTraces::Render(void) { }
void CBulletTraces::Update(void) { }
void CBulletTrace::Update(void) { }

// -------------------------------------------------------------------------------------------------
// Marcadores de misiones (Obligatorios, pero optimizados)
// -------------------------------------------------------------------------------------------------
RpAtomic *MarkerAtomicCB(RpAtomic *atomic, void *data) { *(RpAtomic**)data = atomic; return atomic; }

bool
C3dMarker::AddMarker(uint32 identifier, uint16 type, float fSize, uint8 r, uint8 g, uint8 b, uint8 a, uint16 pulsePeriod, float pulseFraction, int16 rotateRate)
{
	m_nIdentifier = identifier;
	m_Matrix.SetUnity();
	RpAtomic *origAtomic = nil;
	RpClumpForAllAtomics(C3dMarkers::m_pRpClumpArray[type], MarkerAtomicCB, &origAtomic);
	RpAtomic *atomic = RpAtomicClone(origAtomic);
	RwFrame *frame = RwFrameCreate();
	RpAtomicSetFrame(atomic, frame);
	CVisibilityPlugins::SetAtomicRenderCallback(atomic, nil);
	RpGeometry *geometry = RpAtomicGetGeometry(atomic);
	RpGeometrySetFlags(geometry, RpGeometryGetFlags(geometry) | rpGEOMETRYMODULATEMATERIALCOLOR);
	m_pAtomic = atomic;
	m_Matrix.Attach(RwFrameGetMatrix(RpAtomicGetFrame(m_pAtomic)));
	m_pMaterial = RpGeometryGetMaterial(geometry, 0);
	m_fSize = fSize;
	m_fStdSize = m_fSize;
	m_Color.red = r; m_Color.green = g; m_Color.blue = b; m_Color.alpha = a;
	m_nPulsePeriod = pulsePeriod;
	m_fPulseFraction = pulseFraction;
	m_nRotateRate = rotateRate;
	m_nStartTime = CTimer::GetTimeInMilliseconds();
	m_nType = type;
	return m_pAtomic != nil;
}

void C3dMarker::DeleteMarkerObject()
{
	m_nIdentifier = 0; m_nStartTime = 0; m_bIsUsed = false;
	m_bFindZOnNextPlacement = false; m_nType = MARKERTYPE_INVALID;
	RwFrame *frame = RpAtomicGetFrame(m_pAtomic);
	RpAtomicDestroy(m_pAtomic);
	RwFrameDestroy(frame);
	m_pAtomic = nil;
}

void C3dMarker::Render()
{
	if (m_pAtomic == nil) return;
	RpMaterialSetColor(m_pMaterial, &m_Color);
	m_Matrix.UpdateRW();
	CMatrix matrix;
	matrix.Attach(m_Matrix.m_attachment);
	matrix.Scale(m_fSize);
	matrix.UpdateRW();
	RwFrameUpdateObjects(RpAtomicGetFrame(m_pAtomic));
	SetBrightMarkerColours(m_fBrightness);
	if (m_nType != MARKERTYPE_ARROW) RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
	RpAtomicRender(m_pAtomic);
	if (m_nType != MARKERTYPE_ARROW) RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
	ReSetAmbientAndDirectionalColours();
}

C3dMarker C3dMarkers::m_aMarkerArray[NUM3DMARKERS];
int32 C3dMarkers::NumActiveMarkers;
RpClump* C3dMarkers::m_pRpClumpArray[NUMMARKERTYPES];

void C3dMarkers::Init()
{
	for (int i = 0; i < NUM3DMARKERS; i++) {
		m_aMarkerArray[i].m_pAtomic = nil;
		m_aMarkerArray[i].m_nType = MARKERTYPE_INVALID;
		m_aMarkerArray[i].m_bIsUsed = false;
		m_aMarkerArray[i].m_bFindZOnNextPlacement = false;
		m_aMarkerArray[i].m_nIdentifier = 0;
		m_aMarkerArray[i].m_nPulsePeriod = 1024;
		m_aMarkerArray[i].m_nRotateRate = 5;
		m_aMarkerArray[i].m_nStartTime = 0;
		m_aMarkerArray[i].m_fPulseFraction = 0.25f;
		m_aMarkerArray[i].m_fStdSize = 1.0f;
		m_aMarkerArray[i].m_fSize = 1.0f;
		m_aMarkerArray[i].m_fBrightness = 1.0f;
		m_aMarkerArray[i].m_fCameraRange = 0.0f;
	}
	NumActiveMarkers = 0;
	int txdSlot = CTxdStore::FindTxdSlot("particle");
	CTxdStore::PushCurrentTxd();
	CTxdStore::SetCurrentTxd(txdSlot);
	CFileMgr::ChangeDir("\\");
	m_pRpClumpArray[MARKERTYPE_ARROW] = CFileLoader::LoadAtomicFile2Return("models/generic/arrow.dff");
	m_pRpClumpArray[MARKERTYPE_CYLINDER] = CFileLoader::LoadAtomicFile2Return("models/generic/zonecylb.dff");
	CTxdStore::PopCurrentTxd();
}

void C3dMarkers::Shutdown()
{
	for (int i = 0; i < NUM3DMARKERS; i++) {
		if (m_aMarkerArray[i].m_pAtomic != nil) m_aMarkerArray[i].DeleteMarkerObject();
	}
	for (int i = 0; i < NUMMARKERTYPES; i++) {
		if (m_pRpClumpArray[i] != nil) RpClumpDestroy(m_pRpClumpArray[i]);
	}
}

void C3dMarkers::Render()
{
	NumActiveMarkers = 0;
	ActivateDirectional();
	for (int i = 0; i < NUM3DMARKERS; i++) {
		if (m_aMarkerArray[i].m_bIsUsed) {
			if (m_aMarkerArray[i].m_fCameraRange < 150.0f) {
				m_aMarkerArray[i].Render();
				// Optimización: CCoronas::RegisterCorona eliminado. Cero destellos lentos.
			}
			NumActiveMarkers++;
			m_aMarkerArray[i].m_bIsUsed = false;
		}
		else if (m_aMarkerArray[i].m_pAtomic != nil) {
			m_aMarkerArray[i].DeleteMarkerObject();
		}
	}
}

C3dMarker *
C3dMarkers::PlaceMarker(uint32 identifier, uint16 type, CVector &pos, float size, uint8 r, uint8 g, uint8 b, uint8 a, uint16 pulsePeriod, float pulseFraction, int16 rotateRate)
{
	C3dMarker *pMarker = nil;
	CVector2D playerPos = FindPlayerCentreOfWorld(0);
	float dist = ((CVector2D)pos - playerPos).Magnitude();
	if (type != MARKERTYPE_ARROW && type != MARKERTYPE_CYLINDER) return nil;

	for (int i = 0; i < NUM3DMARKERS; i++) {
		if (!m_aMarkerArray[i].m_bIsUsed && m_aMarkerArray[i].m_nIdentifier == identifier) { pMarker = &m_aMarkerArray[i]; break; }
	}
	if (pMarker == nil) {
		for (int i = 0; i < NUM3DMARKERS; i++) {
			if (m_aMarkerArray[i].m_nType == MARKERTYPE_INVALID) { pMarker = &m_aMarkerArray[i]; break; }
		}
	}
	if (pMarker == nil && type == MARKERTYPE_ARROW) {
		for (int i = 0; i < NUM3DMARKERS; i++) {
			if (dist < m_aMarkerArray[i].m_fCameraRange && m_aMarkerArray[i].m_nType == MARKERTYPE_ARROW && (pMarker == nil || m_aMarkerArray[i].m_fCameraRange > pMarker->m_fCameraRange)) {
				pMarker = &m_aMarkerArray[i]; break;
			}
		}
		if (pMarker != nil) pMarker->m_nType = MARKERTYPE_INVALID;
	}

	if (pMarker == nil) return pMarker;

	pMarker->m_fCameraRange = dist;
	if (pMarker->m_nIdentifier == identifier && pMarker->m_nType == type) {
		if (type == MARKERTYPE_ARROW) {
			pMarker->m_fStdSize = size;
		}
		else if (type == MARKERTYPE_CYLINDER) {
			pMarker->m_Color.alpha = a;
		}
		float someSin = Sin(TWOPI * (float)((pMarker->m_nPulsePeriod - 1) & (CTimer::GetTimeInMilliseconds() - pMarker->m_nStartTime)) / (float)pMarker->m_nPulsePeriod);
		pMarker->m_fSize = pMarker->m_fStdSize - pulseFraction * pMarker->m_fStdSize * someSin;

		if (type == MARKERTYPE_ARROW) pos.z += 0.25f * pMarker->m_fStdSize * someSin;
		if (pMarker->m_nRotateRate != 0) {
			CVector pos = pMarker->m_Matrix.GetPosition();
			pMarker->m_Matrix.RotateZ(DEGTORAD(pMarker->m_nRotateRate * CTimer::GetTimeStep()));
			pMarker->m_Matrix.GetPosition() = pos;
		}
		if (type == MARKERTYPE_ARROW) pMarker->m_Matrix.GetPosition() = pos;
		if (pMarker->m_bFindZOnNextPlacement) {
			if ((playerPos - pos).MagnitudeSqr() < sq(100.f) && CColStore::HasCollisionLoaded(CVector2D(pos))) {
				float z = CWorld::FindGroundZFor3DCoord(pos.x, pos.y, pos.z + 1.0f, nil);
				if (z != 0.0f) pMarker->m_Matrix.GetPosition().z = z - 0.05f * size;
				pMarker->m_bFindZOnNextPlacement = false;
			}
		}
		pMarker->m_bIsUsed = true;
		return pMarker;
	}

	if (pMarker->m_nIdentifier != 0) pMarker->DeleteMarkerObject();

	pMarker->AddMarker(identifier, type, size, r, g, b, a, pulsePeriod, pulseFraction, rotateRate);
	if (type == MARKERTYPE_CYLINDER) {
		if ((playerPos - pos).MagnitudeSqr() < sq(100.f) && CColStore::HasCollisionLoaded(CVector2D(pos))) {
			float z = CWorld::FindGroundZFor3DCoord(pos.x, pos.y, pos.z + 1.0f, nil);
			if (z != 0.0f) pos.z = z - 0.05f * size;
			pMarker->m_bFindZOnNextPlacement = false;
		}
		else {
			pMarker->m_bFindZOnNextPlacement = true;
		}
	}
	pMarker->m_Matrix.SetTranslate(pos.x, pos.y, pos.z);
	pMarker->m_Matrix.UpdateRW();
	pMarker->m_fStdSize = size;
	pMarker->m_Color.alpha = a;
	pMarker->m_bIsUsed = true;
	return pMarker;
}

void C3dMarkers::PlaceMarkerSet(uint32 id, uint16 type, CVector &pos, float size, uint8 r, uint8 g, uint8 b, uint8 a, uint16 pulsePeriod, float pulseFraction, int16 rotateRate)
{
	PlaceMarker(id, type, pos, size, r, g, b, a, pulsePeriod, pulseFraction, 1);
}
void C3dMarkers::Update() {}

// -------------------------------------------------------------------------------------------------
// Muerte a las luces superpuestas en carros y semáforos
// -------------------------------------------------------------------------------------------------
int CBrightLights::NumBrightLights;
CBrightLight CBrightLights::aBrightLights[NUMBRIGHTLIGHTS];
void CBrightLights::Init(void) { NumBrightLights = 0; }
void CBrightLights::RegisterOne(CVector pos, CVector up, CVector side, CVector front, uint8 type, uint8 red, uint8 green, uint8 blue) { }
void CBrightLights::Render(void) { }
void CBrightLights::RenderOutGeometryBuffer(void) { }

// -------------------------------------------------------------------------------------------------
// Muerte a los textos holográficos/planos
// -------------------------------------------------------------------------------------------------
int CShinyTexts::NumShinyTexts;
CShinyText CShinyTexts::aShinyTexts[NUMSHINYTEXTS];
void CShinyTexts::Init(void) { NumShinyTexts = 0; }
void CShinyTexts::RegisterOne(CVector p0, CVector p1, CVector p2, CVector p3, float u0, float v0, float u1, float v1, float u2, float v2, float u3, float v3, uint8 type, uint8 red, uint8 green, uint8 blue, float maxDist) { }
void CShinyTexts::Render(void) { }
void CShinyTexts::RenderOutGeometryBuffer(void) { }

// -------------------------------------------------------------------------------------------------
// Mensajes de Dinero (Solo se optimiza el renderizado HUD)
// -------------------------------------------------------------------------------------------------
CMoneyMessage CMoneyMessages::aMoneyMessages[NUMMONEYMESSAGES];
void CMoneyMessage::Render() {
	const float MAX_SCALE = 4.0f;
	uint32 nLifeTime = CTimer::GetTimeInMilliseconds() - m_nTimeRegistered;
	if (nLifeTime >= 2000) {
		m_nTimeRegistered = 0;
	}
	else {
		float fLifeTime = (float)nLifeTime / 2000.0f;
		RwV3d vecOut;
		float fDistX, fDistY;
		if (CSprite::CalcScreenCoors(m_vecPosition + CVector(0.0f, 0.0f, fLifeTime), &vecOut, &fDistX, &fDistY, true)) {
			fDistX *= (0.7f * fLifeTime + 2.0f) * m_fSize;
			fDistY *= (0.7f * fLifeTime + 2.0f) * m_fSize;
			CFont::SetPropOn();
			CFont::SetBackgroundOff();
			float fScaleY = Min(fDistY / 100.0f, MAX_SCALE);
			float fScaleX = Min(fDistX / 100.0f, MAX_SCALE);
#ifdef FIX_BUGS
			CFont::SetScale(SCREEN_SCALE_X(fScaleX), SCREEN_SCALE_Y(fScaleY));
#else
			CFont::SetScale(fScaleX, fScaleY);
#endif
			CFont::SetCentreOn();
			CFont::SetCentreSize(SCREEN_WIDTH);
			CFont::SetJustifyOff();
			CFont::SetColor(CRGBA(m_Colour.r, m_Colour.g, m_Colour.b, (255.0f - 255.0f * fLifeTime) * m_fOpacity));
			CFont::SetBackGroundOnlyTextOff();
			FONT_LOCALE(FONT_STANDARD);
			CFont::PrintString(vecOut.x, vecOut.y, m_aText);
		}
	}
}

void CMoneyMessages::Init() {
	for (int32 i = 0; i < NUMMONEYMESSAGES; i++) aMoneyMessages[i].m_nTimeRegistered = 0;
}
void CMoneyMessages::Render() {
	for (int32 i = 0; i < NUMMONEYMESSAGES; i++) {
		if (aMoneyMessages[i].m_nTimeRegistered != 0) aMoneyMessages[i].Render();
	}
}
void CMoneyMessages::RegisterOne(CVector vecPos, const char *pText, uint8 bRed, uint8 bGreen, uint8 bBlue, float fSize, float fOpacity) {
	uint32 i;
	for (i = 0; i < NUMMONEYMESSAGES && aMoneyMessages[i].m_nTimeRegistered != 0; i++);
	if (i < NUMMONEYMESSAGES) {
		AsciiToUnicode(pText, aMoneyMessages[i].m_aText);
		aMoneyMessages[i].m_nTimeRegistered = CTimer::GetTimeInMilliseconds();
		aMoneyMessages[i].m_vecPosition = vecPos;
		aMoneyMessages[i].m_Colour.red = bRed;
		aMoneyMessages[i].m_Colour.green = bGreen;
		aMoneyMessages[i].m_Colour.blue = bBlue;
		aMoneyMessages[i].m_fSize = fSize;
		aMoneyMessages[i].m_fOpacity = fOpacity;
	}
}

// -------------------------------------------------------------------------------------------------
// Muerte a las Partículas de Espuma del Agua
// -------------------------------------------------------------------------------------------------
CRGBA FoamColour(255, 255, 255, 255);
uint32 CSpecialParticleStuff::BoatFromStart;
void CSpecialParticleStuff::CreateFoamAroundObject(CMatrix* pMatrix, float innerFw, float innerRg, float innerUp, int32 particles) { }
void CSpecialParticleStuff::StartBoatFoamAnimation() { BoatFromStart = CTimer::GetTimeInMilliseconds(); }
void CSpecialParticleStuff::UpdateBoatFoamAnimation(CMatrix* pMatrix) { }