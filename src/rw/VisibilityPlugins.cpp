#include "common.h"

#include "RwHelper.h"
#include "templates.h"
#include "main.h"
#include "Entity.h"
#include "ModelInfo.h"
#include "Lights.h"
#include "RwHelper.h"
#include "Renderer.h"
#include "Camera.h"
#include "VisibilityPlugins.h"
#include "World.h"
#include "custompipes.h"
#include "MemoryHeap.h"

CLinkList<CVisibilityPlugins::AlphaObjectInfo> CVisibilityPlugins::m_alphaList;
CLinkList<CVisibilityPlugins::AlphaObjectInfo> CVisibilityPlugins::m_alphaBoatAtomicList;
CLinkList<CVisibilityPlugins::AlphaObjectInfo> CVisibilityPlugins::m_alphaEntityList;
CLinkList<CVisibilityPlugins::AlphaObjectInfo> CVisibilityPlugins::m_alphaUnderwaterEntityList;
#ifdef NEW_RENDERER
CLinkList<CVisibilityPlugins::AlphaObjectInfo> CVisibilityPlugins::m_alphaBuildingList;
#endif

int32 CVisibilityPlugins::ms_atomicPluginOffset = -1;
int32 CVisibilityPlugins::ms_framePluginOffset = -1;
int32 CVisibilityPlugins::ms_clumpPluginOffset = -1;

RwCamera *CVisibilityPlugins::ms_pCamera;
RwV3d *CVisibilityPlugins::ms_pCameraPosn;
float CVisibilityPlugins::ms_cullCompsDist;
float CVisibilityPlugins::ms_vehicleLod0Dist;
float CVisibilityPlugins::ms_vehicleLod1Dist;
float CVisibilityPlugins::ms_vehicleFadeDist;
float CVisibilityPlugins::ms_bigVehicleLod0Dist;
float CVisibilityPlugins::ms_bigVehicleLod1Dist;
float CVisibilityPlugins::ms_pedLod1Dist;
float CVisibilityPlugins::ms_pedFadeDist;

#define RENDERCALLBACK AtomicDefaultRenderCallBack

void
CVisibilityPlugins::Initialise(void)
{
	// --- OPTIMIZACIÓN ATOM N450 ---
	// Si es posible, reduce NUMALPHALIST y afines a la mitad en tu .h 
	// para evitar cache misses en la CPU al recorrer estas listas enlazadas.
	m_alphaList.Init(NUMALPHALIST);
	m_alphaList.head.item.sort = 0.0f;
	m_alphaList.tail.item.sort = 100000000.0f;

	m_alphaBoatAtomicList.Init(NUMBOATALPHALIST);
	m_alphaBoatAtomicList.head.item.sort = 0.0f;
	m_alphaBoatAtomicList.tail.item.sort = 100000000.0f;

#ifdef ASPECT_RATIO_SCALE
	m_alphaEntityList.Init(NUMALPHAENTITYLIST * 3);
#else
	m_alphaEntityList.Init(NUMALPHAENTITYLIST);
#endif // ASPECT_RATIO_SCALE
	m_alphaEntityList.head.item.sort = 0.0f;
	m_alphaEntityList.tail.item.sort = 100000000.0f;

	m_alphaUnderwaterEntityList.Init(NUMALPHAUNTERWATERENTITYLIST);
	m_alphaUnderwaterEntityList.head.item.sort = 0.0f;
	m_alphaUnderwaterEntityList.tail.item.sort = 100000000.0f;

#ifdef NEW_RENDERER
	m_alphaBuildingList.Init(NUMALPHAENTITYLIST);
	m_alphaBuildingList.head.item.sort = 0.0f;
	m_alphaBuildingList.tail.item.sort = 100000000.0f;
#endif
}

void
CVisibilityPlugins::Shutdown(void)
{
	m_alphaList.Shutdown();
	m_alphaBoatAtomicList.Shutdown();
	m_alphaEntityList.Shutdown();
	m_alphaUnderwaterEntityList.Shutdown();
#ifdef NEW_RENDERER
	m_alphaBuildingList.Shutdown();
#endif
}

void
CVisibilityPlugins::InitAlphaEntityList(void)
{
	m_alphaEntityList.Clear();
	m_alphaBoatAtomicList.Clear();
	m_alphaUnderwaterEntityList.Clear();
#ifdef NEW_RENDERER
	m_alphaBuildingList.Clear();
#endif
}

