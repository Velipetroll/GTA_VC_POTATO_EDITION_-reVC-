#define WITHD3D
#include "common.h"
#include <rpskin.h>

#include "Timecycle.h"
#include "skeleton.h"
#include "Debug.h"
#include "MBlur.h"
#if !defined(FINAL) || defined(DEBUGMENU)
#include "rtcharse.h"
#endif
#ifndef FINAL
RtCharset *debugCharset;
bool bDebugRenderGroups;
#endif

#ifdef PS2_ALPHA_TEST
bool gPS2alphaTest = true;
#else
bool gPS2alphaTest = false;
#endif
bool gBackfaceCulling = true;

#if !defined(FINAL) || defined(DEBUGMENU)
static bool charsetOpen;
void OpenCharsetSafe()
{
	if (!charsetOpen)
		RtCharsetOpen();
	charsetOpen = true;
}
#endif

void CreateDebugFont()
{
#ifndef FINAL
	RwRGBA color = { 255, 255, 128, 255 };
	RwRGBA colorbg = { 0, 0, 0, 0 };
	OpenCharsetSafe();
	debugCharset = RtCharsetCreate(&color, &colorbg);
#endif
}

void DestroyDebugFont()
{
#ifndef FINAL
	RtCharsetDestroy(debugCharset);
	RtCharsetClose();
	charsetOpen = false;
#endif
}

void ObrsPrintfString(const char *str, short x, short y)
{
#ifndef FINAL
	RtCharsetPrintBuffered(debugCharset, str, x * 8, y * 16, true);
#endif
}

void FlushObrsPrintfs()
{
#ifndef FINAL
	RtCharsetBufferFlush();
#endif
}

void
DefinedState(void)
{
	RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
	RwRenderStateSet(rwRENDERSTATETEXTUREPERSPECTIVE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEBORDERCOLOR, (void*)RWRGBALONG(0, 0, 0, 255));

	// --- OPTIMIZACIÓN GMA 3150: Niebla forzada a apagado ---
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEFOGCOLOR, (void*)RWRGBALONG(0, 0, 0, 255));
	RwRenderStateSet(rwRENDERSTATEFOGTYPE, (void*)rwFOGTYPELINEAR);

	RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);

#ifdef LIBRW
	rw::SetRenderState(rw::ALPHATESTFUNC, rw::ALPHAGREATEREQUAL);
	rw::SetRenderState(rw::ALPHATESTREF, 3);
	rw::SetRenderState(rw::GSALPHATEST, gPS2alphaTest);
#elif defined(GTA_PC)
	RwD3D8SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	RwD3D8SetRenderState(D3DRS_ALPHAREF, 2);
#endif
}

void
SetCullMode(uint32 mode)
{
	if (gBackfaceCulling)
		RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)mode);
	else
		RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
}

#ifndef FINAL
void PushRendergroup(const char *name) {}
void PopRendergroup(void) {}
#endif

RwFrame* GetFirstFrameCallback(RwFrame *child, void *data) { *(RwFrame**)data = child; return nil; }
RwFrame* GetFirstChild(RwFrame *frame) { RwFrame *child = nil; RwFrameForAllChildren(frame, GetFirstFrameCallback, &child); return child; }

RwObject* GetFirstObjectCallback(RwObject *object, void *data) { *(RwObject**)data = object; return nil; }
RwObject* GetFirstObject(RwFrame *frame) { RwObject *obj = nil; RwFrameForAllObjects(frame, GetFirstObjectCallback, &obj); return obj; }

RpAtomic* GetFirstAtomicCallback(RpAtomic *atm, void *data) { *(RpAtomic**)data = atm; return nil; }
RpAtomic* GetFirstAtomic(RpClump *clump) { RpAtomic *atm = nil; RpClumpForAllAtomics(clump, GetFirstAtomicCallback, &atm); return atm; }

RwTexture* GetFirstTextureCallback(RwTexture *tex, void *data) { *(RwTexture**)data = tex; return nil; }
RwTexture* GetFirstTexture(RwTexDictionary *txd) { RwTexture *tex = nil; RwTexDictionaryForAllTextures(txd, GetFirstTextureCallback, &tex); return tex; }

bool IsClumpSkinned(RpClump *clump) {
	RpAtomic *atomic = GetFirstAtomic(clump);
	return atomic ? RpSkinGeometryGetSkin(RpAtomicGetGeometry(atomic)) : nil;
}

static RpAtomic* GetAnimHierarchyCallback(RpAtomic *atomic, void *data) {
	*(RpHAnimHierarchy**)data = RpSkinAtomicGetHAnimHierarchy(atomic);
	return nil;
}

