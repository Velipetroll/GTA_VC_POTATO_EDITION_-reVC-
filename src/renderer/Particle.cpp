#include "common.h"

#include "main.h"
#include "General.h"
#include "Timer.h"
#include "TxdStore.h"
#include "Sprite.h"
#include "Camera.h"
#include "Clock.h"
#include "Collision.h"
#include "World.h"
#include "Shadows.h"
#include "Replay.h"
#include "Stats.h"
#include "Weather.h"
#include "MBlur.h"
#include "main.h"
#include "AudioScriptObject.h"
#include "ParticleObject.h"
#include "Particle.h"
#include "soundlist.h"
#include "SaveBuf.h"
#include "debugmenu.h"

#define MAX_PARTICLES_ON_SCREEN   (750)

#define MAX_SMOKE_FILES           ARRAY_SIZE(SmokeFiles)
#define MAX_RUBBER_FILES          ARRAY_SIZE(RubberFiles)
#define MAX_RAINSPLASH_FILES      ARRAY_SIZE(RainSplashFiles)
#define MAX_WATERSPRAY_FILES      ARRAY_SIZE(WatersprayFiles)
#define MAX_EXPLOSIONMEDIUM_FILES ARRAY_SIZE(ExplosionMediumFiles)
#define MAX_GUNFLASH_FILES        ARRAY_SIZE(GunFlashFiles)
#define MAX_RAINSPLASHUP_FILES    ARRAY_SIZE(RainSplashupFiles)
#define MAX_BIRDFRONT_FILES       ARRAY_SIZE(BirdfrontFiles)
#define MAX_BOAT_FILES            ARRAY_SIZE(BoatFiles)
#define MAX_CARDEBRIS_FILES       ARRAY_SIZE(CardebrisFiles)
#define MAX_CARSPLASH_FILES       ARRAY_SIZE(CarsplashFiles)
#define MAX_RAINDRIP_FILES        (2)
#define MAX_LEAF_FILES            (2)

const char SmokeFiles[][6+1] =
{
	"smoke1",
	"smoke2",
	"smoke3",
	"smoke4",
	"smoke5"
};

const char RubberFiles[][7+1] =
{
	"rubber1",
	"rubber2",
	"rubber3",
	"rubber4",
	"rubber5"
};

const char RainSplashFiles[][7+1] =
{
	"splash1",
	"splash2",
	"splash3",
	"splash4",
	"splash5"
};

const char WatersprayFiles[][11+1] =
{
	"waterspray1",
	"waterspray2",
	"waterspray3"
};

const char ExplosionMediumFiles[][7+1] =
{
	"explo01",
	"explo02",
	"explo03",
	"explo04",
	"explo05",
	"explo06"
};

const char GunFlashFiles[][9+1] =
{
	"gunflash1",
	"gunflash2",
	"gunflash3",
	"gunflash4"
};

const char RainSplashupFiles[][10+1] =
{
	"splash_up1",
	"splash_up2"
};

const char BirdfrontFiles[][8+1] =
{
	"birdf_01",
	"birdf_02",
	"birdf_03",
	"birdf_04"
};

const char BoatFiles[][8+1] =
{
	"boats_01",
	"boats_02",
	"boats_03",
	"boats_04",
	"boats_05",
	"boats_06",
	"boats_07",
	"boats_08"
};

const char CardebrisFiles[][12+1] =
{
	"cardebris_01",
	"cardebris_02",
	"cardebris_03",
	"cardebris_04"
};
				
const char CarsplashFiles[][12+1] =
{
	"carsplash_01",
	"carsplash_02",
	"carsplash_03",
	"carsplash_04"
};

CParticle gParticleArray[MAX_PARTICLES_ON_SCREEN];

RwTexture *gpSmokeTex[MAX_SMOKE_FILES];
RwTexture *gpSmoke2Tex;
RwTexture *gpRubberTex[MAX_RUBBER_FILES];
RwTexture *gpRainSplashTex[MAX_RAINSPLASH_FILES];
RwTexture *gpWatersprayTex[MAX_WATERSPRAY_FILES];
RwTexture *gpExplosionMediumTex[MAX_EXPLOSIONMEDIUM_FILES];
RwTexture *gpGunFlashTex[MAX_GUNFLASH_FILES];
RwTexture *gpRainSplashupTex[MAX_RAINSPLASHUP_FILES];
RwTexture *gpBirdfrontTex[MAX_BIRDFRONT_FILES];
RwTexture *gpBoatTex[MAX_BOAT_FILES];
RwTexture *gpCarDebrisTex[MAX_CARDEBRIS_FILES];
RwTexture *gpCarSplashTex[MAX_CARSPLASH_FILES];

RwTexture *gpBoatWakeTex;
RwTexture *gpFlame1Tex;
RwTexture *gpFlame5Tex;
RwTexture *gpRainDropSmallTex;
RwTexture *gpBloodTex;
RwTexture *gpLeafTex[MAX_LEAF_FILES];
RwTexture *gpCloudTex1;
RwTexture *gpCloudTex4;
RwTexture *gpBloodSmallTex;
RwTexture *gpGungeTex;
RwTexture *gpCollisionSmokeTex;
RwTexture *gpBulletHitTex;
RwTexture *gpGunShellTex;
RwTexture *gpPointlightTex;

