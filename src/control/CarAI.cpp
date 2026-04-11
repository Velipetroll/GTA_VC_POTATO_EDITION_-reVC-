#include "common.h"

#include "CarAI.h"

#include "Accident.h"
#include "AutoPilot.h"
#include "CarCtrl.h"
#include "General.h"
#include "HandlingMgr.h"
#include "ModelIndices.h"
#include "PlayerPed.h"
#include "Wanted.h"
#include "DMAudio.h"
#include "Fire.h"
#include "Pools.h"
#include "Population.h"
#include "Timer.h"
#include "TrafficLights.h"
#include "Vehicle.h"
#include "World.h"
#include "ZoneCull.h"

#define DISTANCE_TO_SWITCH_DISTANCE_GOTO 20.0f

// =========================================================================
// POTATO EDITION: Helpers súper rápidos para magnitud 2D al cuadrado
// Evitamos modificar los headers matemáticos originales y garantizamos 0 overhead
// =========================================================================
inline float GetMagSqr2D(const CVector& v) { return v.x * v.x + v.y * v.y; }
inline float GetMagSqr2D(const CVector2D& v) { return v.x * v.x + v.y * v.y; }

float CCarAI::FindSwitchDistanceClose(CVehicle* pVehicle)
{
	return pVehicle->AutoPilot.m_nSwitchDistance;
}

float CCarAI::FindSwitchDistanceFarNormalVehicle(CVehicle* pVehicle)
{
	return FindSwitchDistanceClose(pVehicle) + 5.0f;
}

float CCarAI::FindSwitchDistanceFar(CVehicle* pVehicle)
{
	if (pVehicle->bIsLawEnforcer)
		return 50.0f;
	return FindSwitchDistanceFarNormalVehicle(pVehicle);
}

void CCarAI::BackToCruisingIfNoWantedLevel(CVehicle* pVehicle)
{
	if (FindPlayerPed()->m_pWanted->m_bIgnoredByEveryone || pVehicle->bIsLawEnforcer &&
		(FindPlayerPed()->m_pWanted->GetWantedLevel() == 0 || FindPlayerPed()->m_pWanted->m_bIgnoredByCops || CCullZones::NoPolice())) {
		CCarCtrl::JoinCarWithRoadSystem(pVehicle);
		pVehicle->AutoPilot.m_nCarMission = MISSION_CRUISE;
		pVehicle->AutoPilot.m_nDrivingStyle = DRIVINGSTYLE_STOP_FOR_CARS;
		pVehicle->m_bSirenOrAlarm = false;
		if (CCullZones::NoPolice())
			pVehicle->AutoPilot.m_nCarMission = MISSION_NONE;
	}
}