RpHAnimHierarchy* GetAnimHierarchyFromSkinClump(RpClump *clump) {
	RpHAnimHierarchy *hier = nil;
	RpClumpForAllAtomics(clump, GetAnimHierarchyCallback, &hier);
	return hier;
}

static RwFrame* GetAnimHierarchyFromClumpCB(RwFrame *frame, void *data) {
	RpHAnimHierarchy *hier = RpHAnimFrameGetHierarchy(frame);
	if (hier) {
		*(RpHAnimHierarchy**)data = hier;
		return nil;
	}
	RwFrameForAllChildren(frame, GetAnimHierarchyFromClumpCB, data);
	return frame;
}

RpHAnimHierarchy* GetAnimHierarchyFromClump(RpClump *clump) {
	RpHAnimHierarchy *hier = nil;
	RwFrameForAllChildren(RpClumpGetFrame(clump), GetAnimHierarchyFromClumpCB, &hier);
	return hier;
}

void
SkinGetBonePositionsToTable(RpClump *clump, RwV3d *boneTable)
{
	int i, parent;
	RpAtomic *atomic;
	RpSkin *skin;
	RpHAnimHierarchy *hier;
	int numBones;
	RwMatrix m, invmat;
	int stack[32];
	int sp;

	if (boneTable == nil)
		return;

	atomic = GetFirstAtomic(clump);
	assert(atomic);
	skin = RpSkinGeometryGetSkin(RpAtomicGetGeometry(atomic));
	assert(skin);
	hier = GetAnimHierarchyFromSkinClump(clump);
	assert(hier);
	boneTable[0].x = 0.0f;
	boneTable[0].y = 0.0f;
	boneTable[0].z = 0.0f;
	numBones = RpSkinGetNumBones(skin);
	parent = 0;
	sp = 0;
#ifdef FIX_BUGS
	stack[0] = 0;
#endif
	for (i = 1; i < numBones; i++) {
		RwMatrixCopy(&m, &RpSkinGetSkinToBoneMatrices(skin)[i]);
		RwMatrixInvert(&invmat, &m);
		const RwMatrix *x = RpSkinGetSkinToBoneMatrices(skin);
		RwV3dTransformPoints(&boneTable[i], &invmat.pos, 1, &x[parent]);
		if (HIERNODEINFO(hier)[i].flags & rpHANIMPUSHPARENTMATRIX)
			stack[++sp] = parent;
		if (HIERNODEINFO(hier)[i].flags & rpHANIMPOPPARENTMATRIX)
			parent = stack[sp--];
		else
			parent = i;
	}
}

RpHAnimAnimation*
HAnimAnimationCreateForHierarchy(RpHAnimHierarchy *hier)
{
	int i;
#if defined FIX_BUGS || defined LIBRW
	int numNodes = hier->numNodes * 2;
#else
	int numNodes = hier->numNodes;
#endif
	RpHAnimAnimation *anim = RpHAnimAnimationCreate(rpHANIMSTDKEYFRAMETYPEID, numNodes, 0, 0.0f);
	if (anim == nil)
		return nil;
	RpHAnimStdKeyFrame *frame;
	for (i = 0; i < numNodes; i++) {
		frame = (RpHAnimStdKeyFrame*)HANIMFRAME(anim, i);
		frame->q.real = 1.0f;
		frame->q.imag.x = frame->q.imag.y = frame->q.imag.z = 0.0f;
		frame->t.x = frame->t.y = frame->t.z = 0.0f;
#if defined FIX_BUGS || defined LIBRW
		frame->time = i / hier->numNodes;
#else
		frame->time = 0.0f;
#endif
		frame->prevFrame = nil;
	}
	return anim;
}

void RenderSkeleton(RpHAnimHierarchy *hier)
{
	// --- OPTIMIZACIÓN CPU: Vaciado ---
	// Función de debug para dibujar huesos. Silenciada para evitar ciclos muertos.
	return;
}

