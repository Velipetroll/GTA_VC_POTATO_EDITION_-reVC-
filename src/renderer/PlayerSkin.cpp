#include "common.h"

#include "main.h"
#include "PlayerSkin.h"
#include "TxdStore.h"
#include "rtbmp.h"
#include "ClumpModelInfo.h"
#include "VisibilityPlugins.h"
#include "World.h"
#include "PlayerInfo.h"
#include "CdStream.h"
#include "FileMgr.h"
#include "Directory.h"
#include "RwHelper.h"
#include "Timer.h"
#include "Lights.h"
#include "MemoryMgr.h"

RpClump *gpPlayerClump;
float gOldFov;

int CPlayerSkin::m_txdSlot;

void
FindPlayerDff(uint32 &offset, uint32 &size)
{
	// OPTIMIZACIÓN EXTREMA: Vaciado. 
	// Ya no escaneamos el disco duro buscando el modelo 3D extra.
}

void
LoadPlayerDff(void)
{
	// OPTIMIZACIÓN EXTREMA: Vaciado. 
	// Ahorramos un pico masivo de RAM al no cargar un clon de Tommy en memoria.
}

void
CPlayerSkin::Initialise(void)
{
	m_txdSlot = CTxdStore::AddTxdSlot("skin");
	CTxdStore::Create(m_txdSlot);
	CTxdStore::AddRef(m_txdSlot);
}

void
CPlayerSkin::Shutdown(void)
{
	CTxdStore::RemoveTxdSlot(m_txdSlot);
}

RwTexture *
CPlayerSkin::GetSkinTexture(const char *texName)
{
	// INTACTO: Se necesita para que Tommy no sea invisible en el juego.
	// Solo se ejecuta una vez al cargar partida o cambiar de traje.
	RwTexture *tex;
	RwRaster *raster;
	int32 width, height, depth, format;

	CTxdStore::PushCurrentTxd();
	CTxdStore::SetCurrentTxd(m_txdSlot);
	tex = RwTextureRead(texName, NULL);
	CTxdStore::PopCurrentTxd();
	if (tex != nil) return tex;

	if (strcmp(DEFAULT_SKIN_NAME, texName) == 0 || texName[0] == '\0')
		sprintf(gString, "models\\generic\\player.bmp");
	else
		sprintf(gString, "skins\\%s.bmp", texName);

	if (RwImage *image = RtBMPImageRead(gString)) {
		RwImageFindRasterFormat(image, rwRASTERTYPETEXTURE, &width, &height, &depth, &format);
		raster = RwRasterCreate(width, height, depth, format);
		RwRasterSetFromImage(raster, image);

		tex = RwTextureCreate(raster);
		RwTextureSetName(tex, texName);
		RwTextureSetFilterMode(tex, rwFILTERLINEAR);
		RwTexDictionaryAddTexture(CTxdStore::GetSlot(m_txdSlot)->texDict, tex);

		RwImageDestroy(image);
	}
	return tex;
}

void
CPlayerSkin::BeginFrontendSkinEdit(void)
{
	// OPTIMIZACIÓN EXTREMA:
	// El menú 2D ya no fuerza la carga del modelo 3D ni altera el FOV de la cámara.
	CWorld::Players[0].LoadPlayerSkin();
}

void
CPlayerSkin::EndFrontendSkinEdit(void)
{
	// OPTIMIZACIÓN EXTREMA:
	// Vaciado. Como nunca lo creamos, no hay nada que destruir.
}

void
CPlayerSkin::RenderFrontendSkinEdit(void)
{
	// OPTIMIZACIÓN DEFINITIVA:
	// Muerte al Tommy Vercetti 3D giratorio.
	// Cero cálculos de matrices (RwFrameTransform, RwFrameTranslate, RwFrameRotate).
	// El menú de skins ahora es puramente 2D y ultraligero.
}