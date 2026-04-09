#include "common.h"

#include "main.h"
#include "CutsceneMgr.h"
#include "Lights.h"
#include "Camera.h"
#include "Weather.h"
#include "World.h"
#include "Collision.h"
#include "Sprite.h"
#include "Timer.h"
#include "PointLights.h"

int16 CPointLights::NumLights;
CRegisteredPointLight CPointLights::aLights[NUMPOINTLIGHTS];
CVector CPointLights::aCachedMapReads[32];
float CPointLights::aCachedMapReadResults[32];
int32 CPointLights::NextCachedValue;

void
CPointLights::Init(void)
{
	// OPTIMIZACIÓN: Vaciado. Ya no necesitamos caché de colisiones para farolas
	NextCachedValue = 0;
}

void
CPointLights::InitPerFrame(void)
{
	NumLights = 0;
}

void
CPointLights::AddLight(uint8 type, CVector coors, CVector dir, float radius, float red, float green, float blue, uint8 fogType, bool castExtraShadows)
{
	// OPTIMIZACIÓN EXTREMA: 
	// Literalmente apagamos el sol (bueno, las farolas y explosiones). 
	// Ya no se registran luces dinámicas en el arreglo. Cero CPU gastada.
}

float
CPointLights::GenerateLightsAffectingObject(Const CVector *objCoors)
{
	// OPTIMIZACIÓN EXTREMA: 
	// Si alguna función olvidada pregunta "¿Hay luz cerca?", siempre le decimos que no (devuelve multiplicador 1.0 base).
	return 1.0f;
}

void
CPointLights::RemoveLightsAffectingObject(void)
{
	// Mantenemos esta simple orden nativa de RenderWare para evitar 
	// que la memoria gráfica acumule basura si algo insertó una luz forzada.
	RemoveExtraDirectionalLights(Scene.world);
}

float FogSizes[8] = { 1.3f, 2.0f, 1.7f, 2.0f, 1.4f, 2.1f, 1.5f, 2.3f };

void
CPointLights::RenderFogEffect(void)
{
	// OPTIMIZACIÓN EXTREMA Y DEFINITIVA:
	// Muerte a la niebla volumétrica de las farolas. 
	// Muerte a los bucles 'for' anidados.
	// Muerte al Raycasting (ProcessVerticalLine) en cada frame.
	// Muerte al Alpha Blending giratorio (RenderBufferedOneXLUSprite).
}

bool
CPointLights::ProcessVerticalLineUsingCache(CVector coors, float *groundZ)
{
	// Anulado
	return false;
}