RwBool Im2DRenderQuad(RwReal x1, RwReal y1, RwReal x2, RwReal y2, RwReal z, RwReal recipCamZ, RwReal uvOffset)
{
	RwIm2DVertex vx[4];

	RwIm2DVertexSetScreenX(&vx[0], x1); RwIm2DVertexSetScreenY(&vx[0], y1); RwIm2DVertexSetScreenZ(&vx[0], z);
	RwIm2DVertexSetIntRGBA(&vx[0], 255, 255, 255, 255); RwIm2DVertexSetRecipCameraZ(&vx[0], recipCamZ);
	RwIm2DVertexSetU(&vx[0], uvOffset, recipCamZ); RwIm2DVertexSetV(&vx[0], uvOffset, recipCamZ);

	RwIm2DVertexSetScreenX(&vx[1], x1); RwIm2DVertexSetScreenY(&vx[1], y2); RwIm2DVertexSetScreenZ(&vx[1], z);
	RwIm2DVertexSetIntRGBA(&vx[1], 255, 255, 255, 255); RwIm2DVertexSetRecipCameraZ(&vx[1], recipCamZ);
	RwIm2DVertexSetU(&vx[1], uvOffset, recipCamZ); RwIm2DVertexSetV(&vx[1], 1.0f + uvOffset, recipCamZ);

	RwIm2DVertexSetScreenX(&vx[2], x2); RwIm2DVertexSetScreenY(&vx[2], y1); RwIm2DVertexSetScreenZ(&vx[2], z);
	RwIm2DVertexSetIntRGBA(&vx[2], 255, 255, 255, 255); RwIm2DVertexSetRecipCameraZ(&vx[2], recipCamZ);
	RwIm2DVertexSetU(&vx[2], 1.0f + uvOffset, recipCamZ); RwIm2DVertexSetV(&vx[2], uvOffset, recipCamZ);

	RwIm2DVertexSetScreenX(&vx[3], x2); RwIm2DVertexSetScreenY(&vx[3], y2); RwIm2DVertexSetScreenZ(&vx[3], z);
	RwIm2DVertexSetIntRGBA(&vx[3], 255, 255, 255, 255); RwIm2DVertexSetRecipCameraZ(&vx[3], recipCamZ);
	RwIm2DVertexSetU(&vx[3], 1.0f + uvOffset, recipCamZ); RwIm2DVertexSetV(&vx[3], 1.0f + uvOffset, recipCamZ);

	RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vx, 4);
	return TRUE;
}

bool b_cbsUseLTM = true;

RpAtomic *cbsCalcMeanBSphereRadiusCB(RpAtomic *atomic, void *data)
{
	RwV3d atomicPos;

	if (b_cbsUseLTM)
		RwV3dTransformPoints(&atomicPos, &RpAtomicGetBoundingSphere(atomic)->center, 1, RwFrameGetLTM(RpClumpGetFrame(atomic->clump)));
	else
		RwV3dTransformPoints(&atomicPos, &RpAtomicGetBoundingSphere(atomic)->center, 1, RwFrameGetMatrix(RpClumpGetFrame(atomic->clump)));

	RwV3d temp;
	RwV3dSub(&temp, &atomicPos, &((RwSphere *)data)->center);

	// --- OPTIMIZACIÓN ATOM N450: Bypass Sqrt en radios ---
	// Calculamos el radio de la esfera usando una suma Manhattan barata 
	// en lugar del costoso teorema tridimensional.
	float absX = temp.x < 0.0f ? -temp.x : temp.x;
	float absY = temp.y < 0.0f ? -temp.y : temp.y;
	float absZ = temp.z < 0.0f ? -temp.z : temp.z;
	RwReal radius = (absX + absY + absZ) * 0.6f + RpAtomicGetBoundingSphere(atomic)->radius;

	if (((RwSphere *)data)->radius < radius)
		((RwSphere *)data)->radius = radius;

	return atomic;
}

RpAtomic *cbsCalcMeanBSphereCenterCB(RpAtomic *atomic, void *data)
{
	RwV3d atomicPos;

	if (b_cbsUseLTM)
		RwV3dTransformPoints(&atomicPos, &RpAtomicGetBoundingSphere(atomic)->center, 1, RwFrameGetLTM(RpClumpGetFrame(atomic->clump)));
	else
		RwV3dTransformPoints(&atomicPos, &RpAtomicGetBoundingSphere(atomic)->center, 1, RwFrameGetMatrix(RpClumpGetFrame(atomic->clump)));

	RwV3dAdd(&((RwSphere *)data)->center, &((RwSphere *)data)->center, &atomicPos);
	return atomic;
}

