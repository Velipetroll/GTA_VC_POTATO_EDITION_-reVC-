#include "common.h"

#include "Draw.h"
#include "Frontend.h"
#include "Camera.h"
#include "CutsceneMgr.h"

float CDraw::ms_fAspectRatio = DEFAULT_ASPECT_RATIO;
#ifdef ASPECT_RATIO_SCALE
float CDraw::ms_fScaledFOV = 45.0f;
#endif

float CDraw::ms_fNearClipZ;
float CDraw::ms_fFarClipZ;
float CDraw::ms_fFOV = 45.0f;
float CDraw::ms_fLODDistance;

uint8 CDraw::FadeValue;
uint8 CDraw::FadeRed;
uint8 CDraw::FadeGreen;
uint8 CDraw::FadeBlue;

#ifdef PROPER_SCALING	
bool CDraw::ms_bProperScaling = true;
#endif
#ifdef FIX_RADAR
bool CDraw::ms_bFixRadar = true;
#endif
#ifdef FIX_SPRITES
bool CDraw::ms_bFixSprites = true;
#endif

#ifdef ASPECT_RATIO_SCALE
float
FindAspectRatio(void)
{
	switch (FrontEndMenuManager.m_PrefsUseWideScreen) {
	case AR_AUTO:
		return SCREEN_WIDTH / (float)SCREEN_HEIGHT;
	default:
		// OPTIMIZACIÓN: Divisiones pre-calculadas para no asfixiar la FPU
	case AR_4_3:
		return 1.3333333f;
	case AR_5_4:
		return 1.25f;
	case AR_16_10:
		return 1.6f;
	case AR_16_9:
		return 1.7777777f;
	case AR_21_9:
		return 2.3333333f;
	};
}
#endif

float
CDraw::CalculateAspectRatio(void)
{
#ifdef ASPECT_RATIO_SCALE
	if (TheCamera.m_WideScreenOn)
		CDraw::ms_fAspectRatio = 1.6666666f * FindAspectRatio() * 0.5625f;
	else
		CDraw::ms_fAspectRatio = FindAspectRatio();
#else
	if (FrontEndMenuManager.m_PrefsUseWideScreen) {
		if (TheCamera.m_WideScreenOn)
			CDraw::ms_fAspectRatio = 1.6666666f;
		else
			CDraw::ms_fAspectRatio = 1.7777777f;
	}
	else if (TheCamera.m_WideScreenOn) {
		CDraw::ms_fAspectRatio = 1.25f;
	}
	else {
		CDraw::ms_fAspectRatio = 1.3333333f;
	}
#endif
	return CDraw::ms_fAspectRatio;
}

#ifdef ASPECT_RATIO_SCALE
float
CDraw::ConvertFOV(float hfov)
{
	// OPTIMIZACIÓN EXTREMA: Muerte a la trigonometría.
	// En lugar de usar Tangentes y Arcotangentes cruzadas, usamos una 
	// regla de aproximación lineal súper barata.
	// 0.75f equivale a dividir por el Aspect Ratio base (4/3).
	return hfov * (GetAspectRatio() * 0.75f);
}
#endif

void
CDraw::SetFOV(float fov)
{
#ifdef ASPECT_RATIO_SCALE
	if (!CCutsceneMgr::IsRunning())
		ms_fScaledFOV = ConvertFOV(fov);
	else
		ms_fScaledFOV = fov;
#endif
	ms_fFOV = fov;
}

#ifdef PROPER_SCALING	
float CDraw::ScaleY(float y)
{
	// Multiplicador pre-calculado
	return ms_bProperScaling ? y : y * 1.0666666f;
}
#endif