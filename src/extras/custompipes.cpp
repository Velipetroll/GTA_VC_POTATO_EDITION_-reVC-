#define WITHD3D
#include "common.h"

#ifdef EXTENDED_PIPELINES

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

#ifndef LIBRW
#error "Need librw for EXTENDED_PIPELINES"
#endif

namespace CustomPipes {

	rw::int32 CustomMatOffset;

	void*
		CustomMatCtor(void *object, int32, int32)
	{
		CustomMatExt *ext = GetCustomMatExt((rw::Material*)object);
		ext->glossTex = nil;
		ext->haveGloss = false;
		return object;
	}

	void*
		CustomMatCopy(void *dst, void *src, int32, int32)
	{
		CustomMatExt *srcext = GetCustomMatExt((rw::Material*)src);
		CustomMatExt *dstext = GetCustomMatExt((rw::Material*)dst);
		dstext->glossTex = srcext->glossTex;
		dstext->haveGloss = srcext->haveGloss;
		return dst;
	}

	rw::TexDictionary *neoTxd = nil;

	bool bRenderingEnvMap = false;
	int32 EnvMapSize = 128;
	rw::Camera *EnvMapCam = nil;
	rw::Texture *EnvMapTex = nil;
	rw::Texture *EnvMaskTex = nil;

	static rw::Camera* CreateEnvMapCam(rw::World *world) { return nil; }
	static void DestroyCam(rw::Camera *cam) {}

	// VACIADO
	void RenderEnvMapScene(void) {}
	void EnvMapRender(void) {}
	static void EnvMapInit(void) {}
	static void EnvMapShutdown(void) {}

	// Tweaks Dummy
#define INTERP_SETUP \
		int h1 = CClock::GetHours();								  \
		int h2 = (h1+1)%24;										  \
		int w1 = CWeather::OldWeatherType;								  \
		int w2 = CWeather::NewWeatherType;								  \
		float timeInterp = (CClock::GetSeconds()/60.0f + CClock::GetMinutes())/60.0f;	  \
		float c0 = (1.0f-timeInterp)*(1.0f-CWeather::InterpolationValue);				  \
		float c1 = timeInterp*(1.0f-CWeather::InterpolationValue);					  \
		float c2 = (1.0f-timeInterp)*CWeather::InterpolationValue;					  \
		float c3 = timeInterp*CWeather::InterpolationValue;
#define INTERP(v) v[h1][w1]*c0 + v[h2][w1]*c1 + v[h1][w2]*c2 + v[h2][w2]*c3;
#define INTERPF(v,f) v[h1][w1].f*c0 + v[h2][w1].f*c1 + v[h1][w2].f*c2 + v[h2][w2].f*c3;

	InterpolatedFloat::InterpolatedFloat(float init) {
		curInterpolator = 61;
		for (int h = 0; h < 24; h++)
			for (int w = 0; w < NUMWEATHERS; w++)
				data[h][w] = init;
	}
	void InterpolatedFloat::Read(char *s, int line, int field) { sscanf(s, "%f", &data[line][field]); }
	float InterpolatedFloat::Get(void) {
		if (curInterpolator != CClock::GetSeconds()) {
			INTERP_SETUP
				curVal = INTERP(data);
			curInterpolator = CClock::GetSeconds();
		}
		return curVal;
	}

	InterpolatedColor::InterpolatedColor(const Color &init) {
		curInterpolator = 61;
		for (int h = 0; h < 24; h++)
			for (int w = 0; w < NUMWEATHERS; w++)
				data[h][w] = init;
	}
	void InterpolatedColor::Read(char *s, int line, int field) {
		int r, g, b, a;
		sscanf(s, "%i, %i, %i, %i", &r, &g, &b, &a);
		data[line][field] = Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
	}
	Color InterpolatedColor::Get(void) {
		if (curInterpolator != CClock::GetSeconds()) {
			INTERP_SETUP
				curVal.r = INTERPF(data, r);
			curVal.g = INTERPF(data, g);
			curVal.b = INTERPF(data, b);
			curVal.a = INTERPF(data, a);
			curInterpolator = CClock::GetSeconds();
		}
		return curVal;
	}

