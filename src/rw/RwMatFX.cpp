#ifndef LIBRW

#define WITHD3D
#include "common.h"
#include "rpmatfx.h"

struct MatFXNothing { int pad[5]; int effect; };

struct MatFXBump
{
	RwFrame *bumpFrame;
	RwTexture *bumpedTex;
	RwTexture *bumpTex;
	float negBumpCoefficient;
	int pad;
	int effect;
};

struct MatFXEnv
{
	RwFrame *envFrame;
	RwTexture *envTex;
	float envCoeff;
	int envFBalpha;
	int pad;
	int effect;
};

struct MatFXDual
{
	RwTexture *dualTex;
	RwInt32 srcBlend;
	RwInt32 dstBlend;
};

struct MatFX
{
	union {
		MatFXNothing n;
		MatFXBump b;
		MatFXEnv e;
		MatFXDual d;
	} fx[2];
	int effects;
};

extern "C" {
	extern int MatFXMaterialDataOffset;
	extern int MatFXAtomicDataOffset;

	void _rpMatFXD3D8AtomicMatFXEnvRender(RxD3D8InstanceData* inst, int flags, int sel, RwTexture* texture, RwTexture* envMap);
	void _rpMatFXD3D8AtomicMatFXRenderBlack(RxD3D8InstanceData *inst);
	void _rpMatFXD3D8AtomicMatFXBumpMapRender(RxD3D8InstanceData *inst, int flags, RwTexture *texture, RwTexture *bumpMap, RwTexture *envMap);
	void _rpMatFXD3D8AtomicMatFXDualPassRender(RxD3D8InstanceData *inst, int flags, RwTexture *texture, RwTexture *dualTexture);
}


#ifdef PS2_MATFX

// ESTA FUNCIÓN SE QUEDA: Es el render base. Si se vacía, los modelos se vuelven invisibles.
void
_rpMatFXD3D8AtomicMatFXDefaultRender(RxD3D8InstanceData *inst, int flags, RwTexture *texture)
{
	if (flags & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2) && texture)
		RwD3D8SetTexture(texture, 0);
	else
		RwD3D8SetTexture(nil, 0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)(inst->vertexAlpha || inst->material->color.alpha != 0xFF));
	RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha != 0);
	RwD3D8SetPixelShader(0);
	RwD3D8SetVertexShader(inst->vertexShader);
	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);

	if (inst->indexBuffer) {
		RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	}
	else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);
}

// FUNCIONES VACIADAS

void
ApplyEnvMapTextureMatrix(RwTexture *tex, int n, RwFrame *frame)
{
	// VACIADA: Cero cálculos de matrices de reflejo en CPU.
}

void
_rpMatFXD3D8AtomicMatFXEnvRender_ps2(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap)
{
	// VACIADA: Se omite la doble pasada de texturas. Se manda directo al render barato.
	_rpMatFXD3D8AtomicMatFXDefaultRender(inst, flags, texture);
}

void
_rwD3D8EnableClippingIfNeeded(void *object, RwUInt8 type)
{
	// SE QUEDA: El "Culling" le dice a la GPU que no dibuje cosas que no estás viendo. Vital para la GMA 3150.
	int clip;
	if (type == rpATOMIC)
		clip = !RwD3D8CameraIsSphereFullyInsideFrustum(RwCameraGetCurrentCameraMacro(), RpAtomicGetWorldBoundingSphere((RpAtomic *)object));
	else
		clip = !RwD3D8CameraIsBBoxFullyInsideFrustum(RwCameraGetCurrentCameraMacro(), &((RpWorldSector *)object)->tightBoundingBox);
	RwD3D8SetRenderState(D3DRS_CLIPPING, clip);
}

void
_rwD3D8AtomicMatFXRenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RwBool lighting;
	RwBool forceBlack;
	RxD3D8ResEntryHeader *header;
	RxD3D8InstanceData *inst;
	RwInt32 i;

	if (flags & rpGEOMETRYPRELIT) {
		RwD3D8SetRenderState(D3DRS_COLORVERTEX, 1);
		RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1);
	}
	else {
		RwD3D8SetRenderState(D3DRS_COLORVERTEX, 0);
		RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
	}

	_rwD3D8EnableClippingIfNeeded(object, type);

	RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);
	if (lighting || flags & rpGEOMETRYPRELIT) {
		forceBlack = FALSE;
	}
	else {
		forceBlack = TRUE;
		RwD3D8SetTexture(nil, 0);
		RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA(0, 0, 0, 255));
		RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
		RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
	}

	header = (RxD3D8ResEntryHeader *)(repEntry + 1);
	inst = (RxD3D8InstanceData *)(header + 1);
	for (i = 0; i < header->numMeshes; i++) {
		if (forceBlack)
			_rpMatFXD3D8AtomicMatFXRenderBlack(inst);
		else {
			if (lighting)
				RwD3D8SetSurfaceProperties(&inst->material->color, &inst->material->surfaceProps, flags & rpGEOMETRYMODULATEMATERIALCOLOR);

			// BYPASS TOTAL MATFX: Ignoramos el switch de materiales y forzamos renderización estándar.
			_rpMatFXD3D8AtomicMatFXDefaultRender(inst, flags, inst->material->texture);
		}
		inst++;
	}

	if (forceBlack) {
		RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
		RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	}
}

void
ReplaceMatFxCallback()
{
	RxD3D8AllInOneSetRenderCallBack(
		RxPipelineFindNodeByName(RpMatFXGetD3D8Pipeline(rpMATFXD3D8ATOMICPIPELINE), RxNodeDefinitionGetD3D8AtomicAllInOne()->name, nil, nil),
		_rwD3D8AtomicMatFXRenderCallback);

}
#endif // PS2_MATFX

#endif // !LIBRW