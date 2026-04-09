#include "common.h"
#include "main.h"
#include "rwcore.h"
#include "rwplcore.h"
#include "CutsceneShadow.h"
#include "RwHelper.h"

#define DLIGHT_VALUE        0.8f /* Directional light intensity */


CCutsceneShadow::CCutsceneShadow()
{
	m_pAtomic = nil;
	m_nRwObjectType = -1;
	m_pLight = nil;
	m_nBlurPasses = 0;
	m_bResample = false;
	m_bGradient = false;
	m_pObject = nil;
}

CCutsceneShadow::~CCutsceneShadow()
{
	Destroy();
}

bool
CCutsceneShadow::Create(RwObject *object, int32 rasterSize, bool resample, int32 blurPasses, bool gradient)
{
	// Optimización Extrema: Fingimos que la creación fue exitosa para el motor, 
	// pero NO asignamos cámaras (Raster), ni luces, ni texturas en la VRAM.
	if (!object)
		return false;

	m_pObject = object;
	m_nRwObjectType = RwObjectGetType(m_pObject);

	m_nBlurPasses = blurPasses;
	m_bResample = resample;
	m_bGradient = gradient;

	return true;
}

RwFrame *
CCutsceneShadow::SetLightProperties(float angleY, float angleX, bool setLight)
{
	// Sin luz direccional dinámica, no hay matriz que actualizar.
	return nil;
}

bool
CCutsceneShadow::IsInitialized()
{
	// LA CLAVE DEL RENDIMIENTO: Obliga a todas las funciones de cinemáticas 
	// a saltarse el renderizado de la sombra en tiempo real.
	return false;
}

void
CCutsceneShadow::Destroy()
{
	// Limpieza segura: Como no creamos objetos RwRaster o RpLight en Create(), 
	// solo reseteamos el puntero.
	m_pAtomic = nil;
	m_nRwObjectType = -1;
	m_pLight = nil;
	m_pObject = nil;
}

RwRaster *
CCutsceneShadow::Update()
{
	// Vaciado: Cero recálculos geométricos.
	return nil;
}

RwTexture *
CCutsceneShadow::UpdateForCutscene()
{
	return nil;
}

CShadowCamera *
CCutsceneShadow::GetShadowCamera(int32 camType)
{
	// Devolvemos la referencia vacía de m_Camera para evitar 
	// que el motor crashee si intenta leer atributos nulos.
	return &m_Camera;
}

RwTexture *
CCutsceneShadow::GetShadowRwTexture()
{
	return nil;
}

void
CCutsceneShadow::DrawBorderAroundTexture(RwRGBA  const& color)
{
	// Vaciado.
}