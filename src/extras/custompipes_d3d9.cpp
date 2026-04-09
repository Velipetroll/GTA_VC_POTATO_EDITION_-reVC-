#define WITHD3D
#include "common.h"

#ifdef RW_D3D9
#include "main.h"
#include "RwHelper.h"
#include "Lights.h"
#include "Timecycle.h"
#include "FileMgr.h"
#include "Clock.h"
#include "Weather.h"
#include "TxdStore.h"
#include "Renderer.h"
#include "World.h"
#include "custompipes.h"

#ifdef EXTENDED_PIPELINES

#ifndef LIBRW
#error "Need librw for EXTENDED_PIPELINES"
#endif

extern RwTexture *gpWhiteTexture;

namespace CustomPipes {

	// ====================================================================
	// LA TUBERÍA BOBA: Ignora MatFX, EnvMaps y shaders pesados. 
	// Dibuja los carros como plástico sólido (máximo rendimiento).
	// ====================================================================
	static void dumbRenderCB(rw::Atomic *atomic, rw::d3d9::InstanceDataHeader *header)
	{
		// Renderizador nativo ultraligero de D3D9. Cero reflejos.
		rw::d3d9::defaultRenderCB_Shader(atomic, header);
	}

	void uploadSpecLights(void) {}

	void CreateVehiclePipe(void)
	{
		rw::d3d9::ObjPipeline *pipe = rw::d3d9::ObjPipeline::create();
		pipe->instanceCB = rw::d3d9::defaultInstanceCB;
		pipe->uninstanceCB = rw::d3d9::defaultUninstanceCB;
		pipe->renderCB = dumbRenderCB;
		vehiclePipe = pipe;
	}

	void DestroyVehiclePipe(void)
	{
		if (vehiclePipe) ((rw::d3d9::ObjPipeline*)vehiclePipe)->destroy();
		vehiclePipe = nil;
	}

	// VACIADO DEL RESTO DE TUBERÍAS
	void worldRenderCB(rw::Atomic *atomic, rw::d3d9::InstanceDataHeader *header) {}
	void CreateWorldPipe(void) { worldPipe = nil; }
	void DestroyWorldPipe(void) {}

	void glossRenderCB(rw::Atomic *atomic, rw::d3d9::InstanceDataHeader *header) {}
	void CreateGlossPipe(void) { glossPipe = nil; }
	void DestroyGlossPipe(void) {}

	void uploadRimData(bool enable) {}
	void rimRenderCB(rw::Atomic *atomic, rw::d3d9::InstanceDataHeader *header) {}
	void rimSkinRenderCB(rw::Atomic *atomic, rw::d3d9::InstanceDataHeader *header) {}
	void CreateRimLightPipes(void) { rimPipe = nil; rimSkinPipe = nil; }
	void DestroyRimLightPipes(void) {}

}

#endif // EXTENDED_PIPELINES

#ifdef NEW_RENDERER
#ifndef LIBRW
#error "Need librw for NEW_PIPELINES"
#endif

// INTACTO: Este bloque se mantiene para que Renderer.cpp dibuje la ciudad
namespace WorldRender
{

	struct BuildingInst
	{
		rw::RawMatrix combinedMat;
		rw::d3d9::InstanceDataHeader *instHeader;
		uint8 fadeAlpha;
		bool lighting;
	};
	BuildingInst blendInsts[3][2000];
	int numBlendInsts[3];

	static RwRGBAReal black;

	static void
		SetMatrix(BuildingInst *building, rw::Matrix *worldMat)
	{
		using namespace rw;
		RawMatrix world, worldview;
		Camera *cam = engine->currentCamera;
		convMatrix(&world, worldMat);
		RawMatrix::mult(&worldview, &world, &cam->devView);
		RawMatrix::mult(&building->combinedMat, &worldview, &cam->devProj);
	}

	static bool
		IsTextureTransparent(RwTexture *tex)
	{
		if (tex == nil || tex->raster == nil)
			return false;
		return PLUGINOFFSET(rw::d3d::D3dRaster, tex->raster, rw::d3d::nativeRasterOffset)->hasAlpha;
	}

