#include "common.h"

#include "main.h"
#include "RwHelper.h"
#include "Lights.h"
#include "PointLights.h"
#include "RpAnimBlend.h"
#include "AnimBlendClumpData.h"
#include "Bones.h"
#include "Renderer.h"
#include "ModelIndices.h"
#include "Shadows.h"
#include "Timecycle.h"
#include "CutsceneShadow.h"
#include "CutsceneObject.h"
#include "ModelIndices.h"
#include "RpAnimBlend.h"


CCutsceneObject::CCutsceneObject(void)
{
	SetStatus(STATUS_SIMPLE);
	bUsesCollision = false;
	bStreamingDontDelete = true;
	ObjectCreatedBy = CUTSCENE_OBJECT;
	m_fMass = 1.0f;
	m_fTurnMass = 1.0f;
	
	m_pAttachTo = nil;
	m_pAttachmentObject = nil;
	m_pShadow = nil;
}

CCutsceneObject::~CCutsceneObject(void)
{
	if ( m_pShadow )
	{
		delete m_pShadow;
		m_pShadow = nil;
	}
}

void
CCutsceneObject::SetModelIndex(uint32 id)
{
	CEntity::SetModelIndex(id);
	assert(RwObjectGetType(m_rwObject) == rpCLUMP);
	RpAnimBlendClumpInit((RpClump*)m_rwObject);
	(*RPANIMBLENDCLUMPDATA(m_rwObject))->velocity3d = &m_vecMoveSpeed;
	(*RPANIMBLENDCLUMPDATA(m_rwObject))->frames[0].flag |= AnimBlendFrameData::VELOCITY_EXTRACTION_3D;
}

void
CCutsceneObject::CreateShadow(void)
{
	return;
}

void
CCutsceneObject::ProcessControl(void)
{
	CPhysical::ProcessControl();

	if ( m_pAttachTo )
	{
		if ( m_pAttachmentObject )
			GetMatrix() = CMatrix((RwMatrix*)m_pAttachTo);
		else
			GetMatrix() = CMatrix(RwFrameGetLTM((RwFrame*)m_pAttachTo));
	}
	else
	{
		if(CTimer::GetTimeStep() < 1/100.0f)
			m_vecMoveSpeed *= 100.0f;
		else
			m_vecMoveSpeed *= 1.0f/CTimer::GetTimeStep();
	
		ApplyMoveSpeed();
	}
}

static RpMaterial*
MaterialSetAlpha(RpMaterial *material, void *data)
{
	((RwRGBA*)RpMaterialGetColor(material))->alpha = (uint8)(uintptr)data;
	return material;
}

void
CCutsceneObject::PreRender(void)
{
	if ( m_pAttachTo )
	{
		if ( m_pAttachmentObject )
		{
			m_pAttachmentObject->UpdateRpHAnim();
			GetMatrix() = CMatrix((RwMatrix*)m_pAttachTo);
		}
		else
			GetMatrix() = CMatrix(RwFrameGetLTM((RwFrame*)m_pAttachTo));
		
		if ( RwObjectGetType(m_rwObject) == rpCLUMP && IsClumpSkinned(GetClump()) )
		{
			RpAtomic *atomic = GetFirstAtomic(GetClump());
			atomic->boundingSphere.center = (*RPANIMBLENDCLUMPDATA(GetClump()))->frames[0].hanimFrame->t;
		}
	}
	
	if ( RwObjectGetType(m_rwObject) == rpCLUMP )
		UpdateRpHAnim();
	
	return;
	
}

void
CCutsceneObject::Render(void)
{
	SetCullMode(rwCULLMODECULLNONE);
	CObject::Render();
	SetCullMode(rwCULLMODECULLBACK);
}

bool
CCutsceneObject::SetupLighting(void)
{
	return false;
}

void
CCutsceneObject::RemoveLighting(bool reset)
{
	CRenderer::RemoveVehiclePedLights(this, reset);
}