void CCarAI::UpdateCarAI(CVehicle* pVehicle)
{
	if (pVehicle->bIsLawEnforcer) {
		if (pVehicle->AutoPilot.m_nCarMission == MISSION_BLOCKCAR_FARAWAY ||
			pVehicle->AutoPilot.m_nCarMission == MISSION_RAMPLAYER_FARAWAY ||
			pVehicle->AutoPilot.m_nCarMission == MISSION_BLOCKPLAYER_CLOSE ||
			pVehicle->AutoPilot.m_nCarMission == MISSION_RAMPLAYER_CLOSE)
			pVehicle->AutoPilot.m_nCruiseSpeed = FindPoliceCarSpeedForWantedLevel(pVehicle);
	}

	// POTATO EDITION: Pre-calculamos la distancia al jugador al cuadrado para ahorrar Raíces Cuadradas
	float distToPlayerSqr = 0.0f;
	if (FindPlayerVehicle() || FindPlayerPed()) {
		distToPlayerSqr = GetMagSqr2D(FindPlayerCoors() - pVehicle->GetPosition());
	}

	switch (pVehicle->GetStatus()) {
	case STATUS_PLAYER:
	case STATUS_PLAYER_PLAYBACKFROMBUFFER:
	case STATUS_TRAIN_MOVING:
	case STATUS_TRAIN_NOT_MOVING:
	case STATUS_HELI:
	case STATUS_PLANE:
	case STATUS_PLAYER_REMOTE:
	case STATUS_PLAYER_DISABLED:
		break;
	case STATUS_SIMPLE:
	case STATUS_PHYSICS:
		switch (pVehicle->AutoPilot.m_nCarMission) {
		case MISSION_RAMPLAYER_FARAWAY:
			// POTATO EDITION: SQR() evita el sqrt()
			if (SQR(FindSwitchDistanceClose(pVehicle)) > distToPlayerSqr ||
				pVehicle->AutoPilot.m_bIgnorePathfinding) {
				pVehicle->AutoPilot.m_nCarMission = MISSION_RAMPLAYER_CLOSE;
				if (pVehicle->UsesSiren())
					pVehicle->m_bSirenOrAlarm = true;
			}
			BackToCruisingIfNoWantedLevel(pVehicle);
			break;
		case MISSION_RAMPLAYER_CLOSE:
			if (SQR(FindSwitchDistanceFar(pVehicle)) >= distToPlayerSqr ||
				pVehicle->AutoPilot.m_bIgnorePathfinding) {
				if (FindPlayerVehicle()) {
					if (pVehicle->GetHasCollidedWith(FindPlayerVehicle())) {
						if (pVehicle->AutoPilot.m_nTempAction != TEMPACT_TURNLEFT && pVehicle->AutoPilot.m_nTempAction != TEMPACT_TURNRIGHT) {
							if (FindPlayerVehicle()->GetMoveSpeed().MagnitudeSqr() < 0.0025f) { // 0.05^2
								pVehicle->AutoPilot.m_nTempAction = TEMPACT_REVERSE;
								pVehicle->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 800;
							}
							else {
								pVehicle->AutoPilot.m_nTempAction = TEMPACT_REVERSE;
								pVehicle->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 50;
							}
						}
					}
				}
				if (FindPlayerVehicle() && FindPlayerVehicle()->GetMoveSpeed().MagnitudeSqr() < 0.0025f) // 0.05^2
#ifdef FIX_BUGS
					pVehicle->m_nTimeBlocked += CTimer::GetTimeStepInMilliseconds();
#else
					pVehicle->m_nTimeBlocked += 1000.0f / 60.0f * CTimer::GetTimeStep();
#endif
				else
					pVehicle->m_nTimeBlocked = 0;
				if (!FindPlayerVehicle() || FindPlayerVehicle()->IsUpsideDown() ||
					FindPlayerVehicle()->GetMoveSpeed().MagnitudeSqr() < 0.0025f && pVehicle->m_nTimeBlocked > TIME_COPS_WAIT_TO_EXIT_AFTER_STOPPING) {
					if (pVehicle->bIsLawEnforcer &&
						(pVehicle->GetModelIndex() != MI_RHINO || pVehicle->m_randomSeed > 10000) &&
						distToPlayerSqr < 100.0f) { // 10.0^2
						TellOccupantsToLeaveCar(pVehicle);
						pVehicle->AutoPilot.m_nCruiseSpeed = 0;
						pVehicle->AutoPilot.m_nCarMission = MISSION_NONE;
						if (FindPlayerPed()->m_pWanted->GetWantedLevel() <= 1)
							pVehicle->m_bSirenOrAlarm = false;
					}
				}
			}
			else if (!CCarCtrl::JoinCarWithRoadSystemGotoCoors(pVehicle, FindPlayerCoors(), true)) {
				pVehicle->AutoPilot.m_nCarMission = MISSION_RAMPLAYER_FARAWAY;
				pVehicle->m_bSirenOrAlarm = false;
				pVehicle->m_nCarHornTimer = 0;
			}
			if (pVehicle->bIsLawEnforcer)
				MellowOutChaseSpeed(pVehicle);
			BackToCruisingIfNoWantedLevel(pVehicle);
			break;
		case MISSION_BLOCKPLAYER_FARAWAY:
			if (SQR(FindSwitchDistanceClose(pVehicle)) > distToPlayerSqr ||
				pVehicle->AutoPilot.m_bIgnorePathfinding) {
				pVehicle->AutoPilot.m_nCarMission = MISSION_BLOCKPLAYER_CLOSE;
				if (pVehicle->UsesSiren())
					pVehicle->m_bSirenOrAlarm = true;
			}
			BackToCruisingIfNoWantedLevel(pVehicle);
			break;
		case MISSION_BLOCKPLAYER_CLOSE:
			if (SQR(FindSwitchDistanceFar(pVehicle)) >= distToPlayerSqr ||
				pVehicle->AutoPilot.m_bIgnorePathfinding) {
				if (FindPlayerVehicle() && FindPlayerVehicle()->GetMoveSpeed().MagnitudeSqr() < 0.0016f) // 0.04^2
#ifdef FIX_BUGS
					pVehicle->m_nTimeBlocked += CTimer::GetTimeStepInMilliseconds();
#else
					pVehicle->m_nTimeBlocked += 1000.0f / 60.0f * CTimer::GetTimeStep();
#endif
				else
					pVehicle->m_nTimeBlocked = 0;
				if (!FindPlayerVehicle() || FindPlayerVehicle()->IsUpsideDown() ||
					FindPlayerVehicle()->GetMoveSpeed().MagnitudeSqr() < 0.0016f && pVehicle->m_nTimeBlocked > TIME_COPS_WAIT_TO_EXIT_AFTER_STOPPING) {
					if (pVehicle->bIsLawEnforcer &&
						(pVehicle->GetModelIndex() != MI_RHINO || pVehicle->m_randomSeed > 10000) &&
						distToPlayerSqr < 100.0f) { // 10.0^2
						TellOccupantsToLeaveCar(pVehicle);
						pVehicle->AutoPilot.m_nCruiseSpeed = 0;
						pVehicle->AutoPilot.m_nCarMission = MISSION_NONE;
						if (FindPlayerPed()->m_pWanted->GetWantedLevel() <= 1)
							pVehicle->m_bSirenOrAlarm = false;
					}
				}
			}
			else if (!CCarCtrl::JoinCarWithRoadSystemGotoCoors(pVehicle, FindPlayerCoors(), true)) {
				pVehicle->AutoPilot.m_nCarMission = MISSION_BLOCKPLAYER_FARAWAY;
				pVehicle->m_bSirenOrAlarm = false;
				pVehicle->m_nCarHornTimer = 0;
			}
			if (pVehicle->bIsLawEnforcer)
				MellowOutChaseSpeed(pVehicle);
			BackToCruisingIfNoWantedLevel(pVehicle);
			break;
		case MISSION_GOTOCOORDS:
			if (GetMagSqr2D(pVehicle->AutoPilot.m_vecDestinationCoors - pVehicle->GetPosition()) < SQR(FindSwitchDistanceClose(pVehicle)) ||
				pVehicle->AutoPilot.m_bIgnorePathfinding)
				pVehicle->AutoPilot.m_nCarMission = MISSION_GOTOCOORDS_STRAIGHT;
			break;
		case MISSION_GOTOCOORDS_STRAIGHT:
		{
			float distSqr = GetMagSqr2D(pVehicle->AutoPilot.m_vecDestinationCoors - pVehicle->GetPosition());
			if ((pVehicle->bIsAmbulanceOnDuty || pVehicle->bIsFireTruckOnDuty) && distSqr < 400.0f) // 20.0^2
				pVehicle->AutoPilot.m_nCarMission = MISSION_EMERGENCYVEHICLE_STOP;
			if (distSqr < 9.0f) { // 3.0^2
				pVehicle->AutoPilot.m_nCarMission = MISSION_NONE;
				pVehicle->AutoPilot.m_nTempAction = TEMPACT_NONE;
				if (pVehicle->bParking) {
					TellOccupantsToLeaveCar(pVehicle);
					pVehicle->bParking = false;
				}
			}
			else if (distSqr > SQR(FindSwitchDistanceFarNormalVehicle(pVehicle)) && !pVehicle->AutoPilot.m_bIgnorePathfinding && (CTimer::GetFrameCounter() & 7) == 0) {
				pVehicle->AutoPilot.m_nTempAction = TEMPACT_NONE;
				pVehicle->AutoPilot.m_nCarMission = (CCarCtrl::JoinCarWithRoadSystemGotoCoors(pVehicle, pVehicle->AutoPilot.m_vecDestinationCoors, true)) ?
					MISSION_GOTOCOORDS_STRAIGHT : MISSION_GOTOCOORDS;
			}
			break;
		}
		case MISSION_EMERGENCYVEHICLE_STOP:
			if (GetMagSqr2D(pVehicle->GetMoveSpeed()) < 0.0001f) { // 0.01^2
				if (pVehicle->bIsAmbulanceOnDuty) {
					float distance = 30.0f;
					if (gAccidentManager.FindNearestAccident(pVehicle->AutoPilot.m_vecDestinationCoors, &distance)) {
						TellOccupantsToLeaveCar(pVehicle);
						pVehicle->AutoPilot.m_nCarMission = MISSION_STOP_FOREVER;
					}
					else {
						CCarCtrl::JoinCarWithRoadSystem(pVehicle);
						pVehicle->AutoPilot.m_nCarMission = MISSION_CRUISE;
						pVehicle->m_bSirenOrAlarm = false;
						pVehicle->AutoPilot.m_nCruiseSpeed = 17;
						if (pVehicle->bIsAmbulanceOnDuty) {
							pVehicle->bIsAmbulanceOnDuty = false;
							--CCarCtrl::NumAmbulancesOnDuty;
						}
					}
				}
				if (pVehicle->bIsFireTruckOnDuty) {
					float distance = 30.0f;
					if (gFireManager.FindNearestFire(pVehicle->AutoPilot.m_vecDestinationCoors, &distance)) {
						TellOccupantsToLeaveCar(pVehicle);
						pVehicle->AutoPilot.m_nCarMission = MISSION_STOP_FOREVER;
					}
					else {
						CCarCtrl::JoinCarWithRoadSystem(pVehicle);
						pVehicle->AutoPilot.m_nCarMission = MISSION_CRUISE;
						pVehicle->m_bSirenOrAlarm = false;
						pVehicle->AutoPilot.m_nCruiseSpeed = 17;
						if (pVehicle->bIsFireTruckOnDuty) {
							pVehicle->bIsFireTruckOnDuty = false;
							--CCarCtrl::NumFiretrucksOnDuty;
						}
					}
				}
			}
			break;
		case MISSION_GOTOCOORDS_ACCURATE:
			if (GetMagSqr2D(pVehicle->AutoPilot.m_vecDestinationCoors - pVehicle->GetPosition()) < SQR(FindSwitchDistanceClose(pVehicle)) ||
				pVehicle->AutoPilot.m_bIgnorePathfinding)
				pVehicle->AutoPilot.m_nCarMission = MISSION_GOTO_COORDS_STRAIGHT_ACCURATE;
			break;
		case MISSION_GOTO_COORDS_STRAIGHT_ACCURATE:
		{
			float distSqr = GetMagSqr2D(pVehicle->AutoPilot.m_vecDestinationCoors - pVehicle->GetPosition());
			if (distSqr < 1.0f) {
				pVehicle->AutoPilot.m_nCarMission = MISSION_NONE;
				pVehicle->AutoPilot.m_nTempAction = TEMPACT_NONE;
				if (pVehicle->bParking) {
					TellOccupantsToLeaveCar(pVehicle);
					pVehicle->bParking = false;
				}
			}
			else if (distSqr > SQR(FindSwitchDistanceFarNormalVehicle(pVehicle)) && !pVehicle->AutoPilot.m_bIgnorePathfinding && (CTimer::GetFrameCounter() & 7) == 0) {
				pVehicle->AutoPilot.m_nTempAction = TEMPACT_NONE;
				pVehicle->AutoPilot.m_nCarMission = (CCarCtrl::JoinCarWithRoadSystemGotoCoors(pVehicle, pVehicle->AutoPilot.m_vecDestinationCoors, true)) ?
					MISSION_GOTO_COORDS_STRAIGHT_ACCURATE : MISSION_GOTOCOORDS_ACCURATE;
			}
			break;
		}
		case MISSION_RAMCAR_FARAWAY:
			if (pVehicle->AutoPilot.m_pTargetCar) {
				if (GetMagSqr2D(pVehicle->GetPosition() - pVehicle->AutoPilot.m_pTargetCar->GetPosition()) < SQR(FindSwitchDistanceClose(pVehicle)) ||
					pVehicle->AutoPilot.m_bIgnorePathfinding)
					pVehicle->AutoPilot.m_nCarMission = MISSION_RAMCAR_CLOSE;
			}
			else {
				pVehicle->AutoPilot.m_nCarMission = MISSION_NONE;
			}
			break;
		case MISSION_RAMCAR_CLOSE:
			if (pVehicle->AutoPilot.m_pTargetCar) {
#ifdef FIX_BUGS 
				if (FindPlayerVehicle() == pVehicle->AutoPilot.m_pTargetCar)
#endif
					BackToCruisingIfNoWantedLevel(pVehicle);
				if (GetMagSqr2D(pVehicle->AutoPilot.m_pTargetCar->GetPosition() - pVehicle->GetPosition()) <= SQR(FindSwitchDistanceFar(pVehicle)) ||
					pVehicle->AutoPilot.m_bIgnorePathfinding) {
					if (pVehicle->GetHasCollidedWith(pVehicle->AutoPilot.m_pTargetCar)) {
						if (pVehicle->GetMoveSpeed().MagnitudeSqr() < 0.0016f) { // 0.04^2
							pVehicle->AutoPilot.m_nTempAction = TEMPACT_REVERSE;
							pVehicle->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 800;
						}
					}
				}
				else {
					pVehicle->AutoPilot.m_nCarMission = MISSION_RAMCAR_FARAWAY;
					CCarCtrl::JoinCarWithRoadSystem(pVehicle);
				}
			}
			else {
				pVehicle->AutoPilot.m_nCarMission = MISSION_NONE;
			}
			break;
		case MISSION_BLOCKCAR_FARAWAY:
			if (pVehicle->AutoPilot.m_pTargetCar) {
				if (GetMagSqr2D(pVehicle->AutoPilot.m_pTargetCar->GetPosition() - pVehicle->GetPosition()) < SQR(FindSwitchDistanceClose(pVehicle)) ||
					pVehicle->AutoPilot.m_bIgnorePathfinding) {
					pVehicle->AutoPilot.m_nCarMission = MISSION_BLOCKCAR_CLOSE;
					if (pVehicle->UsesSiren())
						pVehicle->m_bSirenOrAlarm = true;
				}
			}
			else {
				pVehicle->AutoPilot.m_nCarMission = MISSION_NONE;
			}
			break;
		case MISSION_BLOCKCAR_CLOSE:
			if (pVehicle->AutoPilot.m_pTargetCar) {
				if (GetMagSqr2D(pVehicle->AutoPilot.m_pTargetCar->GetPosition() - pVehicle->GetPosition()) > SQR(FindSwitchDistanceFar(pVehicle)) &&
					!pVehicle->AutoPilot.m_bIgnorePathfinding) {
					pVehicle->AutoPilot.m_nCarMission = MISSION_BLOCKCAR_FARAWAY;
					pVehicle->m_bSirenOrAlarm = false;
					pVehicle->m_nCarHornTimer = 0;
					CCarCtrl::JoinCarWithRoadSystem(pVehicle);
				}
			}
			else {
				pVehicle->AutoPilot.m_nCarMission = MISSION_NONE;
			}
			break;
		case MISSION_ATTACKPLAYER:
			if (pVehicle->bIsLawEnforcer)
				MellowOutChaseSpeedBoat(pVehicle);
			BackToCruisingIfNoWantedLevel(pVehicle);
			break;
		case MISSION_SLOWLY_DRIVE_TOWARDS_PLAYER_1:
			if (GetMagSqr2D(pVehicle->AutoPilot.m_vecDestinationCoors - pVehicle->GetPosition()) < 2.25f) // 1.5^2
				pVehicle->AutoPilot.m_nCarMission = MISSION_SLOWLY_DRIVE_TOWARDS_PLAYER_2;
			BackToCruisingIfNoWantedLevel(pVehicle);
			break;
		case MISSION_SLOWLY_DRIVE_TOWARDS_PLAYER_2:
		{
			if (distToPlayerSqr < 169.0f) { // 13.0^2
				TellOccupantsToLeaveCar(pVehicle);
				pVehicle->AutoPilot.m_nCruiseSpeed = 0;
				pVehicle->AutoPilot.m_nCarMission = MISSION_STOP_FOREVER;
			}
			if (distToPlayerSqr > 4900.0f || FindPlayerPed()->m_pWanted->m_bIgnoredByEveryone || // 70.0^2
				(FindPlayerPed()->m_pWanted->GetWantedLevel() == 0 || FindPlayerPed()->m_pWanted->m_bIgnoredByCops || CCullZones::NoPolice())) {
				TellOccupantsToLeaveCar(pVehicle);
				pVehicle->AutoPilot.m_nCruiseSpeed = 0;
				pVehicle->AutoPilot.m_nCarMission = MISSION_STOP_FOREVER;
			}
			break;
		}
		case MISSION_BLOCKPLAYER_FORWARDANDBACK:
		{
			CVector2D diff = (CVector2D)FindPlayerCoors() - pVehicle->GetPosition();
			float distSqr = Max(0.000001f, GetMagSqr2D(diff)); // Evitamos div por cero
			if (!FindPlayerVehicle() || (DotProduct2D(diff, FindPlayerSpeed()) / distSqr) > 0.05f) // Simplificación matemática
				pVehicle->AutoPilot.m_nCarMission = MISSION_BLOCKPLAYER_CLOSE;
			BackToCruisingIfNoWantedLevel(pVehicle);
			break;
		}
		default:
			if (pVehicle->bIsLawEnforcer && FindPlayerPed()->m_pWanted->GetWantedLevel() > 0 && !CCullZones::NoPolice()) {
				if (ABS(FindPlayerCoors().x - pVehicle->GetPosition().x) > 10.0f ||
					ABS(FindPlayerCoors().y - pVehicle->GetPosition().y) > 10.0f) {
					pVehicle->AutoPilot.m_nCruiseSpeed = FindPoliceCarSpeedForWantedLevel(pVehicle);
					pVehicle->SetStatus(STATUS_PHYSICS);
					pVehicle->AutoPilot.m_nCarMission =
						pVehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_BOAT ? FindPoliceBoatMissionForWantedLevel() : FindPoliceCarMissionForWantedLevel();
					pVehicle->AutoPilot.m_nTempAction = TEMPACT_NONE;
					pVehicle->AutoPilot.m_nDrivingStyle = DRIVINGSTYLE_AVOID_CARS;
				}
				else if (pVehicle->AutoPilot.m_nCarMission == MISSION_CRUISE) {
					pVehicle->SetStatus(STATUS_PHYSICS);
					TellOccupantsToLeaveCar(pVehicle);
					pVehicle->AutoPilot.m_nCruiseSpeed = 0;
					pVehicle->AutoPilot.m_nCarMission = MISSION_NONE;
					if (FindPlayerPed()->m_pWanted->GetWantedLevel() <= 1)
						pVehicle->m_bSirenOrAlarm = false;
				}
			}
			break;
		}
		break;
	case STATUS_ABANDONED:
	case STATUS_WRECKED:
		pVehicle->AutoPilot.m_nCarMission = MISSION_NONE;
		pVehicle->AutoPilot.m_nCruiseSpeed = 0;
		break;
	}
	if (pVehicle->bIsLawEnforcer && FindPlayerPed()->m_pWanted->GetWantedLevel() >= 1 && CCullZones::PoliceAbandonCars()) {
		TellOccupantsToLeaveCar(pVehicle);
		pVehicle->AutoPilot.m_nCruiseSpeed = 0;
		pVehicle->AutoPilot.m_nCarMission = MISSION_NONE;
	}
	float flatSpeed = GetMagSqr2D(pVehicle->GetMoveSpeed());
	if (flatSpeed > SQR(0.018f)) {
		pVehicle->AutoPilot.m_nTimeToStartMission = CTimer::GetTimeInMilliseconds();
		pVehicle->AutoPilot.m_nAntiReverseTimer = CTimer::GetTimeInMilliseconds();
	}
	if (pVehicle->GetStatus() == STATUS_PHYSICS && pVehicle->AutoPilot.m_nTempAction == TEMPACT_NONE) {
		if (pVehicle->AutoPilot.m_nCarMission != MISSION_NONE) {
			if (pVehicle->AutoPilot.m_nCarMission != MISSION_STOP_FOREVER &&
				pVehicle->AutoPilot.m_nCarMission != MISSION_BLOCKPLAYER_HANDBRAKESTOP &&
				pVehicle->AutoPilot.m_nCruiseSpeed != 0 &&
				(pVehicle->VehicleCreatedBy != RANDOM_VEHICLE || pVehicle->AutoPilot.m_nCarMission != MISSION_CRUISE)) {
				if (pVehicle->AutoPilot.m_nDrivingStyle != DRIVINGSTYLE_STOP_FOR_CARS
					&& pVehicle->AutoPilot.m_nDrivingStyle != DRIVINGSTYLE_STOP_FOR_CARS_IGNORE_LIGHTS ||
					pVehicle->VehicleCreatedBy == MISSION_VEHICLE
					) {
					if (CTimer::GetTimeInMilliseconds() - pVehicle->m_nLastTimeCollided > 500)
						pVehicle->AutoPilot.m_nAntiReverseTimer = CTimer::GetTimeInMilliseconds();
					if (flatSpeed < SQR(0.018f) && CTimer::GetTimeInMilliseconds() - pVehicle->AutoPilot.m_nAntiReverseTimer > 2000) {
						pVehicle->AutoPilot.m_nTempAction = TEMPACT_REVERSE;
						if (pVehicle->AutoPilot.m_nCarMission != MISSION_NONE &&
							pVehicle->AutoPilot.m_nCarMission != MISSION_CRUISE || pVehicle->VehicleCreatedBy == MISSION_VEHICLE)
							pVehicle->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 1500;
						else
							pVehicle->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 750;
						pVehicle->AutoPilot.m_nAntiReverseTimer = CTimer::GetTimeInMilliseconds();
						if (pVehicle->VehicleCreatedBy == RANDOM_VEHICLE)
							pVehicle->AutoPilot.m_nDrivingStyle = Max(DRIVINGSTYLE_AVOID_CARS, pVehicle->AutoPilot.m_nDrivingStyle);
						pVehicle->PlayCarHorn();
					}
				}
			}
		}
	}
	if ((pVehicle->m_randomSeed & 7) == 0) {
		if (CTimer::GetTimeInMilliseconds() - pVehicle->AutoPilot.m_nTimeToStartMission > 30000 &&
			CTimer::GetPreviousTimeInMilliseconds() - pVehicle->AutoPilot.m_nTimeToStartMission <= 30000 &&
			pVehicle->AutoPilot.m_nCarMission == MISSION_CRUISE &&
			!CTrafficLights::ShouldCarStopForBridge(pVehicle)) {
			pVehicle->SetStatus(STATUS_PHYSICS);
			CCarCtrl::SwitchVehicleToRealPhysics(pVehicle);
			pVehicle->AutoPilot.m_nDrivingStyle = DRIVINGSTYLE_AVOID_CARS;
			pVehicle->AutoPilot.m_nTempAction = TEMPACT_REVERSE;
			pVehicle->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 400;
		}
	}
	if (pVehicle->bIsLawEnforcer) {
		if (pVehicle->AutoPilot.m_nCarMission == MISSION_RAMPLAYER_FARAWAY ||
			pVehicle->AutoPilot.m_nCarMission == MISSION_RAMPLAYER_CLOSE) {
			if (FindPlayerVehicle() && FindPlayerVehicle()->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE)
				pVehicle->AutoPilot.m_nCarMission = MISSION_BLOCKPLAYER_FARAWAY;
		}
	}
	if (pVehicle->GetUp().z < -0.7f) {
		pVehicle->AutoPilot.m_nTempAction = TEMPACT_WAIT;
		pVehicle->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 1000;
	}
	if (pVehicle->AutoPilot.m_nTempAction == TEMPACT_NONE) {
		switch (pVehicle->AutoPilot.m_nCarMission) {
		case MISSION_RAMPLAYER_FARAWAY:
		case MISSION_RAMPLAYER_CLOSE:
		case MISSION_BLOCKPLAYER_FARAWAY:
		case MISSION_BLOCKPLAYER_CLOSE:
			if (FindPlayerVehicle() && FindPlayerSpeed().MagnitudeSqr() > pVehicle->GetMoveSpeed().MagnitudeSqr()) {
				if (FindPlayerSpeed().MagnitudeSqr() > 0.01f) { // 0.1^2
					if (DotProduct2D(FindPlayerVehicle()->GetForward(), pVehicle->GetForward()) > 0.0f) {
						CVector2D dist = pVehicle->GetPosition() - FindPlayerCoors();
						CVector2D speed = FindPlayerSpeed();
						if (0.25f * GetMagSqr2D(dist) * GetMagSqr2D(speed) < SQR(DotProduct2D(dist, speed))) {
							if (distToPlayerSqr > 144.0f) { // 12.0^2
								pVehicle->AutoPilot.m_nTempAction = TEMPACT_WAIT;
								pVehicle->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 500;
							}
						}
					}
				}
			}
			break;
		default: break;
		}
	}
	if (pVehicle->pDriver && pVehicle->pDriver->m_objective == OBJECTIVE_KILL_CHAR_ANY_MEANS) {
		if (distToPlayerSqr < 225.0f) { // 15.0^2
			if (!FindPlayerVehicle() || pVehicle->GetHasCollidedWith(FindPlayerVehicle())) {
				pVehicle->AutoPilot.m_nTempAction = TEMPACT_WAIT;
				pVehicle->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 3000;
			}
		}
	}
	if (pVehicle->m_bSirenOrAlarm) {
		if ((uint8)(pVehicle->m_randomSeed ^ CGeneral::GetRandomNumber()) == 0xAD)
			pVehicle->m_nCarHornTimer = 45;
	}
	float target = 1.0f;
	if (pVehicle->AutoPilot.m_nCarMission == MISSION_CRUISE)
		target = CCarCtrl::FindSpeedMultiplierWithSpeedFromNodes(pVehicle->AutoPilot.m_nCruiseSpeedMultiplierType);
	float change = CTimer::GetTimeStep() * 0.01f;
	if (Abs(pVehicle->AutoPilot.m_fCruiseSpeedMultiplier - target) < change)
		pVehicle->AutoPilot.m_fCruiseSpeedMultiplier = target;
	else if (pVehicle->AutoPilot.m_fCruiseSpeedMultiplier > target)
		pVehicle->AutoPilot.m_fCruiseSpeedMultiplier -= change;
	else
		pVehicle->AutoPilot.m_fCruiseSpeedMultiplier += change;

	if (pVehicle->bIsLawEnforcer && FindPlayerPed()->m_pWanted->GetWantedLevel() > 0) {
		if (!FindPlayerVehicle() ||
			FindPlayerVehicle()->GetVehicleAppearance() == VEHICLE_APPEARANCE_CAR ||
			FindPlayerVehicle()->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE) {
			if (pVehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_BOAT) {
				pVehicle->AutoPilot.m_nTempAction = TEMPACT_WAIT;
				pVehicle->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 1000;
			}
		}
		else if (FindPlayerVehicle()->GetVehicleAppearance() == VEHICLE_APPEARANCE_BOAT) {
			if (pVehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_CAR ||
				pVehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE) {
				pVehicle->AutoPilot.m_nTempAction = TEMPACT_WAIT;
				pVehicle->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 1000;
			}
		}
	}
}

