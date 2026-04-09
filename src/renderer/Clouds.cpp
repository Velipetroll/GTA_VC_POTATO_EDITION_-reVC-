#include "common.h"

#include "main.h"
#include "Sprite.h"
#include "Sprite2d.h"
#include "General.h"
#include "Game.h"
#include "Coronas.h"
#include "Camera.h"
#include "TxdStore.h"
#include "Weather.h"
#include "Clock.h"
#include "Timer.h"
#include "Timecycle.h"
#include "Renderer.h"
#include "Clouds.h"

#define SMALLSTRIPHEIGHT 4.0f
#define HORIZSTRIPHEIGHT 48.0f

RwTexture *gpCloudTex[5];

float CClouds::CloudRotation;
uint32 CClouds::IndividualRotation;

float CClouds::ms_cameraRoll;
float CClouds::ms_horizonZ;
float CClouds::ms_HorizonTilt;
CRGBA CClouds::ms_colourTop;
CRGBA CClouds::ms_colourBottom;
CRGBA CClouds::ms_colourBkGrd;

void
CClouds::Init(void)
{
	gpCloudTex[0] = nil;
	gpCloudTex[1] = nil;
	gpCloudTex[2] = nil;
	gpCloudTex[3] = nil;
	gpCloudTex[4] = nil;
	CloudRotation = 0.0f;
}

void
CClouds::Shutdown(void)
{
}

void
CClouds::Update(void)
{
}

// Variables mantenidas vivas para el Linker
float StarCoorsX[9]; float StarCoorsY[9]; float StarSizes[9];
float LowCloudsX[12]; float LowCloudsY[12]; float LowCloudsZ[12];
float CoorsOffsetX[37]; float CoorsOffsetY[37]; float CoorsOffsetZ[37];
uint8 BowRed[6]; uint8 BowGreen[6]; uint8 BowBlue[6];

void
CClouds::Render(void)
{
	CCoronas::SunBlockedByClouds = false;
}

bool
UseDarkBackground(void)
{
	return TheCamera.GetForward().z < -0.9f || gbShowCollisionPolys;
}

void
CClouds::RenderBackground(int16 topred, int16 topgreen, int16 topblue,
	int16 botred, int16 botgreen, int16 botblue, int16 alpha)
{
	if (UseDarkBackground()) {
		ms_colourTop.r = 50; ms_colourTop.g = 50; ms_colourTop.b = 50; ms_colourTop.a = 255;
		if (gbShowCollisionPolys && (CTimer::GetFrameCounter() & 1) == 0) {
			ms_colourTop.r = 255; ms_colourTop.g = 255; ms_colourTop.b = 255;
		}
		CRect r(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT);
		CSprite2d::DrawRect(r, ms_colourTop, ms_colourTop, ms_colourTop, ms_colourTop);
		return;
	}

	// Calculamos DÓNDE está el horizonte en la pantalla (es baratísimo para el CPU)
	ms_horizonZ = CSprite::CalcHorizonCoors();

	// Color del cielo alto
	ms_colourTop.r = topred; ms_colourTop.g = topgreen; ms_colourTop.b = topblue; ms_colourTop.a = alpha;
	// Color del horizonte (niebla)
	ms_colourBottom.r = botred; ms_colourBottom.g = botgreen; ms_colourBottom.b = botblue; ms_colourBottom.a = alpha;

	// OPTIMIZACIÓN EXTREMA CON TRANSICIÓN: 
	// Dibujamos un solo rectángulo desde la parte superior (0.0f) hasta la línea del horizonte.
	// Le pasamos el color Top para la parte superior y el color Bottom para la parte inferior.
	// La GPU hace el degradado automáticamente a 0 costo de rendimiento.
	CSprite2d::DrawAnyRect(
		0.0f, 0.0f,                  // Vértice Superior Izquierdo
		SCREEN_WIDTH, 0.0f,          // Vértice Superior Derecho
		0.0f, ms_horizonZ,           // Vértice Inferior Izquierdo (Toca el horizonte)
		SCREEN_WIDTH, ms_horizonZ,   // Vértice Inferior Derecho (Toca el horizonte)
		ms_colourTop, ms_colourTop,  // Colores de arriba
		ms_colourBottom, ms_colourBottom // Colores de abajo (se fusionarán)
	);
}

void
CClouds::RenderHorizon(void)
{
	if (UseDarkBackground())
		return;

	// Color del suelo base
	ms_colourBkGrd.r = 128.0f*CTimeCycle::GetAmbientRed();
	ms_colourBkGrd.g = 128.0f*CTimeCycle::GetAmbientGreen();
	ms_colourBkGrd.b = 128.0f*CTimeCycle::GetAmbientBlue();
	ms_colourBkGrd.a = 255;

	// TRANSICIÓN DEL SUELO:
	// Dibujamos desde la línea del horizonte hasta el final de la pantalla (SCREEN_HEIGHT).
	// Usamos "ms_colourBottom" (el color de la niebla) en la parte superior del suelo
	// para que se fusione perfectamente con el cielo sin dejar una línea dura.
	CSprite2d::DrawAnyRect(
		0.0f, ms_horizonZ,                // Superior Izquierdo (Toca el horizonte)
		SCREEN_WIDTH, ms_horizonZ,        // Superior Derecho (Toca el horizonte)
		0.0f, SCREEN_HEIGHT,              // Inferior Izquierdo (Fondo de la pantalla)
		SCREEN_WIDTH, SCREEN_HEIGHT,      // Inferior Derecho (Fondo de la pantalla)
		ms_colourBottom, ms_colourBottom, // Colores de arriba (Niebla)
		ms_colourBkGrd, ms_colourBkGrd    // Colores de abajo (Suelo)
	);
}