	void InterpolatedLight::Read(char *s, int line, int field) {
		int r, g, b, a;
		sscanf(s, "%i, %i, %i, %i", &r, &g, &b, &a);
		data[line][field] = Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 100.0f);
	}

	char* ReadTweakValueTable(char *fp, InterpolatedValue &interp) {
		char buf[24], *p;
		int c;
		int line, field;
		line = 0;
		c = *fp++;
		while (c != '\0' && line < 24) {
			field = 0;
			if (c != '\0' && c != '#') {
				while (c != '\0' && c != '\n' && field < NUMWEATHERS) {
					p = buf;
					while (c != '\0' && c == '\t') c = *fp++;
					*p++ = c;
					while (c = *fp++, c != '\0' && c != '\t' && c != '\n') *p++ = c;
					*p++ = '\0';
					interp.Read(buf, line, field);
					field++;
				}
				line++;
			}
			while (c != '\0' && c != '\n') c = *fp++;
			c = *fp++;
		}
		return fp - 1;
	}

	/*
	* Neo Vehicle pipe variables (Variables estables)
	*/
	int32 VehiclePipeSwitch = 0;
	float VehicleShininess = 1.0f;
	float VehicleSpecularity = 1.0f;
	InterpolatedFloat Fresnel(0.4f);
	InterpolatedFloat Power(18.0f);
	InterpolatedLight DiffColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
	InterpolatedLight SpecColor(Color(0.7f, 0.7f, 0.7f, 1.0f));
	rw::ObjPipeline *vehiclePipe = nil;

	// ====================================================================
	// INTERCEPTOR DE REFLEJOS
	// Sobrescribimos el MatFX de los modelos base para forzar nuestro
	// pipeline bobo ultraligero que armamos en custompipes_d3d9.cpp.
	// ====================================================================
	void AttachVehiclePipe(rw::Atomic *atomic)
	{
		if (vehiclePipe) atomic->pipeline = vehiclePipe;
	}

	void AttachVehiclePipe(rw::Clump *clump)
	{
		if (vehiclePipe) {
			FORLIST(lnk, clump->atomics)
				AttachVehiclePipe(rw::Atomic::fromClump(lnk));
		}
	}

	/*
	* Neo World pipe
	*/
	bool LightmapEnable = false;
	float LightmapMult = 1.0f;
	InterpolatedFloat WorldLightmapBlend(1.0f);
	rw::ObjPipeline *worldPipe = nil;
	void AttachWorldPipe(rw::Atomic *atomic) {}
	void AttachWorldPipe(rw::Clump *clump) {}

	/*
	* Neo Gloss pipe
	*/
	bool GlossEnable = false;
	float GlossMult = 1.0f;
	rw::ObjPipeline *glossPipe = nil;
	rw::Texture* GetGlossTex(rw::Material *mat) { return nil; }
	void AttachGlossPipe(rw::Atomic *atomic) {}
	void AttachGlossPipe(rw::Clump *clump) {}

	/*
	* Neo Rim pipes
	*/
	bool RimlightEnable = false;
	float RimlightMult = 1.0f;
	InterpolatedColor RampStart(Color(0.0f, 0.0f, 0.0f, 1.0f));
	InterpolatedColor RampEnd(Color(1.0f, 1.0f, 1.0f, 1.0f));
	InterpolatedFloat Offset(0.5f);
	InterpolatedFloat Scale(1.5f);
	InterpolatedFloat Scaling(2.0f);
	rw::ObjPipeline *rimPipe = nil;
	rw::ObjPipeline *rimSkinPipe = nil;
	void AttachRimPipe(rw::Atomic *atomic) {}
	void AttachRimPipe(rw::Clump *clump) {}

	/*
	* High level stuff
	*/

	void
		CustomPipeInit(void)
	{
		// Inicializamos SÓLO la Tubería Boba para autos
		CreateVehiclePipe();
	}

	void
		CustomPipeShutdown(void)
	{
		DestroyVehiclePipe();
	}

	void
		CustomPipeRegister(void)
	{
#ifdef RW_OPENGL
		CustomPipeRegisterGL();
#endif
		CustomMatOffset = rw::Material::registerPlugin(sizeof(CustomMatExt), MAKECHUNKID(rwVENDORID_ROCKSTAR, 0x80),
			CustomMatCtor, nil, CustomMatCopy);
	}

	// Load textures from generic as fallback
	rw::TexDictionary *genericTxd;
	rw::Texture *(*defaultFindCB)(const char *name);

	static rw::Texture*
		customFindCB(const char *name)
	{
		rw::Texture *res = defaultFindCB(name);
		if (res == nil && genericTxd)
			res = genericTxd->find(name);
		return res;
	}

	void
		SetTxdFindCallback(void)
	{
		int slot = CTxdStore::FindTxdSlot("generic");
		CTxdStore::AddRef(slot);
		genericTxd = CTxdStore::GetSlot(slot)->texDict;
		assert(genericTxd);
		if (defaultFindCB == nil)
			defaultFindCB = rw::Texture::findCB;
		rw::Texture::findCB = customFindCB;
	}

}

#endif