bool
CVisibilityPlugins::InsertEntityIntoSortedList(CEntity *e, float dist)
{
#ifdef FIX_BUGS
	if (!e->m_rwObject) return true;
#endif

	AlphaObjectInfo item;
	item.entity = e;
	item.sort = dist;
#ifdef NEW_RENDERER
	if (gbNewRenderer && e->IsBuilding())
		return !!m_alphaBuildingList.InsertSorted(item);
#endif
	if (e->bUnderwater && m_alphaUnderwaterEntityList.InsertSorted(item))
		return true;
	return !!m_alphaEntityList.InsertSorted(item);
}

void
CVisibilityPlugins::InitAlphaAtomicList(void)
{
	m_alphaList.Clear();
}

bool
CVisibilityPlugins::InsertAtomicIntoSortedList(RpAtomic *a, float dist)
{
	AlphaObjectInfo item;
	item.atomic = a;
	item.sort = dist;
	return !!m_alphaList.InsertSorted(item);
}

bool
CVisibilityPlugins::InsertAtomicIntoBoatSortedList(RpAtomic *a, float dist)
{
	AlphaObjectInfo item;
	item.atomic = a;
	item.sort = dist;
	return !!m_alphaBoatAtomicList.InsertSorted(item);
}

#define VEHICLE_LODDIST_MULTIPLIER (TheCamera.GenerationDistMultiplier)

void
CVisibilityPlugins::SetRenderWareCamera(RwCamera *camera)
{
	ms_pCamera = camera;
	ms_pCameraPosn = RwMatrixGetPos(RwFrameGetMatrix(RwCameraGetFrame(camera)));

	if (TheCamera.Cams[TheCamera.ActiveCam].Mode == CCam::MODE_TOPDOWN ||
		TheCamera.Cams[TheCamera.ActiveCam].Mode == CCam::MODE_TOP_DOWN_PED)
		ms_cullCompsDist = 1000000.0f;
	else
		ms_cullCompsDist = sq(TheCamera.LODDistMultiplier * 20.0f);

	ms_vehicleLod0Dist = sq(70.0f * VEHICLE_LODDIST_MULTIPLIER);
	ms_vehicleLod1Dist = sq(90.0f * VEHICLE_LODDIST_MULTIPLIER);
	ms_vehicleFadeDist = sq(100.0f * VEHICLE_LODDIST_MULTIPLIER);
	ms_bigVehicleLod0Dist = sq(60.0f * VEHICLE_LODDIST_MULTIPLIER);
	ms_bigVehicleLod1Dist = sq(150.0f * VEHICLE_LODDIST_MULTIPLIER);
	ms_pedLod1Dist = sq(60.0f * TheCamera.LODDistMultiplier);
	ms_pedFadeDist = sq(70.0f * TheCamera.LODDistMultiplier);
}

static float DistToCameraSq;
static float PitchToCamera;

void
CVisibilityPlugins::SetupVehicleVariables(RpClump *vehicle)
{
	if (RwObjectGetType((RwObject*)vehicle) != rpCLUMP)
		return;
	DistToCameraSq = GetDistanceSquaredFromCamera(RpClumpGetFrame(vehicle));
	RwV3d distToCam;
	RwV3dSub(&distToCam, ms_pCameraPosn, &RwFrameGetMatrix(RpClumpGetFrame(vehicle))->pos);

	// --- OPTIMIZACIÓN ATOM N450: Evitar Sqrt y Atan2 ---
	float absX = distToCam.x < 0.0f ? -distToCam.x : distToCam.x;
	float absY = distToCam.y < 0.0f ? -distToCam.y : distToCam.y;
	float dist2d = absX + absY - 0.5f * (absX < absY ? absX : absY);

	PitchToCamera = distToCam.z / (dist2d + 0.001f);
}

RpMaterial*
SetAlphaCB(RpMaterial *material, void *data)
{
	((RwRGBA*)RpMaterialGetColor(material))->alpha = (uint8)(uintptr)data;
	return material;
}

RpMaterial*
SetTextureCB(RpMaterial *material, void *data)
{
	RpMaterialSetTexture(material, (RwTexture*)data);
	return material;
}