void CCarAI::CarHasReasonToStop(CVehicle* pVehicle)
{
	pVehicle->AutoPilot.m_nAntiReverseTimer = CTimer::GetTimeInMilliseconds();
}

float CCarAI::GetCarToGoToCoors(CVehicle* pVehicle, CVector* pTarget)
{
	if (pVehicle->AutoPilot.m_nCarMission != MISSION_GOTOCOORDS && pVehicle->AutoPilot.m_nCarMission != MISSION_GOTOCOORDS_STRAIGHT) {
		pVehicle->AutoPilot.m_nDrivingStyle = DRIVINGSTYLE_AVOID_CARS;
		pVehicle->AutoPilot.m_nTempAction = TEMPACT_NONE;
		pVehicle->AutoPilot.m_nCruiseSpeed = 20;
		pVehicle->AutoPilot.m_nAntiReverseTimer = CTimer::GetTimeInMilliseconds();
		pVehicle->SetStatus(STATUS_PHYSICS);
		pVehicle->AutoPilot.m_nCarMission = (CCarCtrl::JoinCarWithRoadSystemGotoCoors(pVehicle, *pTarget, false)) ?
			MISSION_GOTOCOORDS_STRAIGHT : MISSION_GOTOCOORDS;
	}
	else if (Abs(pTarget->x - pVehicle->AutoPilot.m_vecDestinationCoors.x) > 2.0f ||
		Abs(pTarget->y - pVehicle->AutoPilot.m_vecDestinationCoors.y) > 2.0f) {
		pVehicle->AutoPilot.m_vecDestinationCoors = *pTarget;
	}
	return (pVehicle->GetPosition() - *pTarget).Magnitude2D();
}

