#include "common.h"

#include "WaterCannon.h"
#include "Vector.h"
#include "General.h"
#include "main.h"
#include "Timer.h"
#include "Pools.h"
#include "Ped.h"
#include "AnimManager.h"
#include "Fire.h"
#include "WaterLevel.h"
#include "Camera.h"
#include "Particle.h"

#define WATERCANNONVERTS 4
#define WATERCANNONINDEXES 12

RwIm3DVertex WaterCannonVertices[WATERCANNONVERTS];
RwImVertexIndex WaterCannonIndexList[WATERCANNONINDEXES];

CWaterCannon CWaterCannons::aCannons[NUM_WATERCANNONS];

void CWaterCannon::Init(void)
{
	m_nId = 0;
	m_nCur = 0;
	m_nTimeCreated = CTimer::GetTimeInMilliseconds();

	for (int32 i = 0; i < NUM_SEGMENTPOINTS; i++)
		m_abUsed[i] = false;
}

void CWaterCannon::Update_OncePerFrame(int16 index)
{
	if (CTimer::GetTimeInMilliseconds() > m_nTimeCreated + WATERCANNON_LIFETIME)
	{
		m_nCur = (m_nCur + 1) % NUM_SEGMENTPOINTS;
		m_abUsed[m_nCur] = false;
	}

	for (int32 i = 0; i < NUM_SEGMENTPOINTS; i++)
	{
		if (m_abUsed[i])
		{
			m_avecVelocity[i].z += -WATERCANNON_GRAVITY * CTimer::GetTimeStep();
			m_avecPos[i] += m_avecVelocity[i] * CTimer::GetTimeStep();
		}
	}

	// Mantenemos esto: Es vital para poder pasar las misiones de bombero.
	for (int32 i = 0; i < NUM_SEGMENTPOINTS; i++)
	{
		if (m_abUsed[i] && gFireManager.ExtinguishPointWithWater(m_avecPos[i], 4.0f))
		{
			break;
		}
	}

	// Optimización extrema: Eliminado el escaneo masivo del PedPool para empujar peatones.
	// if ( ((index + CTimer::GetFrameCounter()) & 3) == 0 )
	//	PushPeds();

	// free if unused
	int32 i = 0;
	while (1)
	{
		if (m_abUsed[i])
			break;

		if (++i >= NUM_SEGMENTPOINTS)
		{
			m_nId = 0;
			return;
		}
	}
}

void CWaterCannon::Update_NewInput(CVector *pos, CVector *dir)
{
	m_avecPos[m_nCur] = *pos;
	m_avecVelocity[m_nCur] = *dir;
	m_abUsed[m_nCur] = true;
}

void CWaterCannon::Render(void)
{
	// Optimización extrema: Cero cálculos de matrices, cero dibujado 3D, cero Alpha Blending.
	return;
}

void CWaterCannon::PushPeds(void)
{
	// Optimización extrema: Función vaciada. Evita recorrer todo el CPool de peatones, 
	// ahorrando toneladas de ciclos de CPU, especialmente en zonas con mucha gente.
	return;
}

void CWaterCannons::Init(void)
{
	for (int32 i = 0; i < NUM_WATERCANNONS; i++)
		aCannons[i].Init();
}

void CWaterCannons::UpdateOne(uint32 id, CVector *pos, CVector *dir)
{
	// find the one by id
	{
		int32 n = 0;
		while (n < NUM_WATERCANNONS && id != aCannons[n].m_nId)
			n++;

		if (n < NUM_WATERCANNONS)
		{
			aCannons[n].Update_NewInput(pos, dir);
			return;
		}
	}

	// if no luck then find a free one
	{
		int32 n = 0;
		while (n < NUM_WATERCANNONS && 0 != aCannons[n].m_nId)
			n++;

		if (n < NUM_WATERCANNONS)
		{
			aCannons[n].Init();
			aCannons[n].m_nId = id;
			aCannons[n].Update_NewInput(pos, dir);
			return;
		}
	}
}

void CWaterCannons::Update(void)
{
	for (int32 i = 0; i < NUM_WATERCANNONS; i++)
	{
		if (aCannons[i].m_nId != 0)
			aCannons[i].Update_OncePerFrame(i);
	}
}

void CWaterCannons::Render(void)
{
	// Optimización extrema
	return;
}