void
CVisibilityPlugins::RenderAtomicList(CLinkList<AlphaObjectInfo> &list)
{
	CLink<AlphaObjectInfo> *node;
	for (node = list.tail.prev; node != &list.head; node = node->prev)
		RENDERCALLBACK(node->item.atomic);
}

void
CVisibilityPlugins::RenderAlphaAtomics(void)
{
	// --- OPTIMIZACIÓN GMA 3150: Cero Alpha Blending en Vidrios ---
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);

	RenderAtomicList(m_alphaList);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
}

void
CVisibilityPlugins::RenderBoatAlphaAtomics(void)
{
	SetCullMode(rwCULLMODECULLNONE);

	// --- OPTIMIZACIÓN GMA 3150: Parabrisas sólidos en barcos ---
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);

	RenderAtomicList(m_alphaBoatAtomicList);
	SetCullMode(rwCULLMODECULLBACK);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
}

void
CVisibilityPlugins::RenderFadingEntities(CLinkList<AlphaObjectInfo> &list)
{
	CLink<AlphaObjectInfo> *node;
	CSimpleModelInfo *mi;
	for (node = list.tail.prev; node != &list.head; node = node->prev) {
		CEntity *e = node->item.entity;
		if (e->m_rwObject == nil)
			continue;
#ifdef EXTENDED_PIPELINES
		if (CustomPipes::bRenderingEnvMap && (e->IsPed() || e->IsVehicle()))
			continue;
#endif
		mi = (CSimpleModelInfo *)CModelInfo::GetModelInfo(e->GetModelIndex());
		if (mi->GetModelType() == MITYPE_SIMPLE && mi->m_noZwrite)
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);

		if (e->bDistanceFade) {
			DeActivateDirectional();
			SetAmbientColours();
			e->bImBeingRendered = true;
			PUSH_RENDERGROUP(mi->GetModelName());
			RenderFadingAtomic((RpAtomic*)e->m_rwObject, node->item.sort);
			POP_RENDERGROUP();
			e->bImBeingRendered = false;
		}
		else
			CRenderer::RenderOneNonRoad(e);

		if (mi->GetModelType() == MITYPE_SIMPLE && mi->m_noZwrite)
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
	}
}

void
CVisibilityPlugins::RenderFadingEntities(void)
{
	RenderFadingEntities(m_alphaEntityList);
	RenderBoatAlphaAtomics();
}

void
CVisibilityPlugins::RenderFadingUnderwaterEntities(void)
{
	RenderFadingEntities(m_alphaUnderwaterEntityList);
}

