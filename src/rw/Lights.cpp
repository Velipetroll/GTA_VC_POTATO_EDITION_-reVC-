#include "common.h"
#include <rwcore.h>
#include <rpworld.h>

#include "Lights.h"
#include "Timer.h"
#include "Timecycle.h"
#include "Coronas.h"
#include "Weather.h"
#include "ZoneCull.h"
#include "Frontend.h"
#include "MBlur.h"

RpLight *pAmbient;
RpLight *pDirect;
RpLight *pExtraDirectionals[4] = { nil };
int LightStrengths[4];
int NumExtraDirLightsInWorld = 0;

RwRGBAReal AmbientLightColourForFrame = { 1.0f, 1.0f, 1.0f, 1.0f };
RwRGBAReal AmbientLightColourForFrame_PedsCarsAndObjects = { 1.0f, 1.0f, 1.0f, 1.0f };
RwRGBAReal DirectionalLightColourForFrame = { 1.0f, 1.0f, 1.0f, 1.0f };

RwRGBAReal AmbientLightColour = { 1.0f, 1.0f, 1.0f, 1.0f };
RwRGBAReal DirectionalLightColour = { 1.0f, 1.0f, 1.0f, 1.0f };
RwRGBAReal FullLight = { 1.0f, 1.0f, 1.0f, 1.0f };

void
SetLightsWithTimeOfDayColour(RpWorld *)
{
	if (pAmbient)
		RpLightSetColor(pAmbient, &AmbientLightColourForFrame);
}

RpWorld*
LightsCreate(RpWorld *world)
{
	if (world == nil)
		return nil;

	pAmbient = RpLightCreate(rpLIGHTAMBIENT);
	RpLightSetFlags(pAmbient, rpLIGHTLIGHTATOMICS);
	RpLightSetColor(pAmbient, &FullLight);

	pDirect = RpLightCreate(rpLIGHTDIRECTIONAL);
	RpLightSetFlags(pDirect, 0);
	RpLightSetColor(pDirect, &FullLight);

	RwFrame *frame = RwFrameCreate();
	RpLightSetFrame(pDirect, frame);

	RpWorldAddLight(world, pAmbient);
	RpWorldAddLight(world, pDirect);

	return world;
}

void
LightsDestroy(RpWorld *world)
{
	if (world == nil)
		return;

	if (pAmbient) {
		RpWorldRemoveLight(world, pAmbient);
		RpLightDestroy(pAmbient);
		pAmbient = nil;
	}

	if (pDirect) {
		RpWorldRemoveLight(world, pDirect);
		RwFrameDestroy(RpLightGetFrame(pDirect));
		RpLightDestroy(pDirect);
		pDirect = nil;
	}
}

// =========================================================
// EFECTO DE CARRO QUEMADO (RESTAURADO)
// =========================================================
void WorldReplaceNormalLightsWithScorched(RpWorld *world, float l) {
	RwRGBAReal color;
	color.red = l;
	color.green = l;
	color.blue = l;
	if (pAmbient) RpLightSetColor(pAmbient, &color);
}

void WorldReplaceScorchedLightsWithNormal(RpWorld *world) {
	if (pAmbient) RpLightSetColor(pAmbient, &AmbientLightColourForFrame);
}

// =========================================================
// SELLOS DE FUGA: RESTAURAMOS ESTAS FUNCIONES PARA PROTEGER
// A LAS HOJAS, PAPELES Y PEATONES DEL EFECTO QUEMADO.
// =========================================================

void SetFullAmbient(void) {
	if (pAmbient) RpLightSetColor(pAmbient, &FullLight);
}

void SetAmbientColours(void) {
	if (pAmbient) RpLightSetColor(pAmbient, &AmbientLightColourForFrame);
}

void SetAmbientColoursForPedsCarsAndObjects(void) {
	if (pAmbient) RpLightSetColor(pAmbient, &AmbientLightColourForFrame_PedsCarsAndObjects);
}

void SetAmbientColours(RwRGBAReal *color) {
	if (pAmbient && color) RpLightSetColor(pAmbient, color);
}

// =========================================================
// MATEMÁTICAS PESADAS VACIADAS (MÁXIMO RENDIMIENTO)
// =========================================================

void AddAnExtraDirectionalLight(RpWorld *world, float dirx, float diry, float dirz, float red, float green, float blue) {}
void RemoveExtraDirectionalLights(RpWorld *world) {}
void SetAmbientAndDirectionalColours(float f) {}
void SetFlashyColours(float f) {}
void SetFlashyColours_Mild(float f) {}
void SetBrightMarkerColours(float f) {}
void ReSetAmbientAndDirectionalColours(void) {}
void DeActivateDirectional(void) {}
void ActivateDirectional(void) {}
void SetAmbientColoursToIndicateRoadGroup(int i) {}