float CCarAI::GetCarToParkAtCoors(CVehicle* pVehicle, CVector* pTarget)
{
	GetCarToGoToCoors(pVehicle, pTarget);
	pVehicle->bParking = true;
	pVehicle->AutoPilot.m_nCruiseSpeed = 10;
	return (pVehicle->GetPosition() - *pTarget).Magnitude2D();
}

void CCarAI::AddPoliceCarOccupants(CVehicle* pVehicle)
{
	if (pVehicle->bOccupantsHaveBeenGenerated)
		return;
	pVehicle->bOccupantsHaveBeenGenerated = true;
	switch (pVehicle->GetModelIndex()) {
	case MI_FBIRANCH:
	case MI_ENFORCER:
		pVehicle->SetUpDriver();
		for (int i = 0; i < 3; i++)
			pVehicle->SetupPassenger(i);
		return;
	case MI_POLICE:
	case MI_RHINO:
	case MI_BARRACKS:
		pVehicle->SetUpDriver();
		if (FindPlayerPed()->m_pWanted->GetWantedLevel() > 1)
			pVehicle->SetupPassenger(0);
		return;
	case MI_PREDATOR:
		pVehicle->SetUpDriver();
		return;
	case MI_VICECHEE:
	{
		pVehicle->SetUpDriver()->bMiamiViceCop = true;
		pVehicle->SetupPassenger(0)->bMiamiViceCop = true;
		CPopulation::NumMiamiViceCops += 2;
		CCarCtrl::MiamiViceCycle = (CCarCtrl::MiamiViceCycle + 1) % 4;
		CCarCtrl::LastTimeMiamiViceGenerated = CTimer::GetTimeInMilliseconds();
		return;
	}
	default:
		return;
	}
}

