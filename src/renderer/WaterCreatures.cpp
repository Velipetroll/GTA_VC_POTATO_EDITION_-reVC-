#include "common.h"
#include "WaterCreatures.h"
#include "ModelIndices.h"
#include "World.h"
#include "WaterLevel.h"
#include "Camera.h"
#include "PlayerPed.h"
#include "General.h"
#include "Object.h"

int CWaterCreatures::nNumActiveSeaLifeForms;
CWaterCreature CWaterCreatures::aWaterCreatures[NUM_WATER_CREATURES];

// Se mantiene el array intacto para evitar que el enlazador (linker) 
// arroje errores si otro subsistema intenta leer el tamaño o los índices.
struct WaterCreatureProperties aProperties[65] = {
	{ &MI_FISH1SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH2SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH3SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH3S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH1SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH2SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH3SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH3S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_JELLYFISH,	0.01f, 2.2f, 0.0005f, 3.5f },
	{ &MI_JELLYFISH01,	0.01f, 2.2f, 0.0005f, 3.5f },
	{ &MI_FISH1SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH2SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH3SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH3S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH1SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH2SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH3SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH3S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_TURTLE,		0.01f, 2.0f, 0.0005f, 4.0f },
	{ &MI_FISH1SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH2SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH3SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH3S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH1SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH2SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH3SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH3S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_DOLPHIN,		0.03f, 1.5f, 0.0005f, 4.0f },
	{ &MI_FISH1SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH2SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH3SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH3S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH1SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH2SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH3SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH3S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_SHARK,		0.03f, 0.4f, 0.0005f, 4.0f },
	{ &MI_FISH1SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH2SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH3SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH3S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH1SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH2SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH2S,		0.04f, 1.5f, 0.0008f, 3.0f },
	{ &MI_FISH3SINGLE,	0.04f, 1.0f, 0.0008f, 3.0f },
	{ &MI_FISH3S,		0.04f, 1.5f, 0.0008f, 3.0f },
};

CWaterCreature::CWaterCreature() {
}

void CWaterCreature::Initialise(CObject *pObj, float fFwdSpeed, float fZTurnSpeed, float fWaterDepth, uint32 alpha, eFishSlotState state) {
}

void CWaterCreature::Allocate(CObject *pObj, float fFwdSpeed, float fZTurnSpeed, float fWaterDepth, uint32 alpha, eFishSlotState state) {
}

void CWaterCreature::Free() {
}

CWaterCreature *CWaterCreatures::GetFishStructSlot() {
	return nil;
}

CObject *CWaterCreatures::CreateSeaLifeForm(CVector const& pos, int16 modelID, int32 zRotAngle) {
	return nil;
}

bool CWaterCreatures::IsSpaceForMoreWaterCreatures() {
	return false;
}

float CWaterCreatures::CalculateFishHeading(CVector const& pos1, CVector const& pos2) {
	return 0.0f;
}

void CWaterCreatures::CreateOne(CVector const& pos, int32 modelID) {
	// Optimización extrema: Anulada la creación de vida marina.
}

void CWaterCreatures::FreeFishStructSlot(CWaterCreature *wc) {
}

void CWaterCreatures::UpdateAll() {
	// Optimización extrema: Cero cálculos de fotogramas, físicas de nado o profundidad.
}

void CWaterCreatures::RemoveAll() {
	nNumActiveSeaLifeForms = 0;
}