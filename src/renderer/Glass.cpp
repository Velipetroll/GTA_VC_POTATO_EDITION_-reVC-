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
const CVector2D CoorsWithTriangle[NUM_GLASSTRIANGLES][3] =
{
	{
		CVector2D(0.0f, 0.0f),
		CVector2D(0.0f, 1.0f),
		CVector2D(0.4f, 0.5f)
	},

	{
		CVector2D(0.0f, 1.0f),
		CVector2D(1.0f, 1.0f),
		CVector2D(0.4f, 0.5f)
	},

	{
		CVector2D(0.0f, 0.0f),
		CVector2D(0.4f, 0.5f),
		CVector2D(0.7f, 0.0f)
	},

	{
		CVector2D(0.7f, 0.0f),
		CVector2D(0.4f, 0.5f),
		CVector2D(1.0f, 1.0f)
	},

	{
		CVector2D(0.7f, 0.0f),
		CVector2D(1.0f, 1.0f),
		CVector2D(1.0f, 0.0f)
	}
};

#define TEMPBUFFERVERTHILIGHTOFFSET     0
#define TEMPBUFFERINDEXHILIGHTOFFSET    0
#define TEMPBUFFERVERTHILIGHTSIZE       256
#define TEMPBUFFERINDEXHILIGHTSIZE      512

#define TEMPBUFFERVERTSHATTEREDOFFSET   TEMPBUFFERVERTHILIGHTSIZE
#define TEMPBUFFERINDEXSHATTEREDOFFSET  TEMPBUFFERINDEXHILIGHTSIZE
#define TEMPBUFFERVERTSHATTEREDSIZE     384
#define TEMPBUFFERINDEXSHATTEREDSIZE    768

#define TEMPBUFFERVERTREFLECTIONOFFSET  TEMPBUFFERVERTSHATTEREDSIZE
#define TEMPBUFFERINDEXREFLECTIONOFFSET TEMPBUFFERINDEXSHATTEREDSIZE
#define TEMPBUFFERVERTREFLECTIONSIZE    512
#define TEMPBUFFERINDEXREFLECTIONSIZE   1024

int32 TempBufferIndicesStoredHiLight     = 0;
int32 TempBufferVerticesStoredHiLight    = 0;
int32 TempBufferIndicesStoredShattered   = 0;
int32 TempBufferVerticesStoredShattered  = 0;
int32 TempBufferIndicesStoredReflection  = 0;
int32 TempBufferVerticesStoredReflection = 0;

void
CFallingGlassPane::Update(void)
{
    return;
}

void
CFallingGlassPane::Render(void)
{
    return;
}

void
CGlass::Init(void)
{
    return;
}

void
CGlass::Update(void)
{
    return;
}

void
CGlass::Render(void)
{
    return;
}

CFallingGlassPane *
CGlass::FindFreePane(void)
{
    return nil;
}

void
CGlass::GeneratePanesForWindow(uint32 type, CVector pos, CVector up, CVector right, CVector speed, CVector center,
								float moveSpeed, bool cracked, bool explosion, int32 stepmul, bool carGlass)
{
    return;
}

void
CGlass::AskForObjectToBeRenderedInGlass(CEntity *entity)
{
    return;
}

void
CGlass::RenderEntityInGlass(CEntity *entity)
{
    return;
}

int32
CGlass::CalcAlphaWithNormal(CVector *normal)
{
    return 0;
}

void
CGlass::RenderHiLightPolys(void)
{
    return;
}

void
CGlass::RenderShatteredPolys(void)
{
    return;
}

void
CGlass::RenderReflectionPolys(void)
{
    return;
}

void
CGlass::WindowRespondsToCollision(CEntity *entity, float amount, CVector speed, CVector point, bool explosion)
{
    return;
}

void
CGlass::WindowRespondsToSoftCollision(CEntity *entity, float amount)
{
    return;
}

void
CGlass::WasGlassHitByBullet(CEntity *entity, CVector point)
{
    return;
}

void
CGlass::WindowRespondsToExplosion(CEntity *entity, CVector point)
{
    return;
}

void
CGlass::CarWindscreenShatters(CVehicle *vehicle, bool unk)
{
    return;
}

bool
CGlass::HasGlassBeenShatteredAtCoors(float x, float y, float z)
{
    return false;
}

void
CGlass::FindWindowSectorList(CPtrList &list, float *dist, CEntity **entity, float x, float y, float z)
{
    return;
}

void
CGlass::BreakGlassPhysically(CVector pos, float radius)
{
    return;
}