void CCarAI::AddAmbulanceOccupants(CVehicle* pVehicle)
{
	pVehicle->SetUpDriver();
	pVehicle->SetupPassenger(1);
}

void CCarAI::AddFiretruckOccupants(CVehicle* pVehicle)
{
	pVehicle->SetUpDriver();
	pVehicle->SetupPassenger(0);
}

void CCarAI::TellOccupantsToLeaveCar(CVehicle* pVehicle)
{
	if (pVehicle->pDriver) {
		pVehicle->pDriver->SetObjective(OBJECTIVE_LEAVE_CAR, pVehicle);
		switch (pVehicle->GetModelIndex()) {
		case MI_FIRETRUCK:
		case MI_FBICAR:
		case MI_ENFORCER:
		case MI_BARRACKS:
		case MI_RHINO:
		case MI_POLICE:
			break;
		case MI_AMBULAN:
			pVehicle->pDriver->Say(SOUND_PED_LEAVE_VEHICLE);
			break;
		}
	}
	int timer = 100;
	for (int i = 0; i < pVehicle->m_nNumMaxPassengers; i++) {
		if (pVehicle->pPassengers[i]) {
			pVehicle->pPassengers[i]->m_leaveCarTimer = timer;
			pVehicle->pPassengers[i]->SetObjective(OBJECTIVE_LEAVE_CAR, pVehicle);
			timer += CGeneral::GetRandomNumberInRange(200, 400);
		}
	}
}

