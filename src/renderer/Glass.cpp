#include "common.h"

#include "Glass.h"
#include "Timer.h"
#include "Object.h"
#include "Vehicle.h"
#include "Pools.h"
#include "General.h"
#include "AudioScriptObject.h"
#include "World.h"
#include "Timecycle.h"
#include "Particle.h"
#include "Camera.h"
#include "RenderBuffer.h"
#include "Shadows.h"
#include "ModelIndices.h"
#include "main.h"
#include "soundlist.h"
#include "SurfaceTable.h"

uint32 CGlass::NumGlassEntities;
CEntity *CGlass::apEntitiesToBeRendered[NUM_GLASSENTITIES];
CFallingGlassPane CGlass::aGlassPanes[NUM_GLASSPANES];

CVector2D CentersWithTriangle[NUM_GLASSTRIANGLES];
const CVector2D CoorsWithTriangle[NUM_GLASSTRIANGLES][3] = { { CVector2D(0,0), CVector2D(0,0), CVector2D(0,0) } }; // Dummy

																												   // =========================================================================
																												   // --- POTATO EDITION: FUNCIONES VACIADAS PARA AHORRO EXTREMO DE CPU ---
																												   // =========================================================================

void CFallingGlassPane::Update(void) { /* Vaciado: Sin físicas de caída */ }
void CFallingGlassPane::Render(void) { /* Vaciado: Sin renderizado de partículas */ }

void CGlass::Init(void)
{
	NumGlassEntities = 0;
	for (int32 i = 0; i < NUM_GLASSPANES; i++)
		aGlassPanes[i].m_bActive = false;
}

void CGlass::Update(void) { /* Vaciado: No se actualizan los vidrios */ }

void CGlass::Render(void)
{
	// Vaciado por completo. Solo reseteamos el contador.
	NumGlassEntities = 0;
}

CFallingGlassPane *CGlass::FindFreePane(void) { return nil; }

void CGlass::GeneratePanesForWindow(uint32 type, CVector pos, CVector up, CVector right, CVector speed, CVector center, float moveSpeed, bool cracked, bool explosion, int32 stepmul, bool carGlass)
{
	// Vaciado: El juego ya no genera cientos de polígonos voladores
}

void CGlass::AskForObjectToBeRenderedInGlass(CEntity *entity) { /* Vaciado */ }
void CGlass::RenderEntityInGlass(CEntity *entity) { /* Vaciado */ }
int32 CGlass::CalcAlphaWithNormal(CVector *normal) { return 0; }
void CGlass::RenderHiLightPolys(void) { /* Vaciado */ }
void CGlass::RenderShatteredPolys(void) { /* Vaciado */ }
void CGlass::RenderReflectionPolys(void) { /* Vaciado */ }

// =========================================================================
// --- LÓGICA DE JUEGO INTACTA (Solo sonido y banderas físicas) ---
// =========================================================================

void CGlass::WindowRespondsToCollision(CEntity *entity, float amount, CVector speed, CVector point, bool explosion)
{
	if (!entity) return;
	CObject *object = (CObject *)entity;

	if (object->bGlassBroken)
		return;

	// Aplicamos los cambios físicos para que el jugador pueda atravesarlo
	object->bGlassCracked = true;
	object->bGlassBroken = true;
	object->bIsVisible = false;
	object->bUsesCollision = false;

	// Solo sonido, sin generar partículas de vidrio
	if (amount > 300.0f)
		PlayOneShotScriptObject(SCRIPT_SOUND_GLASS_BREAK_L, object->GetPosition());
	else
		PlayOneShotScriptObject(SCRIPT_SOUND_GLASS_BREAK_S, object->GetPosition());
}

void CGlass::WindowRespondsToSoftCollision(CEntity *entity, float amount)
{
	if (!entity) return;
	CObject *object = (CObject *)entity;

	if (entity->bUsesCollision && amount > 50.0f && !object->bGlassCracked)
	{
		PlayOneShotScriptObject(SCRIPT_SOUND_GLASS_CRACK, object->GetPosition());
		object->bGlassCracked = true;
	}
}

void CGlass::WasGlassHitByBullet(CEntity *entity, CVector point)
{
	if (!entity) return;
	CObject *object = (CObject *)entity;

	if (IsGlass(object->GetModelIndex()))
	{
		if (object->bUsesCollision)
		{
			if (!object->bGlassCracked)
			{
				PlayOneShotScriptObject(SCRIPT_SOUND_GLASS_CRACK, object->GetPosition());
				object->bGlassCracked = true;
			}
			else
			{
				if ((CGeneral::GetRandomNumber() & 3) == 2)
					WindowRespondsToCollision(object, 0.0f, CVector(0.0f, 0.0f, 0.0f), point, false);
			}
		}
	}
}

