#include "common.h"

#include "main.h"
#include "TxdStore.h"
#include "Timer.h"
#include "Camera.h"
#include "Timecycle.h"
#include "CutsceneMgr.h"
#include "Automobile.h"
#include "Bike.h"
#include "Ped.h"
#include "PlayerPed.h"
#include "World.h"
#include "Weather.h"
#include "ModelIndices.h"
#include "RenderBuffer.h"
#ifdef FIX_BUGS
#include "Replay.h"
#endif
#include "PointLights.h"
#include "SpecialFX.h"
#include "Script.h"
#include "TimeStep.h"
#include "Shadows.h"
#include "CutsceneObject.h"
#include "CutsceneShadow.h"
#include "Clock.h"
#include "VarConsole.h"

#ifdef DEBUGMENU
//SETTWEAKPATH("Shadows");
//TWEAKBOOL(gbPrintShite);
#endif

RwImVertexIndex ShadowIndexList[24];

RwTexture *gpShadowCarTex;
RwTexture *gpShadowPedTex;
RwTexture *gpShadowHeliTex;
RwTexture *gpShadowBikeTex;
RwTexture *gpShadowBaronTex;
RwTexture *gpShadowExplosionTex;
RwTexture *gpShadowHeadLightsTex;
RwTexture *gpOutline1Tex;
RwTexture *gpOutline2Tex;
RwTexture *gpOutline3Tex;
RwTexture *gpBloodPoolTex;
RwTexture *gpReflectionTex;
RwTexture *gpWalkDontTex;
RwTexture *gpCrackedGlassTex;
RwTexture *gpPostShadowTex;
RwTexture *gpGoalTex;

int16            CShadows::ShadowsStoredToBeRendered;
CStoredShadow    CShadows::asShadowsStored  [MAX_STOREDSHADOWS];
CPolyBunch       CShadows::aPolyBunches     [MAX_POLYBUNCHES];
CStaticShadow    CShadows::aStaticShadows   [MAX_STATICSHADOWS];
CPolyBunch      *CShadows::pEmptyBunchList;
CPermanentShadow CShadows::aPermanentShadows[MAX_PERMAMENTSHADOWS];

#ifndef MASTER
bool gbCountPolysInShadow;
#endif

void
CShadows::Init(void)
{
    return;
}

void
CShadows::Shutdown(void)
{
    return;
}

void
CShadows::AddPermanentShadow(uint8 ShadowType, RwTexture *pTexture, CVector *pPosn,
							float fFrontX, float fFrontY, float fSideX, float fSideY,
							int16 nIntensity, uint8 nRed, uint8 nGreen, uint8 nBlue,
							float fZDistance, uint32 nTime, float fScale)
{
    return;
}

bool
CShadows::StoreStaticShadow(uint32 nID, uint8 ShadowType, RwTexture *pTexture, Const CVector *pPosn,
							float fFrontX, float fFrontY, float fSideX, float fSideY,
							int16 nIntensity, uint8 nRed, uint8 nGreen, uint8 nBlue,
							float fZDistance, float fScale, float fDrawDistance, bool bTempShadow, float fUpDistance)
{
    return false;
}

void
CShadows::StoreShadowToBeRendered(uint8 ShadowTexture, CVector *pPosn,
								float fFrontX, float fFrontY, float fSideX, float fSideY,
								int16 nIntensity, uint8 nRed, uint8 nGreen, uint8 nBlue)
{
    return;
}

void
CShadows::StoreShadowToBeRendered(uint8 ShadowType, RwTexture *pTexture, CVector *pPosn,
								float fFrontX, float fFrontY, float fSideX, float fSideY,
								int16 nIntensity, uint8 nRed, uint8 nGreen, uint8 nBlue,
								float fZDistance, bool bDrawOnWater, float fScale, CCutsceneShadow *pShadow, bool bDrawOnBuildings)
{
    return;
}

void
CShadows::StoreShadowForVehicle(CVehicle *pCar, VEH_SHD_TYPE type)
{
    return;
}

void
CShadows::StoreCarLightShadow(CVehicle *pCar, int32 nID, RwTexture *pTexture, CVector *pPosn,
							float fFrontX, float fFrontY, float fSideX, float fSideY, uint8 nRed, uint8 nGreen, uint8 nBlue,
							float fMaxViewAngle)
{
    return;
}

#ifdef USE_CUTSCENE_SHADOW_FOR_PED
void
StoreShadowForCutscenePedObject(CPed *pObject, float fDisplacementX, float fDisplacementY,
								float fFrontX, float fFrontY, float fSideX, float fSideY)
{
    return;
}
#endif