void CCarAI::TellOccupantsToFleeCar(CVehicle* pVehicle)
{
	if (pVehicle->pDriver && !pVehicle->pDriver->IsPlayer()) {
		pVehicle->pDriver->SetObjective(OBJECTIVE_FLEE_ON_FOOT_TILL_SAFE);
		switch (pVehicle->GetModelIndex()) {
		case MI_FIRETRUCK:
		case MI_FBIRANCH:
		case MI_ENFORCER:
		case MI_BARRACKS:
		case MI_RHINO:
		case MI_POLICE:
			break;
		case MI_AMBULAN:
			pVehicle->pDriver->Say(SOUND_PED_LEAVE_VEHICLE);
			break;
		}
	}
	int timer = 100;
	for (int i = 0; i < pVehicle->m_nNumMaxPassengers; i++) {
		if (pVehicle->pPassengers[i]) {
			pVehicle->pPassengers[i]->m_leaveCarTimer = timer;
			pVehicle->pPassengers[i]->SetObjective(OBJECTIVE_FLEE_ON_FOOT_TILL_SAFE);
			timer += CGeneral::GetRandomNumberInRange(200, 400);
		}
	}
}

void CCarAI::TellCarToRamOtherCar(CVehicle* pVehicle, CVehicle* pTarget)
{
	pVehicle->AutoPilot.m_pTargetCar = pTarget;
	pTarget->RegisterReference((CEntity**)&pVehicle->AutoPilot.m_pTargetCar);
	pVehicle->AutoPilot.m_nCarMission = MISSION_RAMCAR_FARAWAY;
	pVehicle->bEngineOn = true;
	pVehicle->AutoPilot.m_nCruiseSpeed = Max(6, pVehicle->AutoPilot.m_nCruiseSpeed);
}

