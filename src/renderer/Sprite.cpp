#include "common.h"

#include "main.h"
#include "Draw.h"
#include "Camera.h"
#include "Sprite.h"

#ifdef ASPECT_RATIO_SCALE
#include "Frontend.h"
#endif

float CSprite::m_f2DNearScreenZ;
float CSprite::m_f2DFarScreenZ;
float CSprite::m_fRecipNearClipPlane;
int32 CSprite::m_bFlushSpriteBufferSwitchZTest;

float
CSprite::CalcHorizonCoors(void)
{
	CVector p = TheCamera.GetPosition() + CVector(TheCamera.CamFrontXNorm, TheCamera.CamFrontYNorm, 0.0f)*3000.0f;
	p.z = 0.0f;
	p = TheCamera.m_viewMatrix * p;
	return p.y * SCREEN_HEIGHT / p.z;
}

bool
CSprite::CalcScreenCoors(const RwV3d &in, RwV3d *out, float *outw, float *outh, bool farclip)
{
	CVector viewvec = TheCamera.m_viewMatrix * in;
	*out = viewvec;
	if (out->z <= CDraw::GetNearClipZ() + 1.0f) return false;
	if (out->z >= CDraw::GetFarClipZ() && farclip) return false;
	float recip = 1.0f / out->z;
	out->x *= SCREEN_WIDTH * recip;
	out->y *= SCREEN_HEIGHT * recip;
	const float fov = DefaultFOV;
	float fovScale = fov / CDraw::GetFOV();

#ifdef FIX_SPRITES
	*outw = CDraw::ms_bFixSprites ? (fovScale * recip * SCREEN_HEIGHT) : (fovScale * SCREEN_SCALE_AR(recip) * SCREEN_WIDTH);
#else
	*outw = fovScale * SCREEN_SCALE_AR(recip) * SCREEN_WIDTH;
#endif
	*outh = fovScale * recip * SCREEN_HEIGHT;

	return true;
}

#define SPRITEBUFFERSIZE 64
static int32 nSpriteBufferIndex;
static RwIm2DVertex SpriteBufferVerts[SPRITEBUFFERSIZE * 6];
static RwIm2DVertex verts[4];

void
CSprite::InitSpriteBuffer(void)
{
	m_f2DNearScreenZ = RwIm2DGetNearScreenZ();
	m_f2DFarScreenZ = RwIm2DGetFarScreenZ();
}

void
CSprite::InitSpriteBuffer2D(void)
{
	m_fRecipNearClipPlane = 1.0f / RwCameraGetNearClipPlane(Scene.camera);
	InitSpriteBuffer();
}

void
CSprite::FlushSpriteBuffer(void)
{
	if (nSpriteBufferIndex > 0) {
		if (m_bFlushSpriteBufferSwitchZTest) {
			RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
			RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, SpriteBufferVerts, nSpriteBufferIndex * 6);
			RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
		}
		else
			RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, SpriteBufferVerts, nSpriteBufferIndex * 6);
		nSpriteBufferIndex = 0;
	}
}