void
CShadows::StoreShadowForPed(CPed *pPed, float fDisplacementX, float fDisplacementY,
							float fFrontX, float fFrontY, float fSideX, float fSideY)
{
    return;
}

void
CShadows::StoreShadowForPedObject(CEntity *pPedObject, float fDisplacementX, float fDisplacementY,
								float fFrontX, float fFrontY, float fSideX, float fSideY)
{
    return;
}

void
CShadows::StoreShadowForCutscenePedObject(CCutsceneObject *pObject, float fDisplacementX, float fDisplacementY,
								float fFrontX, float fFrontY, float fSideX, float fSideY)
{
    return;
}

void
CShadows::StoreShadowForTree(CEntity *pTree)
{
    return;
}

void
CShadows::StoreShadowForPole(CEntity *pPole, float fOffsetX, float fOffsetY, float fOffsetZ,
								float fPoleHeight, float fPoleWidth, uint32 nID)
{
    return;
}

void
CShadows::SetRenderModeForShadowType(uint8 ShadowType)
{
    return;
}

void
CShadows::RenderStoredShadows(void)
{
    return;
}

void
CShadows::RenderStaticShadows(void)
{
    return;
}

void
CShadows::GeneratePolysForStaticShadow(int16 nStaticShadowID)
{
    return;
}

void
CShadows::CastShadowSectorList(CPtrList &PtrList, float fStartX, float fStartY, float fEndX, float fEndY, CVector *pPosn,
								float fFrontX, float fFrontY, float fSideX, float fSideY,
								int16 nIntensity, uint8 nRed, uint8 nGreen, uint8 nBlue,
								float fZDistance, float fScale, CPolyBunch **ppPolyBunch)
{
    return;
}

void
CShadows::CastPlayerShadowSectorList(CPtrList &PtrList, float fStartX, float fStartY, float fEndX, float fEndY, CVector *pPosn,
								float fFrontX, float fFrontY, float fSideX, float fSideY,
								int16 nIntensity, uint8 nRed, uint8 nGreen, uint8 nBlue,
								float fZDistance, float fScale, CPolyBunch **ppPolyBunch)
{
    return;
}

void
CShadows::CastCutsceneShadowSectorList(CPtrList &PtrList, float fStartX, float fStartY, float fEndX, float fEndY, CVector *pPosn,
								float fFrontX, float fFrontY, float fSideX, float fSideY,
								int16 nIntensity, uint8 nRed, uint8 nGreen, uint8 nBlue,
								float fZDistance, float fScale, CPolyBunch **ppPolyBunch, CCutsceneShadow *pShadow)
{
    return;
}

void
CShadows::CastShadowEntityXY(CEntity *pEntity,  float fStartX, float fStartY, float fEndX, float fEndY, CVector *pPosn,
							float fFrontX, float fFrontY, float fSideX, float fSideY,
							int16 nIntensity, uint8 nRed, uint8 nGreen, uint8 nBlue,
							float fZDistance, float fScale, CPolyBunch **ppPolyBunch)
{
    return;
}

// Dummy definition to satisfy compiler
typedef struct _ProjectionParam
{
    int dummy;
} ProjectionParam;

RwV3d *ShadowRenderTriangleCB(RwV3d *points, RwV3d *normal, ProjectionParam *param)
{
    return points;
}

void
CShadows::CastShadowEntityXYZ(CEntity *pEntity, CVector *pPosn,
								float fFrontX, float fFrontY, float fSideX, float fSideY,
								int16 nIntensity, uint8 nRed, uint8 nGreen, uint8 nBlue,
								float fZDistance, float fScale, CPolyBunch **ppPolyBunch, CCutsceneShadow *pShadow)
{
    return;
}

void
CShadows::UpdateStaticShadows(void)
{
    return;
}

void
CShadows::UpdatePermanentShadows(void)
{
    return;
}

void
CStaticShadow::Free(void)
{
    return;
}

void
CShadows::CalcPedShadowValues(CVector vecLightDir,
							float *pfFrontX, float *pfFrontY,
							float *pfSideX, float *pfSideY,
							float *pfDisplacementX, float *pfDisplacementY)
{
    return;
}

void
CShadows::RenderExtraPlayerShadows(void)
{
    return;
}

void
CShadows::TidyUpShadows(void)
{
    return;
}

void
CShadows::RenderIndicatorShadow(uint32 nID, uint8 ShadowType, RwTexture *pTexture,  CVector *pPosn,
								float fFrontX, float fFrontY, float fSideX, float fSideY,
								int16 nIntensity)
{
    return;
}