#include "common.h"
#include "Coronas.h"
#include "Camera.h"
#include "Clock.h"
#include "Weather.h"
#include "Timecycle.h"
#include "Sprite.h"
#include "World.h"
#include "TxdStore.h"
#include "General.h"
#include "Entity.h"

// Variables estrictamente necesarias para el linker
float CCoronas::LightsMult = 1.0f;
float CCoronas::SunScreenX = 1000000.0f;
float CCoronas::SunScreenY = 1000000.0f;
int CCoronas::MoonSize = 3;
bool CCoronas::SunBlockedByClouds = false;
int CCoronas::bChangeBrightnessImmediately = 0;

RwTexture *gpCoronaTexture[9] = { nil };
CRegisteredCorona CCoronas::aCoronas[NUMCORONAS]; // Vacío pero necesario para compilar

// Constantes de textura
const char aCoronaSpriteNames[][32] = {
	"coronastar", "corona", "coronamoon", "coronareflect",
	"coronaheadlightline", "coronahex", "coronacircle", "coronaringa", "streek"
};

void CCoronas::Init(void) {
	// Ya no cargamos NINGUNA textura. Ahorro total de memoria RAM y VRAM.
}

void CCoronas::Shutdown(void) {
	for (int i = 0; i < 9; i++) {
		if (gpCoronaTexture[i]) {
			RwTextureDestroy(gpCoronaTexture[i]);
			gpCoronaTexture[i] = nil;
		}
	}
}

// Funciones vaciadas para rendimiento absoluto (Costo CPU/GPU = 0)
void CCoronas::Update(void) { }
void CRegisteredCorona::Update(void) { }
void CCoronas::RenderReflections(void) { }
void CCoronas::RenderSunReflection(void) { }
void CEntity::ProcessLightsForEntity(void) { }
void CCoronas::UpdateCoronaCoors(uint32 id, const CVector &coors, float drawDist, float someAngle) { }

// Ignoramos completamente el registro de cualquier otra luz del juego
void CCoronas::RegisterCorona(uint32 id, uint8 red, uint8 green, uint8 blue, uint8 alpha, const CVector &coors, float size, float drawDist, RwTexture *tex, int8 flareType, uint8 reflection, uint8 LOScheck, uint8 drawStreak, float someAngle, bool useNearDist, float nearDist) { }
void CCoronas::RegisterCorona(uint32 id, uint8 red, uint8 green, uint8 blue, uint8 alpha, const CVector &coors, float size, float drawDist, uint8 type, int8 flareType, uint8 reflection, uint8 LOScheck, uint8 drawStreak, float someAngle, bool useNearDist, float nearDist) { }

// -------------------------------------------------------------------------
// RENDERIZADO DIRECTO: ELIMINACIÓN TOTAL
// -------------------------------------------------------------------------

void CCoronas::Render(void) {
	// Completamente vacío. El motor pasará por aquí y saldrá instantáneamente.
}

void CCoronas::DoSunAndMoon(void) {
	// Nada de cálculos espaciales ni renderizado.
	SunScreenX = 1000000.0f;
	SunScreenY = 1000000.0f;
}