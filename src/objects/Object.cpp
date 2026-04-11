#include "common.h"

#include "main.h"
#include "Lights.h"
#include "Pools.h"
#include "Radar.h"
#include "Object.h"
#include "DummyObject.h"
#include "Particle.h"
#include "General.h"
#include "ObjectData.h"
#include "World.h"
#include "Floater.h"
#include "soundlist.h"
#include "WaterLevel.h"
#include "Timecycle.h"
#include "Stats.h"
#include "SpecialFX.h"

#define BEACHBALL_MAX_SCORE 250
// the proportion of the ball speed compared to the player speed when it hits the player
#define BEACHBALL_SPEED_PROPORTION 0.4f

int16 CObject::nNoTempObjects;
//int16 CObject::nBodyCastHealth = 1000;
float CObject::fDistToNearestTree;

// Object pools tends to be full sometimes, let's free a temp. object in this case.
#ifdef FIX_BUGS
void *CObject::operator new(size_t sz) throw() {
	CObject *obj = CPools::GetObjectPool()->New();
	if (!obj) {
		CObjectPool *objectPool = CPools::GetObjectPool();
		for (int32 i = 0; i < objectPool->GetSize(); i++) {
			CObject *existing = objectPool->GetSlot(i);
			if (existing && existing->ObjectCreatedBy == TEMP_OBJECT) {
				int32 handle = objectPool->GetIndex(existing);
				CWorld::Remove(existing);
				delete existing;
				obj = objectPool->New(handle);
				break;
			}
		}
	}
	return obj;
}
#else
void *CObject::operator new(size_t sz) throw() { return CPools::GetObjectPool()->New(); }
#endif
void *CObject::operator new(size_t sz, int handle) throw() { return CPools::GetObjectPool()->New(handle); };

void CObject::operator delete(void *p, size_t sz) throw() { CPools::GetObjectPool()->Delete((CObject*)p); }
void CObject::operator delete(void *p, int handle) throw() { CPools::GetObjectPool()->Delete((CObject*)p); }

CObject::CObject(void)
{
	m_type = ENTITY_TYPE_OBJECT;
	m_fUprootLimit = 0.0f;
	m_nCollisionDamageEffect = 0;
	m_nSpecialCollisionResponseCases = COLLRESPONSE_NONE;
	m_bCameraToAvoidThisObject = false;
	ObjectCreatedBy = UNKNOWN_OBJECT;
	m_nEndOfLifeTime = 0;
	m_colour2 = 0;
	m_colour1 = m_colour2;
	m_nBonusValue = 0;
	m_nCostValue = 0;
	bIsPickup = false;
	bPickupObjWithMessage = false;
	bOutOfStock = false;
	bGlassCracked = false;
	bGlassBroken = false;
	bHasBeenDamaged = false;
	m_nRefModelIndex = -1;
	bUseVehicleColours = false;
	m_pCurSurface = nil;
	m_pCollidingEntity = nil;
	m_nBeachballBounces = 0;
	bIsStreetLight = false;
	m_area = AREA_EVERYWHERE;
}

CObject::CObject(int32 mi, bool createRW)
{
	if (createRW)
		SetModelIndex(mi);
	else
		SetModelIndexNoCreate(mi);
	Init();
}

CObject::CObject(CDummyObject *dummy)
{
	SetModelIndexNoCreate(dummy->GetModelIndex());

	if (dummy->m_rwObject)
		AttachToRwObject(dummy->m_rwObject);
	else
		SetMatrix(dummy->GetMatrix());

	m_objectMatrix = dummy->GetMatrix();
	dummy->DetachFromRwObject();
	Init();
	m_level = dummy->m_level;
	m_area = dummy->m_area;
}

CObject::~CObject(void)
{
	CRadar::ClearBlipForEntity(BLIP_OBJECT, CPools::GetObjectPool()->GetIndex(this));

	if (m_nRefModelIndex != -1)
		CModelInfo::GetModelInfo(m_nRefModelIndex)->RemoveRef();

	if (ObjectCreatedBy == TEMP_OBJECT && nNoTempObjects != 0)
		nNoTempObjects--;
}

