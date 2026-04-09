#include "common.h"

#include "main.h"
#include "TxdStore.h"
#include "Timer.h"
#include "Replay.h"
#include "Skidmarks.h"

CSkidmark CSkidmarks::aSkidmarks[NUMSKIDMARKS];

RwImVertexIndex SkidmarkIndexList[SKIDMARK_LENGTH * 6];
RwIm3DVertex SkidmarkVertices[SKIDMARK_LENGTH * 2];
RwTexture *gpSkidTex;

void
CSkidmarks::Init(void)
{
	// OPTIMIZACIÓN EXTREMA: No cargamos la textura 'particleskid'. Ahorramos VRAM y RAM.
	gpSkidTex = nil;

	// Inicializamos la lista limpia por seguridad para evitar crasheos de memoria
	for (int i = 0; i < NUMSKIDMARKS; i++) {
		aSkidmarks[i].m_state = 0;
		aSkidmarks[i].m_wasUpdated = false;
	}
}

void
CSkidmarks::Shutdown(void)
{
	// Vaciado: Como nunca cargamos la textura, no hay nada que destruir.
}

void
CSkidmarks::Clear(void)
{
	for (int i = 0; i < NUMSKIDMARKS; i++) {
		aSkidmarks[i].m_state = 0;
		aSkidmarks[i].m_wasUpdated = false;
	}
}

void
CSkidmarks::Update(void)
{
	// OPTIMIZACIÓN EXTREMA: Vaciado. 
	// Cero ciclos de CPU gastados en calcular tiempos de desvanecimiento (Fade) y estados.
}

void
CSkidmarks::Render(void)
{
	// OPTIMIZACIÓN EXTREMA: Vaciado. 
	// Cero uso de "Immediate Mode 3D" (RwIm3D). 
	// Ahorro masivo de comunicación entre CPU y GPU, y cero fillrate gastado en el asfalto.
}

void
CSkidmarks::RegisterOne(uintptr id, const CVector &pos, float fwdX, float fwdY, bool *isMuddy, bool *isBloody)
{
	// Apagamos los punteros instantáneamente para que el juego deje de 
	// generar cálculos de sangre o lodo en otros archivos.
	if (isBloody) *isBloody = false;
	if (isMuddy) *isMuddy = false;
}

void
CSkidmarks::RegisterOne(uintptr id, const CVector &pos, float fwdX, float fwdY, eSkidmarkType type, bool *isBloody)
{
	// Apagamos el puntero de sangre instantáneamente.
	if (isBloody) *isBloody = false;
}