RwRaster  *gpSmokeRaster[MAX_SMOKE_FILES];
RwRaster  *gpSmoke2Raster;
RwRaster  *gpRubberRaster[MAX_RUBBER_FILES];
RwRaster  *gpRainSplashRaster[MAX_RAINSPLASH_FILES];
RwRaster  *gpWatersprayRaster[MAX_WATERSPRAY_FILES];
RwRaster  *gpExplosionMediumRaster[MAX_EXPLOSIONMEDIUM_FILES];
RwRaster  *gpGunFlashRaster[MAX_GUNFLASH_FILES];
RwRaster  *gpRainSplashupRaster[MAX_RAINSPLASHUP_FILES];
RwRaster  *gpBirdfrontRaster[MAX_BIRDFRONT_FILES];
RwRaster  *gpBoatRaster[MAX_BOAT_FILES];
RwRaster  *gpCarDebrisRaster[MAX_CARDEBRIS_FILES];
RwRaster  *gpCarSplashRaster[MAX_CARSPLASH_FILES];

RwRaster  *gpBoatWakeRaster;
RwRaster  *gpFlame1Raster;
RwRaster  *gpFlame5Raster;
RwRaster  *gpRainDropSmallRaster;
RwRaster  *gpBloodRaster;
RwRaster  *gpLeafRaster[MAX_LEAF_FILES];
RwRaster  *gpCloudRaster1;
RwRaster  *gpCloudRaster4;
RwRaster  *gpBloodSmallRaster;
RwRaster  *gpGungeRaster;
RwRaster  *gpCollisionSmokeRaster;
RwRaster  *gpBulletHitRaster;
RwRaster  *gpGunShellRaster;
RwRaster  *gpPointlightRaster;

RwTexture *gpRainDropTex;
RwRaster  *gpRainDropRaster;

RwTexture *gpSparkTex;
RwTexture *gpNewspaperTex;
RwTexture *gpGunSmokeTex;
RwTexture *gpDotTex;
RwTexture *gpHeatHazeTex;
RwTexture *gpBeastieTex;
RwTexture *gpRainDripTex[MAX_RAINDRIP_FILES];
RwTexture *gpRainDripDarkTex[MAX_RAINDRIP_FILES];

RwRaster *gpSparkRaster;
RwRaster *gpNewspaperRaster;
RwRaster *gpGunSmokeRaster;
RwRaster *gpDotRaster;
RwRaster *gpHeatHazeRaster;
RwRaster *gpBeastieRaster;
RwRaster *gpRainDripRaster[MAX_RAINDRIP_FILES];
RwRaster *gpRainDripDarkRaster[MAX_RAINDRIP_FILES];

float      CParticle::ms_afRandTable[CParticle::RAND_TABLE_SIZE];
CParticle *CParticle::m_pUnusedListHead;
float      CParticle::m_SinTable[CParticle::SIN_COS_TABLE_SIZE];
float      CParticle::m_CosTable[CParticle::SIN_COS_TABLE_SIZE]; 

int32 Randomizer;
int32 nParticleCreationInterval = 1;
float PARTICLE_WIND_TEST_SCALE  = 0.002f;
float fParticleScaleLimit       = 0.5f;

bool clearWaterDrop;
int32 numWaterDropOnScreen;

#ifdef DEBUGMENU
SETTWEAKPATH("Particle");
TWEAKINT32(nParticleCreationInterval, 0, 5, 1);
TWEAKFLOAT(fParticleScaleLimit, 0.0f, 1.0f, 0.1f);
TWEAKFUNC(CParticle::ReloadConfig);
#endif

void CParticle::ReloadConfig()
{
    return;
}

void CParticle::Initialise()
{
    return;
}

void CParticle::Shutdown()
{
    return;
}

void CParticle::AddParticlesAlongLine(tParticleType type, CVector const &vecStart, CVector const &vecEnd, CVector const &vecDir, float fPower, CEntity *pEntity, float fSize, int32 nRotationSpeed, int32 nRotation, int32 nCurFrame, int32 nLifeSpan)
{
    return;
}

void CParticle::AddParticlesAlongLine(tParticleType type, CVector const &vecStart, CVector const &vecEnd, CVector const &vecDir, float fPower, CEntity *pEntity, float fSize, RwRGBA const &color, int32 nRotationSpeed, int32 nRotation, int32 nCurFrame, int32 nLifeSpan)
{
    return;
}

CParticle *CParticle::AddParticle(tParticleType type, CVector const &vecPos, CVector const &vecDir, CEntity *pEntity, float fSize, int32 nRotationSpeed, int32 nRotation, int32 nCurFrame, int32 nLifeSpan)
{
    return nil;
}

CParticle *CParticle::AddParticle(tParticleType type, CVector const &vecPos, CVector const &vecDir, CEntity *pEntity, float fSize, RwRGBA const &color, int32 nRotationSpeed, int32 nRotation, int32 nCurFrame, int32 nLifeSpan)
{
    return nil;
}

void CParticle::Update()
{
    return;
}

void CParticle::Render()
{
    return;
}

void CParticle::RemovePSystem(tParticleType type)
{
    return;
}

void CParticle::RemoveParticle(CParticle *pParticle, CParticle *pPrevParticle, tParticleSystemData *pPSystemData)
{
    return;
}

void CParticle::AddJetExplosion(CVector const &vecPos, float fPower, float fSize)
{
    return;
}

void CParticle::AddYardieDoorSmoke(CVector const &vecPos, CMatrix const &matMatrix)
{
    return;
}

void CParticle::CalWindDir(CVector *vecDirIn, CVector *vecDirOut)
{
    return;
}

void CParticle::HandleShipsAtHorizonStuff()
{
    return;
}

void CParticle::HandleShootableBirdsStuff(CEntity *entity, CVector const&camPos)
{
    return;
}

void CEntity::AddSteamsFromGround(CVector *unused)
{
    return;
}