void
CObject::ProcessControl(void)
{
	CVector point, impulse;
	if (m_nCollisionDamageEffect)
		ObjectDamage(m_fDamageImpulse);
	CPhysical::ProcessControl();
	if (mod_Buoyancy.ProcessBuoyancy(this, m_fBuoyancy, &point, &impulse)) {
		bIsInWater = true;
		SetIsStatic(false);
		ApplyMoveForce(impulse);
		ApplyTurnForce(impulse, point);
		float fTimeStep = Pow(0.97f, CTimer::GetTimeStep());
		m_vecMoveSpeed *= fTimeStep;
		m_vecTurnSpeed *= fTimeStep;
	}
	int16 mi = GetModelIndex();
	if ((mi == MI_EXPLODINGBARREL || mi == MI_PETROLPUMP || mi == MI_PETROLPUMP2) && bHasBeenDamaged && bIsVisible
		&& (CGeneral::GetRandomNumber() & 0x1F) == 10) {
		bExplosionProof = true;
		bIsVisible = false;
		bUsesCollision = false;
		bAffectedByGravity = false;
		m_vecMoveSpeed = CVector(0.0f, 0.0f, 0.0f);
	}
	if (mi == MI_RCBOMB) {
		float fTurnForce = -(m_fTurnMass / 20.0f);
		CPhysical::ApplyTurnForce(m_vecMoveSpeed * fTurnForce, -GetForward());
		float fScalar = 1.0f - m_vecMoveSpeed.MagnitudeSqr() / 5.0f;
		float fScalarTimed = Pow(fScalar, CTimer::GetTimeStep());
		m_vecMoveSpeed *= fScalarTimed;
	}
	if (mi == MI_BEACHBALL) {
		float fTimeStep = Pow(0.95f, CTimer::GetTimeStep());
		float fPreviousVecSpeedMag = m_vecMoveSpeed.Magnitude2D();
		m_vecMoveSpeed.x *= fTimeStep;
		m_vecMoveSpeed.y *= fTimeStep;
		m_vecMoveSpeed.z += fPreviousVecSpeedMag - m_vecMoveSpeed.Magnitude2D();
		if (!FindPlayerVehicle()) {
			CVector distance;
			distance.x = FindPlayerCoors().x - GetPosition().x;
			distance.y = FindPlayerCoors().y - GetPosition().y;
			distance.z = FindPlayerCoors().z - GetPosition().z;
			if (distance.z > 0.0 && distance.z < 1.5f && distance.Magnitude2D() < 1.0f) {
				CVector playerSpeed = FindPlayerSpeed();
				if (fPreviousVecSpeedMag < 0.05f && playerSpeed.Magnitude() > 0.1f) {
					playerSpeed.z = 0.0f;
					playerSpeed.Normalise();
					playerSpeed.z = 0.3f;
					m_vecMoveSpeed = CVector(
						playerSpeed.x * BEACHBALL_SPEED_PROPORTION,
						playerSpeed.y * BEACHBALL_SPEED_PROPORTION,
						0.3f          * BEACHBALL_SPEED_PROPORTION
					);
					PlayOneShotScriptObject(SCRIPT_SOUND_HIT_BALL, GetPosition());
					m_vecTurnSpeed += CVector(
						((CGeneral::GetRandomNumber() % 16) - 7) / 10.0f,
						((CGeneral::GetRandomNumber() % 16) - 7) / 10.0f,
						0.0f);
					if (m_nBeachballBounces > 0) {
						m_nBeachballBounces++;
					}
					if (m_nBeachballBounces > 0) {
						sprintf(gString, "%d", m_nBeachballBounces);
						CMoneyMessages::RegisterOne(GetPosition(), gString, 255, 50, 0, 0.6f, 0.5f);
						CStats::RegisterHighestScore(3, m_nBeachballBounces);
					}
				}
			}
			if (distance.z > -1.05 && distance.z < -0.6 && m_vecMoveSpeed.z < 0.0f && distance.Magnitude2D() < 0.9f) {
				m_vecMoveSpeed.x += (CGeneral::GetRandomNumber() % 8 - 3) / 100.0f;
				m_vecMoveSpeed.y += (CGeneral::GetRandomNumber() % 8 - 3) / 100.0f;
				m_vecMoveSpeed.z = Max(m_vecMoveSpeed.z + 0.3f, 0.2f);
				PlayOneShotScriptObject(SCRIPT_SOUND_HIT_BALL, GetPosition());
				m_vecTurnSpeed.x += (CGeneral::GetRandomNumber() % 16 - 7) / 10.0f;
				m_vecTurnSpeed.y += (CGeneral::GetRandomNumber() % 16 - 7) / 10.0f;
				m_nBeachballBounces++;
				m_nBeachballBounces = Min(m_nBeachballBounces, BEACHBALL_MAX_SCORE);
				sprintf(gString, "%d", m_nBeachballBounces);
				CMoneyMessages::RegisterOne(GetPosition(), gString, 255, 50, 0, 0.6f, 0.5f);
				CStats::RegisterHighestScore(3, m_nBeachballBounces);
			}
		}
	}
	if (bIsBIGBuilding) {
		bIsInSafePosition = true;
	}
}