void
CSprite::RenderOneXLUSprite(float x, float y, float z, float w, float h, uint8 r, uint8 g, uint8 b, int16 intens, float recipz, uint8 a)
{
	float xs[4];
	float ys[4];
	float us[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	float vs[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
	int i;

	xs[0] = x - w;
	xs[1] = x - w;
	xs[2] = x + w;
	xs[3] = x + w;

	ys[0] = y - h;
	ys[1] = y + h;
	ys[2] = y + h;
	ys[3] = y - h;

	// CPU Software Clipping eliminado. Delegado al hardware de la GPU para ahorrar ciclos.

	float screenz = m_f2DNearScreenZ +
		(z - CDraw::GetNearClipZ())*(m_f2DFarScreenZ - m_f2DNearScreenZ)*CDraw::GetFarClipZ() /
		((CDraw::GetFarClipZ() - CDraw::GetNearClipZ())*z);

	// Precálculo de color
	uint8 fR = (r * intens) >> 8;
	uint8 fG = (g * intens) >> 8;
	uint8 fB = (b * intens) >> 8;

	for (i = 0; i < 4; i++) {
		RwIm2DVertexSetScreenX(&verts[i], xs[i]);
		RwIm2DVertexSetScreenY(&verts[i], ys[i]);
		RwIm2DVertexSetScreenZ(&verts[i], screenz);
		RwIm2DVertexSetCameraZ(&verts[i], z);
		RwIm2DVertexSetRecipCameraZ(&verts[i], recipz);
		RwIm2DVertexSetIntRGBA(&verts[i], fR, fG, fB, a);
		RwIm2DVertexSetU(&verts[i], us[i], recipz);
		RwIm2DVertexSetV(&verts[i], vs[i], recipz);
	}
	RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, verts, 4);
}

void
CSprite::RenderOneXLUSprite_Rotate_Aspect(float x, float y, float z, float w, float h, uint8 r, uint8 g, uint8 b, int16 intens, float recipz, float rotation, uint8 a)
{
	float c = Cos(rotation);
	float s = Sin(rotation);

	float xs[4];
	float ys[4];
	float us[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	float vs[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
	int i;

	if (z < 2.3f) {
		if (z < 1.3f) return;
		int f = (z - 1.3f) / (2.3f - 1.3f) * 255;
		r = f*r >> 8;
		g = f*g >> 8;
		b = f*b >> 8;
		intens = f*intens >> 8;
	}

	xs[0] = x + w*(-c - s);
	xs[1] = x + w*(-c + s);
	xs[2] = x + w*(+c + s);
	xs[3] = x + w*(+c - s);

	ys[0] = y + h*(-c + s);
	ys[1] = y + h*(+c + s);
	ys[2] = y + h*(+c - s);
	ys[3] = y + h*(-c - s);

	if (xs[0] < 0.0f && xs[1] < 0.0f && xs[2] < 0.0f && xs[3] < 0.0f) return;
	if (ys[0] < 0.0f && ys[1] < 0.0f && ys[2] < 0.0f && ys[3] < 0.0f) return;
	if (xs[0] > SCREEN_WIDTH && xs[1] > SCREEN_WIDTH && xs[2] > SCREEN_WIDTH && xs[3] > SCREEN_WIDTH) return;
	if (ys[0] > SCREEN_HEIGHT && ys[1] > SCREEN_HEIGHT && ys[2] > SCREEN_HEIGHT && ys[3] > SCREEN_HEIGHT) return;

	float screenz = m_f2DNearScreenZ +
		(z - CDraw::GetNearClipZ())*(m_f2DFarScreenZ - m_f2DNearScreenZ)*CDraw::GetFarClipZ() /
		((CDraw::GetFarClipZ() - CDraw::GetNearClipZ())*z);

	uint8 fR = (r * intens) >> 8;
	uint8 fG = (g * intens) >> 8;
	uint8 fB = (b * intens) >> 8;

	for (i = 0; i < 4; i++) {
		RwIm2DVertexSetScreenX(&verts[i], xs[i]);
		RwIm2DVertexSetScreenY(&verts[i], ys[i]);
		RwIm2DVertexSetScreenZ(&verts[i], screenz);
		RwIm2DVertexSetCameraZ(&verts[i], z);
		RwIm2DVertexSetRecipCameraZ(&verts[i], recipz);
		RwIm2DVertexSetIntRGBA(&verts[i], fR, fG, fB, a);
		RwIm2DVertexSetU(&verts[i], us[i], recipz);
		RwIm2DVertexSetV(&verts[i], vs[i], recipz);
	}
	RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, verts, 4);
}

void
CSprite::RenderBufferedOneXLUSprite(float x, float y, float z, float w, float h, uint8 r, uint8 g, uint8 b, int16 intens, float recipz, uint8 a)
{
	m_bFlushSpriteBufferSwitchZTest = 0;

	float xs[4];
	float ys[4];
	float us[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	float vs[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
	int i;

	xs[0] = x - w;
	xs[1] = x - w;
	xs[2] = x + w;
	xs[3] = x + w;

	ys[0] = y - h;
	ys[1] = y + h;
	ys[2] = y + h;
	ys[3] = y - h;

	// CPU Software Clipping eliminado

	float screenz = m_f2DNearScreenZ +
		(z - CDraw::GetNearClipZ())*(m_f2DFarScreenZ - m_f2DNearScreenZ)*CDraw::GetFarClipZ() /
		((CDraw::GetFarClipZ() - CDraw::GetNearClipZ())*z);

	uint8 fR = (r * intens) >> 8;
	uint8 fG = (g * intens) >> 8;
	uint8 fB = (b * intens) >> 8;

	RwIm2DVertex *vert = &SpriteBufferVerts[nSpriteBufferIndex * 6];
	static int indices[6] = { 0, 1, 2, 3, 0, 2 };
	for (i = 0; i < 6; i++) {
		RwIm2DVertexSetScreenX(&vert[i], xs[indices[i]]);
		RwIm2DVertexSetScreenY(&vert[i], ys[indices[i]]);
		RwIm2DVertexSetScreenZ(&vert[i], screenz);
		RwIm2DVertexSetCameraZ(&vert[i], z);
		RwIm2DVertexSetRecipCameraZ(&vert[i], recipz);
		RwIm2DVertexSetIntRGBA(&vert[i], fR, fG, fB, a);
		RwIm2DVertexSetU(&vert[i], us[indices[i]], recipz);
		RwIm2DVertexSetV(&vert[i], vs[indices[i]], recipz);
	}
	nSpriteBufferIndex++;
	if (nSpriteBufferIndex >= SPRITEBUFFERSIZE)
		FlushSpriteBuffer();
}

void
CSprite::RenderBufferedOneXLUSprite_Rotate_Dimension(float x, float y, float z, float w, float h, uint8 r, uint8 g, uint8 b, int16 intens, float recipz, float rotation, uint8 a)
{
	m_bFlushSpriteBufferSwitchZTest = 0;
	float c = Cos(rotation);
	float s = Sin(rotation);

	float xs[4];
	float ys[4];
	float us[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	float vs[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
	int i;

	xs[0] = x - c*w - s*h;
	xs[1] = x - c*w + s*h;
	xs[2] = x + c*w + s*h;
	xs[3] = x + c*w - s*h;

	ys[0] = y - c*h + s*w;
	ys[1] = y + c*h + s*w;
	ys[2] = y + c*h - s*w;
	ys[3] = y - c*h - s*w;

	if (xs[0] < 0.0f && xs[1] < 0.0f && xs[2] < 0.0f && xs[3] < 0.0f) return;
	if (ys[0] < 0.0f && ys[1] < 0.0f && ys[2] < 0.0f && ys[3] < 0.0f) return;
	if (xs[0] > SCREEN_WIDTH && xs[1] > SCREEN_WIDTH && xs[2] > SCREEN_WIDTH && xs[3] > SCREEN_WIDTH) return;
	if (ys[0] > SCREEN_HEIGHT && ys[1] > SCREEN_HEIGHT && ys[2] > SCREEN_HEIGHT && ys[3] > SCREEN_HEIGHT) return;

	float screenz = m_f2DNearScreenZ +
		(z - CDraw::GetNearClipZ())*(m_f2DFarScreenZ - m_f2DNearScreenZ)*CDraw::GetFarClipZ() /
		((CDraw::GetFarClipZ() - CDraw::GetNearClipZ())*z);

	uint8 fR = (r * intens) >> 8;
	uint8 fG = (g * intens) >> 8;
	uint8 fB = (b * intens) >> 8;

	RwIm2DVertex *vert = &SpriteBufferVerts[nSpriteBufferIndex * 6];
	static int indices[6] = { 0, 1, 2, 3, 0, 2 };
	for (i = 0; i < 6; i++) {
		RwIm2DVertexSetScreenX(&vert[i], xs[indices[i]]);
		RwIm2DVertexSetScreenY(&vert[i], ys[indices[i]]);
		RwIm2DVertexSetScreenZ(&vert[i], screenz);
		RwIm2DVertexSetCameraZ(&vert[i], z);
		RwIm2DVertexSetRecipCameraZ(&vert[i], recipz);
		RwIm2DVertexSetIntRGBA(&vert[i], fR, fG, fB, a);
		RwIm2DVertexSetU(&vert[i], us[indices[i]], recipz);
		RwIm2DVertexSetV(&vert[i], vs[indices[i]], recipz);
	}
	nSpriteBufferIndex++;
	if (nSpriteBufferIndex >= SPRITEBUFFERSIZE)
		FlushSpriteBuffer();
}

void
CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect(float x, float y, float z, float w, float h, uint8 r, uint8 g, uint8 b, int16 intens, float recipz, float rotation, uint8 a)
{
	m_bFlushSpriteBufferSwitchZTest = 0;
	float c = Cos(rotation);
	float s = Sin(rotation);

	float xs[4];
	float ys[4];
	float us[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	float vs[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
	int i;

	xs[0] = x + w*(-c - s);
	xs[1] = x + w*(-c + s);
	xs[2] = x + w*(+c + s);
	xs[3] = x + w*(+c - s);

	ys[0] = y + h*(-c + s);
	ys[1] = y + h*(+c + s);
	ys[2] = y + h*(+c - s);
	ys[3] = y + h*(-c - s);

	if (xs[0] < 0.0f && xs[1] < 0.0f && xs[2] < 0.0f && xs[3] < 0.0f) return;
	if (ys[0] < 0.0f && ys[1] < 0.0f && ys[2] < 0.0f && ys[3] < 0.0f) return;
	if (xs[0] > SCREEN_WIDTH && xs[1] > SCREEN_WIDTH && xs[2] > SCREEN_WIDTH && xs[3] > SCREEN_WIDTH) return;
	if (ys[0] > SCREEN_HEIGHT && ys[1] > SCREEN_HEIGHT && ys[2] > SCREEN_HEIGHT && ys[3] > SCREEN_HEIGHT) return;

	float screenz = m_f2DNearScreenZ +
		(z - CDraw::GetNearClipZ())*(m_f2DFarScreenZ - m_f2DNearScreenZ)*CDraw::GetFarClipZ() /
		((CDraw::GetFarClipZ() - CDraw::GetNearClipZ())*z);

	uint8 fR = (r * intens) >> 8;
	uint8 fG = (g * intens) >> 8;
	uint8 fB = (b * intens) >> 8;

	RwIm2DVertex *vert = &SpriteBufferVerts[nSpriteBufferIndex * 6];
	static int indices[6] = { 0, 1, 2, 3, 0, 2 };
	for (i = 0; i < 6; i++) {
		RwIm2DVertexSetScreenX(&vert[i], xs[indices[i]]);
		RwIm2DVertexSetScreenY(&vert[i], ys[indices[i]]);
		RwIm2DVertexSetScreenZ(&vert[i], screenz);
		RwIm2DVertexSetCameraZ(&vert[i], z);
		RwIm2DVertexSetRecipCameraZ(&vert[i], recipz);
		RwIm2DVertexSetIntRGBA(&vert[i], fR, fG, fB, a);
		RwIm2DVertexSetU(&vert[i], us[indices[i]], recipz);
		RwIm2DVertexSetV(&vert[i], vs[indices[i]], recipz);
	}
	nSpriteBufferIndex++;
	if (nSpriteBufferIndex >= SPRITEBUFFERSIZE)
		FlushSpriteBuffer();
}

void
CSprite::RenderBufferedOneXLUSprite_Rotate_2Colours(float x, float y, float z, float w, float h, uint8 r1, uint8 g1, uint8 b1, uint8 r2, uint8 g2, uint8 b2, float cx, float cy, float recipz, float rotation, uint8 a)
{
	m_bFlushSpriteBufferSwitchZTest = 0;
	float c = Cos(rotation);
	float s = Sin(rotation);

	float xs[4];
	float ys[4];
	float us[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	float vs[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
	int i;

	xs[0] = x + w*(-c - s);
	xs[1] = x + w*(-c + s);
	xs[2] = x + w*(+c + s);
	xs[3] = x + w*(+c - s);

	ys[0] = y + h*(-c + s);
	ys[1] = y + h*(+c + s);
	ys[2] = y + h*(+c - s);
	ys[3] = y + h*(-c - s);

	if (xs[0] < 0.0f && xs[1] < 0.0f && xs[2] < 0.0f && xs[3] < 0.0f) return;
	if (ys[0] < 0.0f && ys[1] < 0.0f && ys[2] < 0.0f && ys[3] < 0.0f) return;
	if (xs[0] > SCREEN_WIDTH && xs[1] > SCREEN_WIDTH && xs[2] > SCREEN_WIDTH && xs[3] > SCREEN_WIDTH) return;
	if (ys[0] > SCREEN_HEIGHT && ys[1] > SCREEN_HEIGHT && ys[2] > SCREEN_HEIGHT && ys[3] > SCREEN_HEIGHT) return;

	float screenz = m_f2DNearScreenZ +
		(z - CDraw::GetNearClipZ())*(m_f2DFarScreenZ - m_f2DNearScreenZ)*CDraw::GetFarClipZ() /
		((CDraw::GetFarClipZ() - CDraw::GetNearClipZ())*z);

	RwIm2DVertex *vert = &SpriteBufferVerts[nSpriteBufferIndex * 6];
	static int indices[6] = { 0, 1, 2, 3, 0, 2 };
	for (i = 0; i < 6; i++) {
		RwIm2DVertexSetScreenX(&vert[i], xs[indices[i]]);
		RwIm2DVertexSetScreenY(&vert[i], ys[indices[i]]);
		RwIm2DVertexSetScreenZ(&vert[i], screenz);
		RwIm2DVertexSetCameraZ(&vert[i], z);
		RwIm2DVertexSetRecipCameraZ(&vert[i], recipz);
		// CPU Math Optimization: Aniquilada la interpolación compleja de dos colores,
		// se usa color sólido de la luz primaria.
		RwIm2DVertexSetIntRGBA(&vert[i], r1, g1, b1, a);
		RwIm2DVertexSetU(&vert[i], us[indices[i]], recipz);
		RwIm2DVertexSetV(&vert[i], vs[indices[i]], recipz);
	}
	nSpriteBufferIndex++;
	if (nSpriteBufferIndex >= SPRITEBUFFERSIZE)
		FlushSpriteBuffer();
}

void
CSprite::Set6Vertices2D(RwIm2DVertex *verts, const CRect &r, const CRGBA &c0, const CRGBA &c1, const CRGBA &c2, const CRGBA &c3)
{
	float screenz, recipz;
	float z = RwCameraGetNearClipPlane(Scene.camera);

	screenz = m_f2DNearScreenZ;
	recipz = m_fRecipNearClipPlane;

	RwIm2DVertexSetScreenX(&verts[0], r.left);
	RwIm2DVertexSetScreenY(&verts[0], r.top);
	RwIm2DVertexSetScreenZ(&verts[0], screenz);
	RwIm2DVertexSetCameraZ(&verts[0], z);
	RwIm2DVertexSetRecipCameraZ(&verts[0], recipz);
	RwIm2DVertexSetIntRGBA(&verts[0], c2.r, c2.g, c2.b, c2.a);
	RwIm2DVertexSetU(&verts[0], 0.0f, recipz);
	RwIm2DVertexSetV(&verts[0], 0.0f, recipz);

	RwIm2DVertexSetScreenX(&verts[1], r.right);
	RwIm2DVertexSetScreenY(&verts[1], r.top);
	RwIm2DVertexSetScreenZ(&verts[1], screenz);
	RwIm2DVertexSetCameraZ(&verts[1], z);
	RwIm2DVertexSetRecipCameraZ(&verts[1], recipz);
	RwIm2DVertexSetIntRGBA(&verts[1], c3.r, c3.g, c3.b, c3.a);
	RwIm2DVertexSetU(&verts[1], 1.0f, recipz);
	RwIm2DVertexSetV(&verts[1], 0.0f, recipz);

	RwIm2DVertexSetScreenX(&verts[2], r.right);
	RwIm2DVertexSetScreenY(&verts[2], r.bottom);
	RwIm2DVertexSetScreenZ(&verts[2], screenz);
	RwIm2DVertexSetCameraZ(&verts[2], z);
	RwIm2DVertexSetRecipCameraZ(&verts[2], recipz);
	RwIm2DVertexSetIntRGBA(&verts[2], c1.r, c1.g, c1.b, c1.a);
	RwIm2DVertexSetU(&verts[2], 1.0f, recipz);
	RwIm2DVertexSetV(&verts[2], 1.0f, recipz);

	RwIm2DVertexSetScreenX(&verts[3], r.left);
	RwIm2DVertexSetScreenY(&verts[3], r.bottom);
	RwIm2DVertexSetScreenZ(&verts[3], screenz);
	RwIm2DVertexSetCameraZ(&verts[3], z);
	RwIm2DVertexSetRecipCameraZ(&verts[3], recipz);
	RwIm2DVertexSetIntRGBA(&verts[3], c0.r, c0.g, c0.b, c0.a);
	RwIm2DVertexSetU(&verts[3], 0.0f, recipz);
	RwIm2DVertexSetV(&verts[3], 1.0f, recipz);

	RwIm2DVertexSetScreenX(&verts[4], r.left);
	RwIm2DVertexSetScreenY(&verts[4], r.top);
	RwIm2DVertexSetScreenZ(&verts[4], screenz);
	RwIm2DVertexSetCameraZ(&verts[4], z);
	RwIm2DVertexSetRecipCameraZ(&verts[4], recipz);
	RwIm2DVertexSetIntRGBA(&verts[4], c2.r, c2.g, c2.b, c2.a);
	RwIm2DVertexSetU(&verts[4], 0.0f, recipz);
	RwIm2DVertexSetV(&verts[4], 0.0f, recipz);

	RwIm2DVertexSetScreenX(&verts[5], r.right);
	RwIm2DVertexSetScreenY(&verts[5], r.bottom);
	RwIm2DVertexSetScreenZ(&verts[5], screenz);
	RwIm2DVertexSetCameraZ(&verts[5], z);
	RwIm2DVertexSetRecipCameraZ(&verts[5], recipz);
	RwIm2DVertexSetIntRGBA(&verts[5], c1.r, c1.g, c1.b, c1.a);
	RwIm2DVertexSetU(&verts[5], 1.0f, recipz);
	RwIm2DVertexSetV(&verts[5], 1.0f, recipz);
}

void
CSprite::Set6Vertices2D(RwIm2DVertex *verts, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4,
	const CRGBA &c0, const CRGBA &c1, const CRGBA &c2, const CRGBA &c3)
{
	float screenz, recipz;
	float z = RwCameraGetNearClipPlane(Scene.camera);

	screenz = m_f2DNearScreenZ;
	recipz = m_fRecipNearClipPlane;

	RwIm2DVertexSetScreenX(&verts[0], x3);
	RwIm2DVertexSetScreenY(&verts[0], y3);
	RwIm2DVertexSetScreenZ(&verts[0], screenz);
	RwIm2DVertexSetCameraZ(&verts[0], z);
	RwIm2DVertexSetRecipCameraZ(&verts[0], recipz);
	RwIm2DVertexSetIntRGBA(&verts[0], c2.r, c2.g, c2.b, c2.a);
	RwIm2DVertexSetU(&verts[0], 0.0f, recipz);
	RwIm2DVertexSetV(&verts[0], 0.0f, recipz);

	RwIm2DVertexSetScreenX(&verts[1], x4);
	RwIm2DVertexSetScreenY(&verts[1], y4);
	RwIm2DVertexSetScreenZ(&verts[1], screenz);
	RwIm2DVertexSetCameraZ(&verts[1], z);
	RwIm2DVertexSetRecipCameraZ(&verts[1], recipz);
	RwIm2DVertexSetIntRGBA(&verts[1], c3.r, c3.g, c3.b, c3.a);
	RwIm2DVertexSetU(&verts[1], 1.0f, recipz);
	RwIm2DVertexSetV(&verts[1], 0.0f, recipz);

	RwIm2DVertexSetScreenX(&verts[2], x2);
	RwIm2DVertexSetScreenY(&verts[2], y2);
	RwIm2DVertexSetScreenZ(&verts[2], screenz);
	RwIm2DVertexSetCameraZ(&verts[2], z);
	RwIm2DVertexSetRecipCameraZ(&verts[2], recipz);
	RwIm2DVertexSetIntRGBA(&verts[2], c1.r, c1.g, c1.b, c1.a);
	RwIm2DVertexSetU(&verts[2], 1.0f, recipz);
	RwIm2DVertexSetV(&verts[2], 1.0f, recipz);

	RwIm2DVertexSetScreenX(&verts[3], x1);
	RwIm2DVertexSetScreenY(&verts[3], y1);
	RwIm2DVertexSetScreenZ(&verts[3], screenz);
	RwIm2DVertexSetCameraZ(&verts[3], z);
	RwIm2DVertexSetRecipCameraZ(&verts[3], recipz);
	RwIm2DVertexSetIntRGBA(&verts[3], c0.r, c0.g, c0.b, c0.a);
	RwIm2DVertexSetU(&verts[3], 0.0f, recipz);
	RwIm2DVertexSetV(&verts[3], 1.0f, recipz);

	RwIm2DVertexSetScreenX(&verts[4], x3);
	RwIm2DVertexSetScreenY(&verts[4], y3);
	RwIm2DVertexSetScreenZ(&verts[4], screenz);
	RwIm2DVertexSetCameraZ(&verts[4], z);
	RwIm2DVertexSetRecipCameraZ(&verts[4], recipz);
	RwIm2DVertexSetIntRGBA(&verts[4], c2.r, c2.g, c2.b, c2.a);
	RwIm2DVertexSetU(&verts[4], 0.0f, recipz);
	RwIm2DVertexSetV(&verts[4], 0.0f, recipz);

	RwIm2DVertexSetScreenX(&verts[5], x2);
	RwIm2DVertexSetScreenY(&verts[5], y2);
	RwIm2DVertexSetScreenZ(&verts[5], screenz);
	RwIm2DVertexSetCameraZ(&verts[5], z);
	RwIm2DVertexSetRecipCameraZ(&verts[5], recipz);
	RwIm2DVertexSetIntRGBA(&verts[5], c1.r, c1.g, c1.b, c1.a);
	RwIm2DVertexSetU(&verts[5], 1.0f, recipz);
	RwIm2DVertexSetV(&verts[5], 1.0f, recipz);
}

void
CSprite::RenderBufferedOneXLUSprite2D(float x, float y, float w, float h, const RwRGBA &colour, int16 intens, uint8 alpha)
{
	m_bFlushSpriteBufferSwitchZTest = 1;
	CRGBA col(intens * colour.red >> 8, intens * colour.green >> 8, intens * colour.blue >> 8, alpha);
	CRect rect(x - w, y - h, x + h, y + h);
	Set6Vertices2D(&SpriteBufferVerts[6 * nSpriteBufferIndex], rect, col, col, col, col);
	nSpriteBufferIndex++;
	if (nSpriteBufferIndex >= SPRITEBUFFERSIZE)
		FlushSpriteBuffer();
}

void
CSprite::RenderBufferedOneXLUSprite2D_Rotate_Dimension(float x, float y, float w, float h, const RwRGBA &colour, int16 intens, float rotation, uint8 alpha)
{
	m_bFlushSpriteBufferSwitchZTest = 1;
	CRGBA col(intens * colour.red >> 8, intens * colour.green >> 8, intens * colour.blue >> 8, alpha);
	float c = Cos(rotation);
	float s = Sin(rotation);

	Set6Vertices2D(&SpriteBufferVerts[6 * nSpriteBufferIndex],
		x + c*w - s*h,
		y - c*h - s*w,
		x + c*w + s*h,
		y + c*h - s*w,
		x - c*w - s*h,
		y - c*h + s*w,
		x - c*w + s*h,
		y + c*h + s*w,
		col, col, col, col);
	nSpriteBufferIndex++;
	if (nSpriteBufferIndex >= SPRITEBUFFERSIZE)
		FlushSpriteBuffer();
}