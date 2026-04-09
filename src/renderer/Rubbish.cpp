#include "common.h"
#include "main.h"

#include "General.h"
#include "Timer.h"
#include "Weather.h"
#include "Camera.h"
#include "World.h"
#include "Vehicle.h"
#include "ZoneCull.h"
#include "Stats.h"
#include "TxdStore.h"
#include "RenderBuffer.h"
#include "Rubbish.h"

#define RUBBISH_MAX_DIST (23.0f)
#define RUBBISH_FADE_DIST (20.0f)

RwTexture *gpRubbishTexture[4];
RwImVertexIndex RubbishIndexList[6];
bool CRubbish::bRubbishInvisible;
int CRubbish::RubbishVisibility;
COneSheet CRubbish::aSheets[NUM_RUBBISH_SHEETS];
COneSheet CRubbish::StartEmptyList;
COneSheet CRubbish::EndEmptyList;
COneSheet CRubbish::StartStaticsList;
COneSheet CRubbish::EndStaticsList;
COneSheet CRubbish::StartMoversList;
COneSheet CRubbish::EndMoversList;


void
COneSheet::AddToList(COneSheet *list)
{
	this->m_next = list->m_next;
	this->m_prev = list;
	list->m_next = this;
	this->m_next->m_prev = this;
}

void
COneSheet::RemoveFromList(void)
{
	m_next->m_prev = m_prev;
	m_prev->m_next = m_next;
}


void
CRubbish::Render(void)
{
	// Optimización extrema: Cero renderizado de basura, hojas o periódicos.
	return;
}

void
CRubbish::StirUp(CVehicle *veh)
{
	// Optimización extrema: Ignorar cálculos físicos del viento al pasar vehículos.
	return;
}

void
CRubbish::Update(void)
{
	// Optimización extrema: Aniquilación del bucle de lógica y generación de polígonos.
	return;
}

void
CRubbish::SetVisibility(bool visible)
{
	bRubbishInvisible = !visible;
}

void
CRubbish::Init(void)
{
	// Inicializamos las listas en blanco de forma segura para evitar crashes del motor
	StartEmptyList.m_next = &EndEmptyList;
	StartEmptyList.m_prev = nil;
	EndEmptyList.m_next = nil;
	EndEmptyList.m_prev = &StartEmptyList;

	StartStaticsList.m_next = &EndStaticsList;
	StartStaticsList.m_prev = nil;
	EndStaticsList.m_next = nil;
	EndStaticsList.m_prev = &StartStaticsList;

	StartMoversList.m_next = &EndMoversList;
	StartMoversList.m_prev = nil;
	EndMoversList.m_next = nil;
	EndMoversList.m_prev = &StartMoversList;

	// Optimización de Memoria (RAM/VRAM): NO cargamos las texturas
	gpRubbishTexture[0] = nil;
	gpRubbishTexture[1] = nil;
	gpRubbishTexture[2] = nil;
	gpRubbishTexture[3] = nil;

	RubbishVisibility = 0;
	bRubbishInvisible = true;
}

void
CRubbish::Shutdown(void)
{
	// Como evitamos cargar las texturas en memoria durante el Init, no hay nada que limpiar aquí.
	return;
}