void
CObject::Teleport(CVector vecPos)
{
	CWorld::Remove(this);
	GetMatrix().GetPosition() = vecPos;
	GetMatrix().UpdateRW();
	UpdateRwFrame();
	CWorld::Add(this);
}

void
CObject::Render(void)
{
	if (bDoNotRender)
		return;

	if (m_nRefModelIndex != -1 && ObjectCreatedBy == TEMP_OBJECT && bUseVehicleColours) {
		CVehicleModelInfo *mi = (CVehicleModelInfo*)CModelInfo::GetModelInfo(m_nRefModelIndex);
		assert(mi->GetModelType() == MITYPE_VEHICLE);
		mi->SetVehicleColour(m_colour1, m_colour2);
	}

	// =========================================================================
	// POTATO EDITION: GESTIÓN DE SALPICADURAS DE BARCOS ELIMINADA
	// =========================================================================

	CEntity::Render();
}

bool
CObject::SetupLighting(void)
{
	if (bRenderScorched) {
		WorldReplaceNormalLightsWithScorched(Scene.world, 0.1f);
		return true;
	}
	else if (bIsPickup) {
		SetFullAmbient();
		return true;
	}
	else if (bIsWeapon) {
		ActivateDirectional();
		SetAmbientColoursForPedsCarsAndObjects();
		return true;
	}
	return false;
}

void
CObject::RemoveLighting(bool reset)
{
	if (reset) {
		SetAmbientColours();
		DeActivateDirectional();
	}
}