void CGlass::WindowRespondsToExplosion(CEntity *entity, CVector point)
{
	if (!entity) return;
	CObject *object = (CObject *)entity;

	if (object->bUsesCollision)
	{
		float fDistToGlass = (object->GetPosition() - point).Magnitude();

		if (fDistToGlass < 10.0f)
		{
			WindowRespondsToCollision(object, 10000.0f, CVector(0, 0, 0), object->GetPosition(), true);
		}
		else if (fDistToGlass < 30.0f)
		{
			object->bGlassCracked = true;
		}
	}
}

void CGlass::CarWindscreenShatters(CVehicle *vehicle, bool unk)
{
	if (!vehicle) return;
	// Sonido al romperse el parabrisas, sin física de pedazos voladores
	PlayOneShotScriptObject(SCRIPT_SOUND_GLASS_BREAK_L, vehicle->GetPosition());
}

// Estas dos funciones se mantienen intactas porque ciertas misiones dependen de saber si una ventana está rota
bool CGlass::HasGlassBeenShatteredAtCoors(float x, float y, float z)
{
	CEntity *entity = nil;
	float dist = 20.0f;

	int32 nStartX = Max(CWorld::GetSectorIndexX(x - 30.0f), 0);
	int32 nStartY = Max(CWorld::GetSectorIndexY(y - 30.0f), 0);
	int32 nEndX = Min(CWorld::GetSectorIndexX(x + 30.0f), NUMSECTORS_X - 1);
	int32 nEndY = Min(CWorld::GetSectorIndexY(y + 30.0f), NUMSECTORS_Y - 1);

	CWorld::AdvanceCurrentScanCode();

	for (int32 ys = nStartY; ys <= nEndY; ys++)
	{
		for (int32 xs = nStartX; xs <= nEndX; xs++)
		{
			CSector *sector = CWorld::GetSector(xs, ys);
			if (!sector) continue;

			FindWindowSectorList(sector->m_lists[ENTITYLIST_OBJECTS], &dist, &entity, x, y, z);
			FindWindowSectorList(sector->m_lists[ENTITYLIST_DUMMIES], &dist, &entity, x, y, z);
		}
	}

	if (entity)
	{
		if (entity->GetType() == ENTITY_TYPE_DUMMY)
			return false;

		return !!((CObject*)entity)->bGlassBroken;
	}

	return false;
}

void CGlass::FindWindowSectorList(CPtrList &list, float *dist, CEntity **entity, float x, float y, float z)
{
	CPtrNode *node = list.first;
	while (node != nil)
	{
		CEntity *ent = (CEntity *)node->item;
		uint16 scanCode = ent->m_scanCode;
		node = node->next;

		if (IsGlass(ent->GetModelIndex()))
		{
			if (scanCode != CWorld::GetCurrentScanCode())
			{
				ent->m_scanCode = CWorld::GetCurrentScanCode();
				float dst = (CVector(x, y, z) - ent->GetPosition()).Magnitude();
				if (dst < *dist)
				{
					*dist = dst;
					*entity = ent;
				}
			}
		}
	}
}

void CGlass::BreakGlassPhysically(CVector pos, float radius)
{
	static uint32 breakTime = 0;

	if (CTimer::GetTimeInMilliseconds() < breakTime + 1000 && CTimer::GetTimeInMilliseconds() >= breakTime)
		return;

	CColSphere sphere;
	sphere.piece = 0;
	sphere.radius = radius;
	sphere.surface = 0;

	for (int32 i = CPools::GetObjectPool()->GetSize() - 1; i >= 0; i--)
	{
		CObject *object = CPools::GetObjectPool()->GetSlot(i);
		if (object && IsGlass(object->GetModelIndex()) && object->bUsesCollision)
		{
			CColModel *col = object->GetColModel();
			if (col && col->numTriangles >= 2)
			{
				bool hit = false;
				CVector dist = pos - object->GetPosition();

				sphere.center.x = DotProduct(dist, object->GetRight());
				sphere.center.y = DotProduct(dist, object->GetForward());
				sphere.center.z = DotProduct(dist, object->GetUp());

				CCollision::CalculateTrianglePlanes(col);

				for (int32 j = 0; j < col->numTriangles; j++)
				{
					if (CCollision::TestSphereTriangle(sphere, col->vertices, col->triangles[j], col->trianglePlanes[j]))
					{
						hit = true;
						break; // Salimos temprano para ahorrar CPU
					}
				}

				if (hit)
				{
					breakTime = CTimer::GetTimeInMilliseconds();

					if (object->bGlassCracked)
					{
						PlayOneShotScriptObject(SCRIPT_SOUND_GLASS_BREAK_S, object->GetPosition());

						// Sin generar panes voladores. Solo cambiamos la colisión.
						object->bGlassBroken = true;
						object->bIsVisible = false;
						object->bUsesCollision = false;
					}
					else
					{
						PlayOneShotScriptObject(SCRIPT_SOUND_GLASS_CRACK, object->GetPosition());
						object->bGlassCracked = true;
					}
				}
			}
		}
	}
}