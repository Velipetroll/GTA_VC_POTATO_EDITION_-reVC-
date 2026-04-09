#include "common.h"
#include "main.h"
#include "RenderBuffer.h"
#include "Entity.h"
#include "Fluff.h"
#include "Camera.h"
#include "Sprite.h"
#include "Coronas.h"
#include "PointLights.h"
#include "Rubbish.h"
#include "Timecycle.h"
#include "General.h"
#include "Timer.h"
#include "Clock.h"
#include "Weather.h"
#include "Stats.h"
#include "maths.h"
#include "Frontend.h"
#include "CutsceneMgr.h"
#include "PlayerPed.h"
#include "Bones.h"
#include "World.h"
#include "Replay.h"
#include "SaveBuf.h"

// Variables mantenidas estrictamente para el linker
CPlaneTrail CPlaneTrails::aArray[6];
RwImVertexIndex TrailIndices[32];
CPlaneBanner CPlaneBanners::aArray[5];
bool CSmokeTrails::CigOn = false;
CSmokeTrail CSmokeTrails::aSmoke[3];
RwImVertexIndex SmokeTrailIndices[32];
float RandomSmoke[16];
uint8 ScrollCharSet[59][5];
CMovingThing CMovingThings::StartCloseList;
CMovingThing CMovingThings::EndCloseList;
int16 CMovingThings::Num;
CMovingThing CMovingThings::aMovingThings[NUMMOVINGTHINGS];
int32 CScrollBar::TonightsEvent;
CEscalator CEscalators::aEscalators[NUM_ESCALATORS];
int32 CEscalators::NumEscalators;
CScriptPath CScriptPaths::aArray[3];
CObject *g_pScriptPathObjects[18];

// --- CPlaneTrails ---
void CPlaneTrail::Init(void) { }
void CPlaneTrail::Render(float visibility) { }
void CPlaneTrail::RegisterPoint(CVector pos) { }
void CPlaneTrails::Init(void) { }
void CPlaneTrails::Update(void) { }
void CPlaneTrails::Render(void) { }
void CPlaneTrails::RegisterPoint(CVector pos, uint32 id) { }

// --- CPlaneBanners ---
void CPlaneBanner::Init(void) { }
void CPlaneBanner::Update(void) { }
void CPlaneBanner::Render(void) { }
void CPlaneBanner::RegisterPoint(CVector pos) { }
void CPlaneBanners::Init(void) { }
void CPlaneBanners::Update(void) { }
void CPlaneBanners::Render(void) { }
void CPlaneBanners::RegisterPoint(CVector pos, uint32 id) { }

// --- CSmokeTrails ---
void CSmokeTrail::RegisterPoint(CVector regPosition, float opacity) { }
void CSmokeTrail::Init(int num) { }
void CSmokeTrail::Render(void) { }
void CSmokeTrails::Init(void) { }
void CSmokeTrails::Render(void) { }
void CSmokeTrails::Update(void) { }

// --- CMovingThings ---
void CMovingThings::Init() {
	CPlaneTrails::Init();
	CSmokeTrails::Init();
	CPlaneBanners::Init();
	CPointLights::Init();
	CEscalators::Init();
	Num = 0;
}
void CMovingThings::Shutdown() { CEscalators::Shutdown(); }
void CMovingThings::Update() { }
void CMovingThings::Render() { }
void CMovingThings::RegisterOne(CEntity *pEnt, uint16 nType) { }
void CMovingThings::PossiblyAddThisEntity(CEntity *pEnt) { }
void CMovingThing::Update() { }
void CMovingThing::AddToList(CMovingThing *pThing) { }
void CMovingThing::RemoveFromList() { }
int16 CMovingThing::SizeList() { return 0; }

// --- CScrollBar ---
void CScrollBar::Init(CVector pos1, CVector pos2, uint8 type, uint8 red, uint8 green, uint8 blue, float scale) { }
void CScrollBar::Update() { m_bVisible = false; }
void CScrollBar::Render() { }

// --- CEscalators ---
CEscalator::CEscalator() { m_bIsActive = false; }
void CEscalator::AddThisOne(CVector pos0, CVector pos1, CVector pos2, CVector pos3, bool b_isMovingDown) { }
void CEscalator::Update(void) { }
void CEscalator::SwitchOff(void) { m_bIsActive = false; }
void CEscalators::AddOne(CVector pos0, CVector pos1, CVector pos2, CVector pos3, bool b_isMovingDown) { }
void CEscalators::Init(void) { NumEscalators = 0; }
void CEscalators::Update(void) { }
void CEscalators::Shutdown(void) { NumEscalators = 0; }

// --- CScriptPaths (Mantenemos carga/guardado para no corromper partidas) ---
void CScriptPath::FindCoorsFromDistanceOnPath(float t, float *pX, float *pY, float *pZ) { }
void CScriptPath::Update(void) { }
void CScriptPath::Clear(void) {
	if (m_pNode) delete[] m_pNode;
	m_pNode = nil; m_numNodes = 0;
	for (int i = 0; i < 6; i++) m_pObjects[i] = nil;
	m_state = SCRIPT_PATH_DISABLED;
}
void CScriptPath::InitialiseOne(int32 numNodes, float length) { m_state = SCRIPT_PATH_INITIALIZED; }
void CScriptPath::SetObjectToControl(CObject *pObj) { m_state = SCRIPT_PATH_ACTIVE; }
void CScriptPaths::Init(void) { for (int i = 0; i < 3; i++) aArray[i].Clear(); }
void CScriptPaths::Shutdown(void) { for (int i = 0; i < 3; i++) aArray[i].Clear(); }
void CScriptPaths::Update(void) { }
bool CScriptPaths::IsOneActive(void) { return false; }
void CScriptPaths::Load(uint8 *buf, uint32 size) { }
void CScriptPaths::Save(uint8 *buf, uint32 *size) { }
void CScriptPaths::Load_ForReplay(void) { }
void CScriptPaths::Save_ForReplay(void) { }

const char* FindTimeMessage() { return ""; }