void
CObject::ObjectDamage(float amount)
{
	if (!m_nCollisionDamageEffect || !bUsesCollision)
		return;
	static int8 nFrameGen = 0;
	bool bBodyCastDamageEffect = false;

	if ((amount * m_fCollisionDamageMultiplier > 150.0f || bBodyCastDamageEffect) && m_nCollisionDamageEffect) {
		const CVector& vecPos = GetMatrix().GetPosition();

		switch (m_nCollisionDamageEffect) {
		case DAMAGE_EFFECT_CHANGE_MODEL:
			bRenderDamaged = true;
			return;
		case DAMAGE_EFFECT_SPLIT_MODEL:
			return;
		case DAMAGE_EFFECT_SMASH_AND_DAMAGE_TRAFFICLIGHTS:
		{
			if (bRenderDamaged) {
				break;
			}
			bRenderDamaged = true;
			CVector min = 0.85f * GetColModel()->boundingBox.min;
			CVector max = 0.85f * GetColModel()->boundingBox.max;
			min.z = max.z;
			min = GetMatrix() * min;

			// POTATO EDITION: Eliminadas las partículas de escombros de semáforo
			PlayOneShotScriptObject(SCRIPT_SOUND_METAL_COLLISION, min);
			break;
		}
		case DAMAGE_EFFECT_CHANGE_THEN_SMASH: {
			if (!bRenderDamaged) {
				bRenderDamaged = true;
				return;
			}
			// fall through
		}
		case DAMAGE_EFFECT_SMASH_COMPLETELY: {
			bIsVisible = false;
			bUsesCollision = false;
			if (!GetIsStatic()) {
				RemoveFromMovingList();
			}
			SetIsStatic(true);
			bExplosionProof = true;
			SetMoveSpeed(0.0f, 0.0f, 0.0f);
			SetTurnSpeed(0.0f, 0.0f, 0.0f);
			break;
		}
		case DAMAGE_EFFECT_SMASH_CARDBOARD_COMPLETELY:
		case DAMAGE_EFFECT_SMASH_YELLOW_TARGET_COMPLETELY:
		{
			bIsVisible = false;
			bUsesCollision = false;
			if (!GetIsStatic()) {
				RemoveFromMovingList();
			}
			SetIsStatic(true);
			bExplosionProof = true;
			SetMoveSpeed(0.0f, 0.0f, 0.0f);
			SetTurnSpeed(0.0f, 0.0f, 0.0f);

			// POTATO EDITION: Eliminadas las partículas de cartón/blancos
			PlayOneShotScriptObject(SCRIPT_SOUND_BOX_DESTROYED_2, vecPos);
			break;
		}
		case DAMAGE_EFFECT_SMASH_WOODENBOX_COMPLETELY:
		{
			bIsVisible = false;
			bUsesCollision = false;
			if (!GetIsStatic()) {
				RemoveFromMovingList();
			}
			SetIsStatic(true);
			bExplosionProof = true;
			SetMoveSpeed(0.0f, 0.0f, 0.0f);
			SetTurnSpeed(0.0f, 0.0f, 0.0f);

			// POTATO EDITION: Eliminadas partículas de caja de madera
			PlayOneShotScriptObject(SCRIPT_SOUND_BOX_DESTROYED_1, vecPos);
			break;
		}
		case DAMAGE_EFFECT_SMASH_TRAFFICCONE_COMPLETELY:
		case DAMAGE_EFFECT_BURST_BEACHBALL:
		{
			bIsVisible = false;
			bUsesCollision = false;
			if (!GetIsStatic()) {
				RemoveFromMovingList();
			}
			SetIsStatic(true);
			bExplosionProof = true;
			SetMoveSpeed(0.0f, 0.0f, 0.0f);
			SetTurnSpeed(0.0f, 0.0f, 0.0f);

			// POTATO EDITION: Eliminadas partículas de conos/pelota
			if (m_nCollisionDamageEffect == DAMAGE_EFFECT_BURST_BEACHBALL) {
				PlayOneShotScriptObject(SCRIPT_SOUND_HIT_BALL, vecPos);
			}
			else {
				PlayOneShotScriptObject(SCRIPT_SOUND_TIRE_COLLISION, vecPos);
			}
			break;
		}
		case DAMAGE_EFFECT_SMASH_BARPOST_COMPLETELY:
		{
			bIsVisible = false;
			bUsesCollision = false;
			if (!GetIsStatic()) {
				RemoveFromMovingList();
			}
			SetIsStatic(true);
			bExplosionProof = true;
			SetMoveSpeed(0.0f, 0.0f, 0.0f);
			SetTurnSpeed(0.0f, 0.0f, 0.0f);

			// POTATO EDITION: Eliminadas partículas de postes
			PlayOneShotScriptObject(SCRIPT_SOUND_METAL_COLLISION, vecPos);
			break;
		}
		case DAMAGE_EFFECT_SMASH_NEWSTANDNEW1:
		case DAMAGE_EFFECT_SMASH_NEWSTANDNEW2:
		case DAMAGE_EFFECT_SMASH_NEWSTANDNEW3:
		case DAMAGE_EFFECT_SMASH_NEWSTANDNEW4:
		case DAMAGE_EFFECT_SMASH_NEWSTANDNEW5:
		{
			bIsVisible = false;
			bUsesCollision = false;
			if (!GetIsStatic()) {
				RemoveFromMovingList();
			}
			SetIsStatic(true);
			bExplosionProof = true;
			SetMoveSpeed(0.0f, 0.0f, 0.0f);
			SetTurnSpeed(0.0f, 0.0f, 0.0f);

			// POTATO EDITION: Eliminadas partículas de kioscos de revistas
			PlayOneShotScriptObject(SCRIPT_SOUND_METAL_COLLISION, vecPos);
			break;
		}
		case DAMAGE_EFFECT_SMASH_VEGPALM:
		{
			bIsVisible = false;
			bUsesCollision = false;
			if (!GetIsStatic()) {
				RemoveFromMovingList();
			}
			SetIsStatic(true);
			bExplosionProof = true;
			SetMoveSpeed(0.0f, 0.0f, 0.0f);
			SetTurnSpeed(0.0f, 0.0f, 0.0f);

			// POTATO EDITION: Eliminadas partículas de palmera (hojas y polvo)
			PlayOneShotScriptObject(SCRIPT_SOUND_BOX_DESTROYED_2, vecPos);
			break;
		}
		case DAMAGE_EFFECT_SMASH_BLACKBAG:
		case DAMAGE_EFFECT_SMASH_BEACHLOUNGE_WOOD:
		case DAMAGE_EFFECT_SMASH_BEACHLOUNGE_TOWEL:
		{
			bIsVisible = false;
			bUsesCollision = false;
			if (!GetIsStatic()) {
				RemoveFromMovingList();
			}
			SetIsStatic(true);
			bExplosionProof = true;
			SetMoveSpeed(0.0f, 0.0f, 0.0f);
			SetTurnSpeed(0.0f, 0.0f, 0.0f);

			// POTATO EDITION: Eliminadas partículas de bolsas y sillas
			if (m_nCollisionDamageEffect == DAMAGE_EFFECT_SMASH_BLACKBAG) {
				PlayOneShotScriptObject(SCRIPT_SOUND_BOX_DESTROYED_2, vecPos);
			}
			else if (m_nCollisionDamageEffect == DAMAGE_EFFECT_SMASH_BEACHLOUNGE_WOOD) {
				PlayOneShotScriptObject(SCRIPT_SOUND_METAL_COLLISION, vecPos);
			}
			break;
		}
		default:
			DEV("Unhandled collision damage effect id: %d\n", m_nCollisionDamageEffect);
			return;
		}
	}
}