RpAtomic*
CVisibilityPlugins::RenderWheelAtomicCB(RpAtomic *atomic)
{
	RpAtomic *lodatm;
	float len;
	CSimpleModelInfo *mi;

	mi = GetAtomicModelInfo(atomic);

	// --- OPTIMIZACIÓN ATOM N450: Bypass de Sqrt de cerca ---
	if (DistToCameraSq < 1600.0f) {
		RENDERCALLBACK(atomic);
		return atomic;
	}

	len = Sqrt(DistToCameraSq);
	lodatm = mi->GetAtomicFromDistance(len * TheCamera.LODDistMultiplier / VEHICLE_LODDIST_MULTIPLIER);
	if (lodatm) {
		if (RpAtomicGetGeometry(lodatm) != RpAtomicGetGeometry(atomic))
			RpAtomicSetGeometry(atomic, RpAtomicGetGeometry(lodatm), rpATOMICSAMEBOUNDINGSPHERE);
		RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderObjNormalAtomic(RpAtomic *atomic)
{
	RwMatrix *m;
	RwV3d view;
	float len;

	m = RwFrameGetLTM(RpAtomicGetFrame(atomic));
	RwV3dSub(&view, RwMatrixGetPos(m), ms_pCameraPosn);
	len = RwV3dLength(&view);
	if (RwV3dDotProduct(&view, RwMatrixGetUp(m)) < -0.3f*len && len > 8.0f)
		return atomic;
	RENDERCALLBACK(atomic);
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderAlphaAtomic(RpAtomic *atomic, int alpha)
{
	RpGeometry *geo;
	uint32 flags;

	geo = RpAtomicGetGeometry(atomic);
	flags = RpGeometryGetFlags(geo);
	RpGeometrySetFlags(geo, flags | rpGEOMETRYMODULATEMATERIALCOLOR);
	RpGeometryForAllMaterials(geo, SetAlphaCB, (void*)alpha);
	RENDERCALLBACK(atomic);
	RpGeometryForAllMaterials(geo, SetAlphaCB, (void*)255);
	RpGeometrySetFlags(geo, flags);
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderWeaponCB(RpAtomic *atomic)
{
	// --- OPTIMIZACIÓN ATOM N450: Distancia simple Manhattan ---
	CSimpleModelInfo *mi = GetAtomicModelInfo(atomic);
	RwMatrix *m = RwFrameGetLTM(RpAtomicGetFrame(atomic));
	RwV3d view;
	RwV3dSub(&view, RwMatrixGetPos(m), ms_pCameraPosn);

	float absX = view.x < 0.0f ? -view.x : view.x;
	float absY = view.y < 0.0f ? -view.y : view.y;
	float distAprox = absX + absY;

	if (distAprox < mi->GetLodDistance(0))
		RENDERCALLBACK(atomic);

	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderFadingAtomic(RpAtomic *atomic, float camdist)
{
	// --- OPTIMIZACIÓN GMA 3150: Cero Fading (Pop-in duro) ---
	CSimpleModelInfo *mi = GetAtomicModelInfo(atomic);

	if (mi->m_additive)
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

	RENDERCALLBACK(atomic);

	if (mi->m_additive)
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

	return atomic;
}

// --- OPTIMIZACIONES ATOM N450 PARA VEHÍCULOS: Fuerza Bruta, Cero Culling ---

RpAtomic*
CVisibilityPlugins::RenderVehicleHiDetailCB(RpAtomic *atomic)
{
	if (DistToCameraSq < ms_vehicleLod0Dist) {
		RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleHiDetailAlphaCB(RpAtomic *atomic)
{
	if (DistToCameraSq < ms_vehicleLod0Dist) {
		uint32 flags = GetAtomicId(atomic);
		if (flags & ATOMIC_FLAG_DRAWLAST) {
			if (!InsertAtomicIntoSortedList(atomic, DistToCameraSq - 0.0001f))
				RENDERCALLBACK(atomic);
		}
		else {
			if (!InsertAtomicIntoSortedList(atomic, DistToCameraSq))
				RENDERCALLBACK(atomic);
		}
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleHiDetailCB_BigVehicle(RpAtomic *atomic)
{
	if (DistToCameraSq < ms_bigVehicleLod0Dist) {
		RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleHiDetailAlphaCB_BigVehicle(RpAtomic *atomic)
{
	if (DistToCameraSq < ms_bigVehicleLod0Dist) {
		if (!InsertAtomicIntoSortedList(atomic, DistToCameraSq))
			RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleHiDetailCB_Boat(RpAtomic *atomic)
{
	if (DistToCameraSq < ms_vehicleLod0Dist)
		RENDERCALLBACK(atomic);
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleHiDetailAlphaCB_Boat(RpAtomic *atomic)
{
	if (DistToCameraSq < ms_vehicleLod0Dist) {
		if (GetAtomicId(atomic) & ATOMIC_FLAG_DRAWLAST) {
			if (!InsertAtomicIntoBoatSortedList(atomic, DistToCameraSq))
				RENDERCALLBACK(atomic);
		}
		else
			RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleLoDetailCB_Boat(RpAtomic *atomic)
{
	RpClump *clump;
	int32 alpha;

	clump = RpAtomicGetClump(atomic);
	if (DistToCameraSq >= ms_vehicleLod0Dist) {
		alpha = GetClumpAlpha(clump);
		if (alpha == 255)
			RENDERCALLBACK(atomic);
		else
			RenderAlphaAtomic(atomic, alpha);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleLowDetailCB_BigVehicle(RpAtomic *atomic)
{
	if (DistToCameraSq >= ms_bigVehicleLod0Dist && DistToCameraSq < ms_bigVehicleLod1Dist) {
		RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleLowDetailAlphaCB_BigVehicle(RpAtomic *atomic)
{
	if (DistToCameraSq >= ms_bigVehicleLod0Dist && DistToCameraSq < ms_bigVehicleLod1Dist) {
		if (!InsertAtomicIntoSortedList(atomic, DistToCameraSq))
			RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleReallyLowDetailCB(RpAtomic *atomic)
{
	RpClump *clump;
	int32 alpha;

	clump = RpAtomicGetClump(atomic);
	if (DistToCameraSq >= ms_vehicleLod0Dist) {
		alpha = GetClumpAlpha(clump);
		if (alpha == 255)
			RENDERCALLBACK(atomic);
		else
			RenderAlphaAtomic(atomic, alpha);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleReallyLowDetailCB_BigVehicle(RpAtomic *atomic)
{
	if (DistToCameraSq >= ms_bigVehicleLod1Dist)
		RENDERCALLBACK(atomic);
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderTrainHiDetailCB(RpAtomic *atomic)
{
	if (DistToCameraSq < ms_bigVehicleLod1Dist) {
		RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderTrainHiDetailAlphaCB(RpAtomic *atomic)
{
	if (DistToCameraSq < ms_bigVehicleLod1Dist) {
		if (!InsertAtomicIntoSortedList(atomic, DistToCameraSq))
			RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleRotorAlphaCB(RpAtomic *atomic)
{
	// --- OPTIMIZACIÓN ATOM N450 ---
	// Ignoramos el cálculo del DotProduct exacto de las aspas
	if (DistToCameraSq < ms_bigVehicleLod1Dist) {
		if (!InsertAtomicIntoSortedList(atomic, DistToCameraSq))
			RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleTailRotorAlphaCB(RpAtomic *atomic)
{
	// --- OPTIMIZACIÓN ATOM N450 ---
	if (DistToCameraSq < ms_bigVehicleLod0Dist) {
		if (!InsertAtomicIntoSortedList(atomic, DistToCameraSq))
			RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderPlayerCB(RpAtomic *atomic)
{
	if (CWorld::Players[0].m_pSkinTexture)
		RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), SetTextureCB, CWorld::Players[0].m_pSkinTexture);
	RENDERCALLBACK(atomic);
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderPedCB(RpAtomic *atomic)
{
	RpClump *clump;
	float dist;
	int32 alpha;

	clump = RpAtomicGetClump(atomic);
	dist = GetDistanceSquaredFromCamera(RpClumpGetFrame(clump));
	if (dist < ms_pedLod1Dist) {
		alpha = GetClumpAlpha(clump);
		if (alpha == 255)
			RENDERCALLBACK(atomic);
		else
			RenderAlphaAtomic(atomic, alpha);
	}
	return atomic;
}

float
CVisibilityPlugins::GetDistanceSquaredFromCamera(RwV3d *pos)
{
	RwV3d dist;
	RwV3dSub(&dist, pos, ms_pCameraPosn);
	return RwV3dDotProduct(&dist, &dist);
}

float
CVisibilityPlugins::GetDistanceSquaredFromCamera(RwFrame *frame)
{
	RwMatrix *m;
	RwV3d dist;
	m = RwFrameGetLTM(frame);
	RwV3dSub(&dist, RwMatrixGetPos(m), ms_pCameraPosn);
	return RwV3dDotProduct(&dist, &dist);
}

float
CVisibilityPlugins::GetDotProductWithCameraVector(RwMatrix *atomicMat, RwMatrix *clumpMat, uint32 flags)
{
	// NOTA: Esta función ya casi no se llama gracias a nuestra reescritura de fuerza bruta.
	// La dejamos intacta por si otro módulo externo la necesita.
	RwV3d dist;
	float dot, dotdoor;

	RwV3dSub(&dist, RwMatrixGetPos(atomicMat), ms_pCameraPosn);

	if (flags & (ATOMIC_FLAG_FRONT | ATOMIC_FLAG_REAR))
		dot = RwV3dDotProduct(&dist, RwMatrixGetUp(clumpMat));
	else if (flags & (ATOMIC_FLAG_LEFT | ATOMIC_FLAG_RIGHT))
		dot = RwV3dDotProduct(&dist, RwMatrixGetRight(clumpMat));
	else
		dot = 0.0f;
	if (flags & (ATOMIC_FLAG_LEFT | ATOMIC_FLAG_REAR))
		dot = -dot;

	if (flags & (ATOMIC_FLAG_REARDOOR | ATOMIC_FLAG_FRONTDOOR)) {
		if (flags & ATOMIC_FLAG_REARDOOR)
			dotdoor = -RwV3dDotProduct(&dist, RwMatrixGetUp(clumpMat));
		else if (flags & ATOMIC_FLAG_FRONTDOOR)
			dotdoor = RwV3dDotProduct(&dist, RwMatrixGetUp(clumpMat));
		else
			dotdoor = 0.0f;

		if (dot < 0.0f && dotdoor < 0.0f)
			dot += dotdoor;
		if (dot > 0.0f && dotdoor > 0.0f)
			dot += dotdoor;
	}

	return dot;
}

/* These are all unused */

bool
CVisibilityPlugins::DefaultVisibilityCB(RpClump *clump)
{
	return true;
}

bool
CVisibilityPlugins::FrustumSphereCB(RpClump *clump)
{
	RwSphere sphere;
	RwFrame *frame = RpClumpGetFrame(clump);

	CClumpModelInfo *modelInfo = (CClumpModelInfo*)GetFrameHierarchyId(frame);
	sphere.radius = modelInfo->GetColModel()->boundingSphere.radius;
	sphere.center.x = modelInfo->GetColModel()->boundingSphere.center.x;
	sphere.center.y = modelInfo->GetColModel()->boundingSphere.center.y;
	sphere.center.z = modelInfo->GetColModel()->boundingSphere.center.z;
	RwV3dTransformPoints(&sphere.center, &sphere.center, 1, RwFrameGetLTM(frame));
	return RwCameraFrustumTestSphere(ms_pCamera, &sphere) != rwSPHEREOUTSIDE;
}

bool
CVisibilityPlugins::MloVisibilityCB(RpClump *clump)
{
	RwFrame *frame = RpClumpGetFrame(clump);
	CMloModelInfo *modelInfo = (CMloModelInfo*)GetFrameHierarchyId(frame);
	if (SQR(modelInfo->drawDist) < GetDistanceSquaredFromCamera(frame))
		return false;
	return CVisibilityPlugins::FrustumSphereCB(clump);
}

bool
CVisibilityPlugins::VehicleVisibilityCB(RpClump *clump)
{
	RwFrame *frame = RpClumpGetFrame(clump);
	if (ms_vehicleLod1Dist < GetDistanceSquaredFromCamera(frame))
		return false;
	return FrustumSphereCB(clump);
}

bool
CVisibilityPlugins::VehicleVisibilityCB_BigVehicle(RpClump *clump)
{
	return FrustumSphereCB(clump);
}

//
// RW Plugins
//

enum
{
	ID_VISIBILITYATOMIC = MAKECHUNKID(rwVENDORID_ROCKSTAR, 0x00),
	ID_VISIBILITYCLUMP = MAKECHUNKID(rwVENDORID_ROCKSTAR, 0x01),
	ID_VISIBILITYFRAME = MAKECHUNKID(rwVENDORID_ROCKSTAR, 0x02),
};

bool
CVisibilityPlugins::PluginAttach(void)
{
	ms_atomicPluginOffset = RpAtomicRegisterPlugin(sizeof(AtomicExt),
		ID_VISIBILITYATOMIC,
		AtomicConstructor, AtomicDestructor, AtomicCopyConstructor);

	ms_framePluginOffset = RwFrameRegisterPlugin(sizeof(FrameExt),
		ID_VISIBILITYFRAME,
		FrameConstructor, FrameDestructor, FrameCopyConstructor);

	ms_clumpPluginOffset = RpClumpRegisterPlugin(sizeof(ClumpExt),
		ID_VISIBILITYCLUMP,
		ClumpConstructor, ClumpDestructor, ClumpCopyConstructor);
	return ms_atomicPluginOffset != -1 && ms_clumpPluginOffset != -1;
}

#define ATOMICEXT(o) (RWPLUGINOFFSET(AtomicExt, o, ms_atomicPluginOffset))
#define FRAMEEXT(o) (RWPLUGINOFFSET(FrameExt, o, ms_framePluginOffset))
#define CLUMPEXT(o) (RWPLUGINOFFSET(ClumpExt, o, ms_clumpPluginOffset))

//
// Atomic
//

void*
CVisibilityPlugins::AtomicConstructor(void *object, int32, int32)
{
	ATOMICEXT(object)->modelInfo = nil;
	return object;
}

void*
CVisibilityPlugins::AtomicDestructor(void *object, int32, int32)
{
	return object;
}

void*
CVisibilityPlugins::AtomicCopyConstructor(void *dst, const void *src, int32, int32)
{
	*ATOMICEXT(dst) = *ATOMICEXT(src);
	return dst;
}

void
CVisibilityPlugins::SetAtomicModelInfo(RpAtomic *atomic,
	CSimpleModelInfo *modelInfo)
{
	AtomicExt *ext = ATOMICEXT(atomic);
	ext->modelInfo = modelInfo;
}

CSimpleModelInfo*
CVisibilityPlugins::GetAtomicModelInfo(RpAtomic *atomic)
{
	return ATOMICEXT(atomic)->modelInfo;
}

void
CVisibilityPlugins::SetAtomicFlag(RpAtomic *atomic, int f)
{
	ATOMICEXT(atomic)->flags |= f;
}

void
CVisibilityPlugins::ClearAtomicFlag(RpAtomic *atomic, int f)
{
	ATOMICEXT(atomic)->flags &= ~f;
}

void
CVisibilityPlugins::SetAtomicId(RpAtomic *atomic, int id)
{
	ATOMICEXT(atomic)->flags = id;
}

int
CVisibilityPlugins::GetAtomicId(RpAtomic *atomic)
{
	return ATOMICEXT(atomic)->flags;
}

void
CVisibilityPlugins::SetAtomicRenderCallback(RpAtomic *atomic, RpAtomicCallBackRender cb)
{
	if (cb == nil)
		cb = RENDERCALLBACK;	// not necessary
	RpAtomicSetRenderCallBack(atomic, cb);
}

//
// Frame
//

void*
CVisibilityPlugins::FrameConstructor(void *object, int32, int32)
{
	FRAMEEXT(object)->id = 0;
	return object;
}

void*
CVisibilityPlugins::FrameDestructor(void *object, int32, int32)
{
	return object;
}

void*
CVisibilityPlugins::FrameCopyConstructor(void *dst, const void *src, int32, int32)
{
	*FRAMEEXT(dst) = *FRAMEEXT(src);
	return dst;
}

void
CVisibilityPlugins::SetFrameHierarchyId(RwFrame *frame, intptr id)
{
	FRAMEEXT(frame)->id = id;
}

intptr
CVisibilityPlugins::GetFrameHierarchyId(RwFrame *frame)
{
	return FRAMEEXT(frame)->id;
}

//
// Clump
//

void*
CVisibilityPlugins::ClumpConstructor(void *object, int32, int32)
{
	ClumpExt *ext = CLUMPEXT(object);
	ext->visibilityCB = DefaultVisibilityCB;
	ext->alpha = 0xFF;
	return object;
}

void*
CVisibilityPlugins::ClumpDestructor(void *object, int32, int32)
{
	return object;
}

void*
CVisibilityPlugins::ClumpCopyConstructor(void *dst, const void *src, int32, int32)
{
	CLUMPEXT(dst)->visibilityCB = CLUMPEXT(src)->visibilityCB;
	return dst;
}

void
CVisibilityPlugins::SetClumpModelInfo(RpClump *clump, CClumpModelInfo *modelInfo)
{
	CVehicleModelInfo *vmi;
	SetFrameHierarchyId(RpClumpGetFrame(clump), (intptr)modelInfo);

	// Unused
	switch (modelInfo->GetModelType()) {
	case MITYPE_MLO:
		CLUMPEXT(clump)->visibilityCB = MloVisibilityCB;
		break;
	case MITYPE_VEHICLE:
		vmi = (CVehicleModelInfo*)modelInfo;
		if (vmi->m_vehicleType == VEHICLE_TYPE_TRAIN ||
			vmi->m_vehicleType == VEHICLE_TYPE_HELI ||
			vmi->m_vehicleType == VEHICLE_TYPE_PLANE)
			CLUMPEXT(clump)->visibilityCB = VehicleVisibilityCB_BigVehicle;
		else
			CLUMPEXT(clump)->visibilityCB = VehicleVisibilityCB;
		break;
	default: break;
	}
}

CClumpModelInfo*
CVisibilityPlugins::GetClumpModelInfo(RpClump *clump)
{
	return (CClumpModelInfo*)GetFrameHierarchyId(RpClumpGetFrame(clump));
}

void
CVisibilityPlugins::SetClumpAlpha(RpClump *clump, int alpha)
{
	CLUMPEXT(clump)->alpha = alpha;
}

int
CVisibilityPlugins::GetClumpAlpha(RpClump *clump)
{
	return CLUMPEXT(clump)->alpha;
}

bool
CVisibilityPlugins::IsClumpVisible(RpClump *clump)
{
	return CLUMPEXT(clump)->visibilityCB(clump);
}