RpClump *RpClumpGetBoundingSphere(RpClump *clump, RwSphere *sphere, bool useLTM)
{
	RwMatrix matrix;
	RwSphere result = { 0.0f, 0.0f, 0.0f, 0.0f };

	b_cbsUseLTM = useLTM;

	if (clump == nil || sphere == nil)
		return nil;

	sphere->radius = 0.0f;
	sphere->center.x = 0.0f;
	sphere->center.y = 0.0f;
	sphere->center.z = 0.0f;

	RwInt32 numAtomics = RpClumpGetNumAtomics(clump);
	if (numAtomics < 1.0f)
		return nil;

	RpClumpForAllAtomics(clump, cbsCalcMeanBSphereCenterCB, &result);
	RwV3dScale(&result.center, &result.center, 1.0f / numAtomics);
	RpClumpForAllAtomics(clump, cbsCalcMeanBSphereRadiusCB, &result);

	if (b_cbsUseLTM)
		RwMatrixInvert(&matrix, RwFrameGetLTM(RpClumpGetFrame(clump)));
	else
		RwMatrixInvert(&matrix, RwFrameGetMatrix(RpClumpGetFrame(clump)));

	RwV3dTransformPoints(&result.center, &result.center, 1, &matrix);
	*sphere = result;

	return clump;
}

void CameraSize(RwCamera * camera, RwRect * rect, RwReal viewWindow, RwReal aspectRatio)
{
	if (camera)
	{
		RwVideoMode videoMode;
		RwRect r;
		RwRect origSize = { 0, 0, 0, 0 };
		RwV2d vw;

		RwEngineGetVideoModeInfo(&videoMode, RwEngineGetCurrentVideoMode());

		origSize.w = RwRasterGetWidth(RwCameraGetRaster(camera));
		origSize.h = RwRasterGetHeight(RwCameraGetRaster(camera));

		if (!rect)
		{
			if (videoMode.flags & rwVIDEOMODEEXCLUSIVE)
			{
				r.x = r.y = 0;
				r.w = videoMode.width;
				r.h = videoMode.height;
				rect = &r;
			}
			else
			{
				r.w = RwRasterGetWidth(RwCameraGetRaster(camera));
				r.h = RwRasterGetHeight(RwCameraGetRaster(camera));
				r.x = r.y = 0;
				rect = &r;
			}
		}

		if ((origSize.w != rect->w) || (origSize.h != rect->h))
		{
			RwRaster *raster;
			RwRaster *zRaster;

#if defined FIX_BUGS || defined LIBRW
			raster = RwCameraGetRaster(camera);
			if (raster) { RwRasterDestroy(raster); camera->frameBuffer = nil; }
			zRaster = RwCameraGetZRaster(camera);
			if (zRaster) { RwRasterDestroy(zRaster); camera->zBuffer = nil; }

			raster = RwRasterCreate(rect->w, rect->h, 0, rwRASTERTYPECAMERA);
			zRaster = RwRasterCreate(rect->w, rect->h, 0, rwRASTERTYPEZBUFFER);

			if (raster && zRaster)
			{
				RwCameraSetRaster(camera, raster);
				RwCameraSetZRaster(camera, zRaster);
			}
			else
			{
				if (raster) RwRasterDestroy(raster);
				if (zRaster) RwRasterDestroy(zRaster);

				rect->x = origSize.x; rect->y = origSize.y;
				rect->w = origSize.w; rect->h = origSize.h;

				raster = RwRasterCreate(rect->w, rect->h, 0, rwRASTERTYPECAMERA);
				zRaster = RwRasterCreate(rect->w, rect->h, 0, rwRASTERTYPEZBUFFER);
				RwCameraSetRaster(camera, raster);
				RwCameraSetZRaster(camera, zRaster);
			}
#else
			raster = RwCameraGetRaster(camera);
			zRaster = RwCameraGetZRaster(camera);
			raster->width = zRaster->width = rect->w;
			raster->height = zRaster->height = rect->h;
#endif
#ifdef FIX_BUGS
			if (CMBlur::BlurOn) {
				CMBlur::MotionBlurClose();
				CMBlur::MotionBlurOpen(camera);
			}
#endif
		}

		if (videoMode.flags & rwVIDEOMODEEXCLUSIVE)
		{
			vw.x = viewWindow;
			vw.y = viewWindow / aspectRatio;
		}
		else
		{
			if (rect->w > rect->h)
			{
				vw.x = viewWindow;
				vw.y = (rect->h * viewWindow) / rect->w;
			}
			else
			{
				vw.x = (rect->w * viewWindow) / rect->h;
				vw.y = viewWindow;
			}
		}

		RwCameraSetViewWindow(camera, &vw);
		RsGlobal.width = rect->w;
		RsGlobal.height = rect->h;
	}
}