void
CObject::RefModelInfo(int32 modelId)
{
	m_nRefModelIndex = modelId;
	CModelInfo::GetModelInfo(modelId)->AddRef();
}

void
CObject::Init(void)
{
	m_type = ENTITY_TYPE_OBJECT;
	CObjectData::SetObjectData(GetModelIndex(), *this);
	m_nEndOfLifeTime = 0;
	ObjectCreatedBy = GAME_OBJECT;
	SetIsStatic(true);
	bIsPickup = false;
	bPickupObjWithMessage = false;
	bOutOfStock = false;
	bGlassCracked = false;
	bGlassBroken = false;
	bHasBeenDamaged = false;
	bUseVehicleColours = false;
	m_nRefModelIndex = -1;
	m_colour1 = 0;
	m_colour2 = 0;
	m_nBonusValue = 0;
	bIsWeapon = false;
	m_nCostValue = 0;
	m_pCollidingEntity = nil;
	CColPoint point;
	CEntity *outEntity = nil;
	const CVector& vecPos = GetMatrix().GetPosition();
	if (CWorld::ProcessVerticalLine(vecPos, vecPos.z - 10.0f, point, outEntity, true, false, false, false, false, false, nil))
		m_pCurSurface = outEntity;
	else
		m_pCurSurface = nil;

	if (GetModelIndex() == MI_BUOY)
		bTouchingWater = true;

	if (CModelInfo::GetModelInfo(GetModelIndex())->GetModelType() == MITYPE_WEAPON)
		bIsWeapon = true;
	bIsStreetLight = IsLightObject(GetModelIndex());

	m_area = AREA_EVERYWHERE;
}

bool
CObject::CanBeDeleted(void)
{
	switch (ObjectCreatedBy) {
	case GAME_OBJECT:
		return true;
	case MISSION_OBJECT:
		return false;
	case TEMP_OBJECT:
		return true;
	case CUTSCENE_OBJECT:
		return false;
	case CONTROLLED_SUB_OBJECT:
		return false;
	default:
		return true;
	}
}

void
CObject::DeleteAllMissionObjects()
{
	CObjectPool *objectPool = CPools::GetObjectPool();
	for (int32 i = 0; i < objectPool->GetSize(); i++) {
		CObject *pObject = objectPool->GetSlot(i);
		if (pObject && pObject->ObjectCreatedBy == MISSION_OBJECT) {
			CWorld::Remove(pObject);
			delete pObject;
		}
	}
}

void
CObject::DeleteAllTempObjects()
{
	CObjectPool *objectPool = CPools::GetObjectPool();
	for (int32 i = 0; i < objectPool->GetSize(); i++) {
		CObject *pObject = objectPool->GetSlot(i);
		if (pObject && pObject->ObjectCreatedBy == TEMP_OBJECT) {
			CWorld::Remove(pObject);
			delete pObject;
		}
	}
}

void
CObject::DeleteAllTempObjectsInArea(CVector point, float fRadius)
{
	CObjectPool *objectPool = CPools::GetObjectPool();
	for (int32 i = 0; i < objectPool->GetSize(); i++) {
		CObject *pObject = objectPool->GetSlot(i);
		if (pObject && pObject->ObjectCreatedBy == TEMP_OBJECT && (point - pObject->GetPosition()).MagnitudeSqr() < SQR(fRadius)) {
			CWorld::Remove(pObject);
			delete pObject;
		}
	}
}

bool
IsObjectPointerValid(CObject *pObject)
{
	if (!pObject)
		return false;
	int index = CPools::GetObjectPool()->GetJustIndex_NoFreeAssert(pObject);
#ifdef FIX_BUGS
	if (index < 0 || index >= CPools::GetObjectPool()->GetSize())
#else
	if (index < 0 || index > CPools::GetObjectPool()->GetSize())
#endif
		return false;
	return pObject->bIsBIGBuilding || pObject->m_entryInfoList.first;
}
