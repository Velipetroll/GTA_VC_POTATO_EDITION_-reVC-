#ifndef LIBRW
#define WITHD3D
#endif
#include "common.h"
#ifndef LIBRW
#include <d3d8caps.h>
#endif

#include "main.h"
#include "General.h"
#include "RwHelper.h"
#include "Camera.h"
#include "Timecycle.h"
#include "Particle.h"
#include "Timer.h"
#include "Hud.h"
#include "Frontend.h"
#include "MBlur.h"
#include "postfx.h"

RwRaster *CMBlur::pFrontBuffer;
bool CMBlur::ms_bJustInitialised;
bool CMBlur::ms_bScaledBlur;
bool CMBlur::BlurOn;
float CMBlur::Drunkness;

int32 CMBlur::pBufVertCount;

static RwIm2DVertex Vertex[4];
static RwIm2DVertex Vertex2[4];
static RwImVertexIndex Index[6] = { 0, 1, 2, 0, 2, 3 };

#ifndef LIBRW
extern "C" D3DCAPS8 _RwD3D8DeviceCaps;
#endif

RwBool
CMBlur::MotionBlurOpen(RwCamera *cam)
{
	// VITAL PARA NO CRASHEAR: Mantener el puente con el filtro de color de VC
#ifdef EXTENDED_COLOURFILTER
	CPostFX::Open(cam);
	return TRUE;
#else
	// Optimizado para no reservar VRAM
	pFrontBuffer = nil;
	BlurOn = false;
	ms_bJustInitialised = false;
	ms_bScaledBlur = false;
	return TRUE;
#endif
}

RwBool
CMBlur::MotionBlurClose(void)
{
#ifdef EXTENDED_COLOURFILTER
	CPostFX::Close();
	return TRUE;
#else
	if (pFrontBuffer) {
		RwRasterDestroy(pFrontBuffer);
		pFrontBuffer = nil;
		return TRUE;
	}
	return FALSE;
#endif
}

void
CMBlur::CreateImmediateModeData(RwCamera *cam, RwRect *rect)
{
	// Vaciado seguro
}

void
CMBlur::CreateImmediateModeData(RwCamera *cam, RwRect *rect, RwIm2DVertex *verts, RwRGBA color, float u1Off, float v1Off, float u2Off, float v2Off, float z, int fullTexture)
{
	// Vaciado seguro
}

void
CMBlur::MotionBlurRender(RwCamera *cam, uint32 red, uint32 green, uint32 blue, uint32 blur, int32 type, uint32 bluralpha)
{
	// VITAL PARA NO CRASHEAR
#ifdef EXTENDED_COLOURFILTER
	CPostFX::Render(cam, red, green, blue, blur, type, bluralpha);
#else
	// Vaciado: Cero blur en versiones vanilla
#endif
}

void
CMBlur::OverlayRender(RwCamera *cam, RwRaster *raster, RwRGBA color, int32 type, int32 bluralpha)
{
	// Vaciado extremo: Nada de capas secundarias
}

void
CMBlur::SetDrunkBlur(float drunkness)
{
	// Cero distorsión al beber
	Drunkness = 0.0f;
}

void
CMBlur::ClearDrunkBlur()
{
	Drunkness = 0.0f;
	CTimer::SetTimeScale(1.0f);
}

// -------------------------------------------------------------------------------------------------
// Muerte a los Fx de lente (Gotas de agua, sangre, distorsión por calor)
// -------------------------------------------------------------------------------------------------

#define NUM_RENDER_FX 64

static RwRect fxRect[NUM_RENDER_FX];
static FxType fxType[NUM_RENDER_FX];
static float fxZ[NUM_RENDER_FX];

bool
CMBlur::PosInside(RwRect *rect, float x1, float y1, float x2, float y2)
{
	return false;
}

bool
CMBlur::AddRenderFx(RwCamera *cam, RwRect *rect, float z, FxType type)
{
	// Abortamos agregar efectos al búfer
	return false;
}

void
CMBlur::OverlayRenderFx(RwCamera *cam, RwRaster *frontBuf)
{
	// Cero renderizado de gotas/sangre/calor con stencil
	pBufVertCount = 0;
}