void CameraDestroy(RwCamera *camera)
{
	RwRaster *raster, *tmpRaster;
	RwFrame *frame;

	if (camera)
	{
		frame = RwCameraGetFrame(camera);
		if (frame) RwFrameDestroy(frame);

		raster = RwCameraGetRaster(camera);
		if (raster)
		{
			tmpRaster = RwRasterGetParent(raster);
			RwRasterDestroy(raster);
			if ((tmpRaster != nil) && (tmpRaster != raster)) RwRasterDestroy(tmpRaster);
		}

		raster = RwCameraGetZRaster(camera);
		if (raster)
		{
			tmpRaster = RwRasterGetParent(raster);
			RwRasterDestroy(raster);
			if ((tmpRaster != nil) && (tmpRaster != raster)) RwRasterDestroy(tmpRaster);
		}

		RwCameraDestroy(camera);
	}
}

RwCamera* CameraCreate(RwInt32 width, RwInt32 height, RwBool zBuffer)
{
	RwCamera *camera = RwCameraCreate();

	if (camera)
	{
		RwCameraSetFrame(camera, RwFrameCreate());
		RwCameraSetRaster(camera, RwRasterCreate(0, 0, 0, rwRASTERTYPECAMERA));

		if (zBuffer)
			RwCameraSetZRaster(camera, RwRasterCreate(0, 0, 0, rwRASTERTYPEZBUFFER));

		if (RwCameraGetFrame(camera) &&
			RwCameraGetRaster(camera) &&
			RwRasterGetParent(RwCameraGetRaster(camera)) &&
			(!zBuffer || (RwCameraGetZRaster(camera) && RwRasterGetParent(RwCameraGetZRaster(camera)))))
		{
			return (camera);
		}
	}
	CameraDestroy(camera);
	return (nil);
}

#ifdef LIBRW
#include <rpmatfx.h>
#include "VehicleModelInfo.h"

int32 findPlatform(rw::Atomic *a)
{
	rw::Geometry *g = a->geometry;
	if (g->instData) return g->instData->platform;
	return 0;
}

void attachPipe(rw::Atomic *atomic)
{
	if (RpSkinGeometryGetSkin(RpAtomicGetGeometry(atomic)))
		atomic->pipeline = rw::skinGlobals.pipelines[rw::platform];
	else {
		int fx = rpMATFXEFFECTNULL;
		RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), CVehicleModelInfo::GetMatFXEffectMaterialCB, &fx);
		if (fx != rpMATFXEFFECTNULL)
			RpMatFXAtomicEnableEffects(atomic);
	}
}

void switchPipes(rw::Atomic *a, int32 platform)
{
	if (a->pipeline && a->pipeline->platform != platform) {
		uint32 plgid = a->pipeline->pluginID;
		switch (plgid) {
		case rw::ID_SKIN: a->pipeline = rw::skinGlobals.pipelines[platform]; break;
		case rw::ID_MATFX: a->pipeline = rw::matFXGlobals.pipelines[platform]; break;
		}
	}
}

RpAtomic* ConvertPlatformAtomic(RpAtomic *atomic, void *data)
{
	int32 driver = rw::platform;
	int32 platform = findPlatform(atomic);
	if (platform != 0 && platform != driver) {
		attachPipe(atomic);
		rw::ObjPipeline *origPipe = atomic->pipeline;
		rw::platform = platform;
		switchPipes(atomic, rw::platform);
		if (atomic->geometry->flags & rw::Geometry::NATIVE)
			atomic->uninstance();
		rw::platform = driver;
		atomic->pipeline = origPipe;
	}
	return atomic;
}
#endif

#if defined(FIX_BUGS) && defined(GTA_PC)
RwUInt32 saved_alphafunc, saved_alpharef;

void SetAlphaTest(RwUInt32 alpharef)
{
#ifdef LIBRW
	saved_alphafunc = rw::GetRenderState(rw::ALPHATESTFUNC);
	saved_alpharef = rw::GetRenderState(rw::ALPHATESTREF);

	rw::SetRenderState(rw::ALPHATESTFUNC, rw::ALPHAGREATEREQUAL);
	rw::SetRenderState(rw::ALPHATESTREF, 0);
#else
	RwD3D8GetRenderState(D3DRS_ALPHAFUNC, &saved_alphafunc);
	RwD3D8GetRenderState(D3DRS_ALPHAREF, &saved_alpharef);

	RwD3D8SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
	RwD3D8SetRenderState(D3DRS_ALPHAREF, alpharef);
#endif
}

void RestoreAlphaTest()
{
#ifdef LIBRW
	rw::SetRenderState(rw::ALPHATESTFUNC, saved_alphafunc);
	rw::SetRenderState(rw::ALPHATESTREF, saved_alpharef);
#else
	RwD3D8SetRenderState(D3DRS_ALPHAFUNC, saved_alphafunc);
	RwD3D8SetRenderState(D3DRS_ALPHAREF, saved_alpharef);
#endif
}
#endif