	void
		AtomicFirstPass(RpAtomic *atomic, int pass)
	{
		using namespace rw;
		using namespace rw::d3d;
		using namespace rw::d3d9;

		BuildingInst *building = &blendInsts[pass][numBlendInsts[pass]];

		atomic->getPipeline()->instance(atomic);
		building->instHeader = (d3d9::InstanceDataHeader*)atomic->geometry->instData;
		assert(building->instHeader != nil);
		assert(building->instHeader->platform == PLATFORM_D3D9);
		building->fadeAlpha = 255;
		building->lighting = !!(atomic->geometry->flags & rw::Geometry::LIGHT);
		rw::uint32 flags = atomic->geometry->flags;

		bool setupDone = false;
		bool defer = false;
		SetMatrix(building, atomic->getFrame()->getLTM());

		InstanceData *inst = building->instHeader->inst;
		for (rw::uint32 i = 0; i < building->instHeader->numMeshes; i++, inst++) {
			Material *m = inst->material;

			if (inst->vertexAlpha || m->color.alpha != 255 ||
				IsTextureTransparent(m->texture)) {
				defer = true;
				continue;
			}

			if (!setupDone) {
				setStreamSource(0, building->instHeader->vertexStream[0].vertexBuffer, 0, building->instHeader->vertexStream[0].stride);
				setIndices(building->instHeader->indexBuffer);
				setVertexDeclaration(building->instHeader->vertexDeclaration);
				setVertexShader(default_amb_VS);
				d3ddevice->SetVertexShaderConstantF(VSLOC_combined, (float*)&building->combinedMat, 4);
				if (building->lighting)
					setAmbient(pAmbient->color);
				else
					setAmbient(black);
				setupDone = true;
			}

			setMaterial(flags, m->color, m->surfaceProps);

			if (m->texture) {
				d3d::setTexture(0, m->texture);
				setPixelShader(default_tex_PS);
			}
			else
				setPixelShader(default_PS);

			drawInst(building->instHeader, inst);
		}
		if (defer)
			numBlendInsts[pass]++;
	}

	void
		AtomicFullyTransparent(RpAtomic *atomic, int pass, int fadeAlpha)
	{
		using namespace rw;
		using namespace rw::d3d;
		using namespace rw::d3d9;

		BuildingInst *building = &blendInsts[pass][numBlendInsts[pass]];

		atomic->getPipeline()->instance(atomic);
		building->instHeader = (d3d9::InstanceDataHeader*)atomic->geometry->instData;
		assert(building->instHeader != nil);
		assert(building->instHeader->platform == PLATFORM_D3D9);
		building->fadeAlpha = fadeAlpha;
		building->lighting = !!(atomic->geometry->flags & rw::Geometry::LIGHT);
		SetMatrix(building, atomic->getFrame()->getLTM());
		numBlendInsts[pass]++;
	}

	void
		RenderBlendPass(int pass)
	{
		using namespace rw;
		using namespace rw::d3d;
		using namespace rw::d3d9;

		setVertexShader(default_amb_VS);

		int i;
		for (i = 0; i < numBlendInsts[pass]; i++) {
			BuildingInst *building = &blendInsts[pass][i];

			setStreamSource(0, building->instHeader->vertexStream[0].vertexBuffer, 0, building->instHeader->vertexStream[0].stride);
			setIndices(building->instHeader->indexBuffer);
			setVertexDeclaration(building->instHeader->vertexDeclaration);
			d3ddevice->SetVertexShaderConstantF(VSLOC_combined, (float*)&building->combinedMat, 4);
			if (building->lighting)
				setAmbient(pAmbient->color);
			else
				setAmbient(black);

			InstanceData *inst = building->instHeader->inst;
			for (rw::uint32 j = 0; j < building->instHeader->numMeshes; j++, inst++) {
				Material *m = inst->material;
				if (!inst->vertexAlpha && m->color.alpha == 255 && !IsTextureTransparent(m->texture) && building->fadeAlpha == 255)
					continue;

				rw::RGBA color = m->color;
				color.alpha = (color.alpha * building->fadeAlpha) / 255;
				setMaterial(color, m->surfaceProps);

				if (m->texture) {
					d3d::setTexture(0, m->texture);
					setPixelShader(default_tex_PS);
				}
				else
					setPixelShader(default_PS);

				drawInst(building->instHeader, inst);
			}
		}
	}
}
#endif // NEW_RENDERER
#endif // RW_D3D9