void CCarAI::TellCarToBlockOtherCar(CVehicle* pVehicle, CVehicle* pTarget)
{
	pVehicle->AutoPilot.m_pTargetCar = pTarget;
	pTarget->RegisterReference((CEntity**)&pVehicle->AutoPilot.m_pTargetCar);
	pVehicle->AutoPilot.m_nCarMission = MISSION_BLOCKCAR_FARAWAY;
	pVehicle->bEngineOn = true;
	pVehicle->AutoPilot.m_nCruiseSpeed = Max(6, pVehicle->AutoPilot.m_nCruiseSpeed);
}

uint8 CCarAI::FindPoliceCarMissionForWantedLevel()
{
	switch (CWorld::Players[CWorld::PlayerInFocus].m_pPed->m_pWanted->GetWantedLevel()) {
	case 0:
	case 1: return MISSION_BLOCKPLAYER_FARAWAY;
	case 2: return (CGeneral::GetRandomNumber() & 3) >= 3 ? MISSION_RAMPLAYER_FARAWAY : MISSION_BLOCKPLAYER_FARAWAY;
	case 3: return (CGeneral::GetRandomNumber() & 3) >= 2 ? MISSION_RAMPLAYER_FARAWAY : MISSION_BLOCKPLAYER_FARAWAY;
	case 4:
	case 5:
	case 6: return (CGeneral::GetRandomNumber() & 3) >= 1 ? MISSION_RAMPLAYER_FARAWAY : MISSION_BLOCKPLAYER_FARAWAY;
	default: return MISSION_BLOCKPLAYER_FARAWAY;
	}
}

uint8 CCarAI::FindPoliceBoatMissionForWantedLevel()
{
	switch (CWorld::Players[CWorld::PlayerInFocus].m_pPed->m_pWanted->GetWantedLevel()) {
	case 0:
	case 1: return MISSION_BLOCKPLAYER_FARAWAY;
	case 2:
	case 3:
	case 4:
	case 5:
	case 6: return MISSION_ATTACKPLAYER;
	default: return MISSION_BLOCKPLAYER_FARAWAY;
	}
}

int32 CCarAI::FindPoliceCarSpeedForWantedLevel(CVehicle* pVehicle)
{
	switch (CWorld::Players[CWorld::PlayerInFocus].m_pPed->m_pWanted->GetWantedLevel()) {
	case 0: return CGeneral::GetRandomNumberInRange(12, 16);
	case 1: return 25;
	case 2: return 34;
	case 3: return GAME_SPEED_TO_CARAI_SPEED * pVehicle->pHandling->Transmission.fMaxVelocity * 0.9f;
	case 4: return GAME_SPEED_TO_CARAI_SPEED * pVehicle->pHandling->Transmission.fMaxVelocity * 1.2f;
	case 5: return GAME_SPEED_TO_CARAI_SPEED * pVehicle->pHandling->Transmission.fMaxVelocity * 1.25f;
	case 6: return GAME_SPEED_TO_CARAI_SPEED * pVehicle->pHandling->Transmission.fMaxVelocity * 1.3f;
	default: return 0;
	}
}

void CCarAI::MellowOutChaseSpeed(CVehicle* pVehicle)
{
	if (CWorld::Players[CWorld::PlayerInFocus].m_pPed->m_pWanted->GetWantedLevel() == 1) {
		float distanceToPlayer = (pVehicle->GetPosition() - FindPlayerCoors()).Magnitude();
		if (FindPlayerVehicle()) {
			if (distanceToPlayer < 10.0f)
				pVehicle->AutoPilot.m_nCruiseSpeed = 15;
			else if (distanceToPlayer < 20.0f)
				pVehicle->AutoPilot.m_nCruiseSpeed = 22;
			else
				pVehicle->AutoPilot.m_nCruiseSpeed = 25;
		}
		else {
			if (distanceToPlayer < 20.0f)
				pVehicle->AutoPilot.m_nCruiseSpeed = 5;
			else if (distanceToPlayer < 40.0f)
				pVehicle->AutoPilot.m_nCruiseSpeed = 13;
			else
				pVehicle->AutoPilot.m_nCruiseSpeed = 25;
		}
	}
	else if (CWorld::Players[CWorld::PlayerInFocus].m_pPed->m_pWanted->GetWantedLevel() == 2) {
		float distanceToPlayer = (pVehicle->GetPosition() - FindPlayerCoors()).Magnitude();
		if (FindPlayerVehicle()) {
			if (distanceToPlayer < 10.0f)
				pVehicle->AutoPilot.m_nCruiseSpeed = 27;
			else if (distanceToPlayer < 20.0f)
				pVehicle->AutoPilot.m_nCruiseSpeed = 30;
			else
				pVehicle->AutoPilot.m_nCruiseSpeed = 34;
		}
		else {
			if (distanceToPlayer < 20.0f)
				pVehicle->AutoPilot.m_nCruiseSpeed = 5;
			else if (distanceToPlayer < 40.0f)
				pVehicle->AutoPilot.m_nCruiseSpeed = 18;
			else
				pVehicle->AutoPilot.m_nCruiseSpeed = 34;
		}
	}
	if (!FindPlayerVehicle() && FindPlayerPed()->GetMoveSpeed().Magnitude() < 0.07f) {
		if ((FindPlayerCoors() - pVehicle->GetPosition()).Magnitude() < 30.0f)
			pVehicle->AutoPilot.m_nCruiseSpeed = Min(10, pVehicle->AutoPilot.m_nCruiseSpeed);
	}
}

void CCarAI::MellowOutChaseSpeedBoat(CVehicle* pVehicle)
{
	switch (CWorld::Players[CWorld::PlayerInFocus].m_pPed->m_pWanted->GetWantedLevel()) {
	case 0: pVehicle->AutoPilot.m_nCruiseSpeed = 8; break;
	case 1: pVehicle->AutoPilot.m_nCruiseSpeed = 10; break;
	case 2: pVehicle->AutoPilot.m_nCruiseSpeed = 15; break;
	case 3: pVehicle->AutoPilot.m_nCruiseSpeed = 20; break;
	case 4: pVehicle->AutoPilot.m_nCruiseSpeed = 25; break;
	case 5: pVehicle->AutoPilot.m_nCruiseSpeed = 30; break;
	case 6: pVehicle->AutoPilot.m_nCruiseSpeed = 40; break;
	}
}

void CCarAI::MakeWayForCarWithSiren(CVehicle *pVehicle)
{
	float flatSpeedSqr = GetMagSqr2D(pVehicle->GetMoveSpeed());
	if (flatSpeedSqr < 0.01f) // 0.1^2
		return;

	float flatSpeed = Sqrt(flatSpeedSqr);
	CVector2D forward = pVehicle->GetMoveSpeed() / flatSpeed;
	float projection = flatSpeed * 45.0f + 20.0f;
	float projectionSqr = projection * projection;

	int i = CPools::GetVehiclePool()->GetSize();
	while (--i >= 0) {
		CVehicle* vehicle = CPools::GetVehiclePool()->GetSlot(i);
		if (!vehicle)
			continue;
		if (!vehicle->IsCar() && !vehicle->IsBike())
			continue;
		if (vehicle->GetStatus() != STATUS_SIMPLE && vehicle->GetStatus() != STATUS_PHYSICS)
			continue;
		if (vehicle->VehicleCreatedBy != RANDOM_VEHICLE)
			continue;
		if (vehicle->bIsLawEnforcer || vehicle->bIsAmbulanceOnDuty || vehicle->bIsFireTruckOnDuty)
			continue;
		if (vehicle == pVehicle)
			continue;
		if (vehicle->AutoPilot.m_nDrivingStyle == DRIVINGSTYLE_AVOID_CARS)
			return;
		if (Abs(pVehicle->GetPosition().z - vehicle->GetPosition().z) >= 5.0f)
			continue;

		CVector2D distance = vehicle->GetPosition() - pVehicle->GetPosition();

		if (Abs(distance.x) + Abs(distance.y) > projection * 1.5f)
			continue;

		float distSqr = GetMagSqr2D(distance);
		if (distSqr >= projectionSqr)
			continue;
		if (GetMagSqr2D(vehicle->GetMoveSpeed()) <= 0.0025f) // 0.05^2
			continue;

		float dotProd = DotProduct2D(forward, distance);
		if (dotProd <= 0.0f)
			continue;

		if ((dotProd * dotProd / distSqr) > 0.64f && DotProduct2D(forward, vehicle->GetForward()) > 0.7f) {
			if (vehicle->AutoPilot.m_nTempAction != TEMPACT_SWERVELEFT && vehicle->AutoPilot.m_nTempAction != TEMPACT_SWERVERIGHT) {
				vehicle->AutoPilot.m_nTempAction = (distance.x * forward.y - distance.y * forward.x > 0.0f) ?
					TEMPACT_SWERVELEFT : TEMPACT_SWERVERIGHT;
				vehicle->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 2000;
			}
			vehicle->SetStatus(STATUS_PHYSICS);
		}
		else {
			if (DotProduct2D(vehicle->GetMoveSpeed(), distance) < 0.0f && vehicle->AutoPilot.m_nTempAction != TEMPACT_WAIT) {
				vehicle->AutoPilot.m_nTempAction = TEMPACT_WAIT;
				vehicle->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 2000;
			}
		}
	}
}
