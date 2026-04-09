#include "common.h"

#include "Weather.h"

#include "Camera.h"
#include "Clock.h"
#include "CutsceneMgr.h"
#include "DMAudio.h"
#include "General.h"
#include "Pad.h"
#include "PlayerPed.h"
#include "Particle.h"
#include "RenderBuffer.h"
#include "Stats.h"
#include "Shadows.h"
#include "Timecycle.h"
#include "Timer.h"
#include "Vehicle.h"
#include "World.h"
#include "ZoneCull.h"
#include "SpecialFX.h"
#include "Replay.h"

int32 CWeather::SoundHandle = -1;

int32 CWeather::WeatherTypeInList;
int16 CWeather::OldWeatherType;
int16 CWeather::NewWeatherType;
int16 CWeather::ForcedWeatherType;

bool CWeather::LightningFlash;
bool CWeather::LightningBurst;
uint32 CWeather::LightningStart;
uint32 CWeather::LightningFlashLastChange;
uint32 CWeather::WhenToPlayLightningSound;
uint32 CWeather::LightningDuration;
int32 CWeather::StreamAfterRainTimer;

float CWeather::ExtraSunnyness;
float CWeather::Foggyness;
float CWeather::CloudCoverage;
float CWeather::Wind;
float CWeather::Rain;
float CWeather::InterpolationValue;
float CWeather::WetRoads;
float CWeather::Rainbow;
float CWeather::SunGlare;
float CWeather::WindClipped;
float CWeather::TrafficLightBrightness;

bool CWeather::bScriptsForceRain;

tRainStreak Streaks[NUM_RAIN_STREAKS];

int16 WeatherTypesList[] = {
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY,
	WEATHER_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY,
	WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY,
	WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_CLOUDY,
	WEATHER_RAINY, WEATHER_RAINY, WEATHER_RAINY, WEATHER_RAINY,
	WEATHER_CLOUDY, WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY,
	WEATHER_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY
};

int16 WeatherTypesList_WithHurricanes[] = {
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY,
	WEATHER_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_CLOUDY,
	WEATHER_HURRICANE, WEATHER_HURRICANE, WEATHER_CLOUDY, WEATHER_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY,
	WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY,
	WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY,
	WEATHER_CLOUDY, WEATHER_HURRICANE, WEATHER_HURRICANE, WEATHER_HURRICANE,
	WEATHER_CLOUDY, WEATHER_SUNNY, WEATHER_SUNNY, WEATHER_SUNNY,
	WEATHER_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY,
	WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY, WEATHER_EXTRA_SUNNY
};

const float Windyness[] = {
	0.25f,// WEATHER_SUNNY
	0.7f, // WEATHER_CLOUDY
	1.0f, // WEATHER_RAINY
	0.0f, // WEATHER_FOGGY
	0.0f, // WEATHER_EXTRA_SUNNY
	2.0f, // WEATHER_HURRICANE
	0.0f
};

#define MIN_TIME_BETWEEN_LIGHTNING_FLASH_CHANGES (50)

void CWeather::Init(void)
{
	NewWeatherType = WEATHER_EXTRA_SUNNY;
	bScriptsForceRain = false;
	OldWeatherType = WEATHER_EXTRA_SUNNY;
	InterpolationValue = 0.0f;
	WhenToPlayLightningSound = 0;
	WeatherTypeInList = 0;
	ForcedWeatherType = WEATHER_RANDOM;
	SoundHandle = DMAudio.CreateEntity(AUDIOTYPE_WEATHER, (void*)1);
	if (SoundHandle >= 0)
		DMAudio.SetEntityStatus(SoundHandle, TRUE);
}

void CWeather::Update(void)
{
	if (!CReplay::IsPlayingBack()) {
		float fNewInterpolation = (CClock::GetMinutes() + CClock::GetSeconds() / 60.0f) / 60.0f;
		if (fNewInterpolation < InterpolationValue) {
			OldWeatherType = NewWeatherType;
			if (ForcedWeatherType >= 0)
				NewWeatherType = ForcedWeatherType;
			else {
				WeatherTypeInList = (WeatherTypeInList + 1) % ARRAY_SIZE(WeatherTypesList);
				NewWeatherType = CStats::NoMoreHurricanes ? WeatherTypesList[WeatherTypeInList] : WeatherTypesList_WithHurricanes[WeatherTypeInList];
			}
		}
		InterpolationValue = fNewInterpolation;
	}

#ifndef FINAL
	if (CPad::GetPad(1)->GetRightShockJustDown()) {
		NewWeatherType = (NewWeatherType + 1) % WEATHER_TOTAL;
		OldWeatherType = NewWeatherType;
	}
#endif

	// Lightning logic (Kept strictly for the screen flash effect and sound)
	if (NewWeatherType != WEATHER_RAINY || OldWeatherType != WEATHER_RAINY) {
		LightningFlash = false;
		LightningBurst = false;
	}
	else {
		if (LightningBurst) {
			if ((CGeneral::GetRandomNumber() & 255) >= 32) {
				if (CTimer::GetTimeInMilliseconds() - LightningFlashLastChange > MIN_TIME_BETWEEN_LIGHTNING_FLASH_CHANGES) {
					bool bOldLightningFlash = LightningFlash;
					LightningFlash = CGeneral::GetRandomTrueFalse();
					if (LightningFlash != bOldLightningFlash)
						LightningFlashLastChange = CTimer::GetTimeInMilliseconds();
				}
			}
			else {
				LightningBurst = false;
				LightningDuration = Min(CTimer::GetFrameCounter() - LightningStart, 20);
				LightningFlash = false;
				WhenToPlayLightningSound = CTimer::GetTimeInMilliseconds() + 150 * (20 - LightningDuration);
			}
		}
		else {
			if (CGeneral::GetRandomNumber() >= 200) {
				LightningFlash = false;
			}
			else {
				LightningBurst = true;
				LightningStart = CTimer::GetFrameCounter();
				LightningFlashLastChange = CTimer::GetTimeInMilliseconds();
				LightningFlash = true;
			}
		}
	}
	if (WhenToPlayLightningSound && CTimer::GetTimeInMilliseconds() > WhenToPlayLightningSound) {
		DMAudio.PlayOneShot(SoundHandle, SOUND_LIGHTNING, LightningDuration);
		CPad::GetPad(0)->StartShake(40 * LightningDuration + 100, 2 * LightningDuration + 80);
		WhenToPlayLightningSound = 0;
	}

	// Weather interpolation calculations
	if (OldWeatherType == WEATHER_RAINY || OldWeatherType == WEATHER_HURRICANE) {
		if (NewWeatherType == WEATHER_RAINY || NewWeatherType == WEATHER_HURRICANE) WetRoads = 1.0f;
		else WetRoads = 1.0f - InterpolationValue;
	}
	else {
		if (NewWeatherType == WEATHER_RAINY || NewWeatherType == WEATHER_HURRICANE) WetRoads = InterpolationValue;
		else WetRoads = 0.0f;
	}

	float fNewRain;
	if (NewWeatherType == WEATHER_RAINY || NewWeatherType == WEATHER_HURRICANE) {
		fNewRain = (((uint16)CTimer::GetTimeInMilliseconds() >> 14) & 1) * 0.33f;
		if (OldWeatherType != WEATHER_RAINY && OldWeatherType != WEATHER_HURRICANE) {
			if (InterpolationValue < 0.4f) fNewRain = 0.5f;
			else fNewRain = 0.25f + (((uint16)CTimer::GetTimeInMilliseconds() >> 14) & 1) * 0.25f;
		}
		fNewRain = Max(fNewRain, 0.5f);
	}
	else fNewRain = 0.0f;
	Rain = fNewRain;

	if (OldWeatherType != WEATHER_SUNNY && OldWeatherType != WEATHER_EXTRA_SUNNY) CloudCoverage = 1.0f - InterpolationValue;
	else CloudCoverage = 0.0f;
	if (NewWeatherType != WEATHER_SUNNY && OldWeatherType != WEATHER_EXTRA_SUNNY) CloudCoverage += InterpolationValue;

	if (OldWeatherType == WEATHER_FOGGY) Foggyness = 1.0f - InterpolationValue;
	else Foggyness = 0.0f;
	if (NewWeatherType == WEATHER_FOGGY) Foggyness += InterpolationValue;

	if (OldWeatherType == WEATHER_EXTRA_SUNNY) ExtraSunnyness = 1.0f - InterpolationValue;
	else ExtraSunnyness = 0.0f;
	if (NewWeatherType == WEATHER_EXTRA_SUNNY) ExtraSunnyness += InterpolationValue;

	// --- OPTIMIZACIÓN EXTREMA: Arcoíris y Resplandor del Sol Eliminados ---
	// Matamos el cálculo de ángulo del sol y números aleatorios (Ahorro CPU)
	Rainbow = 0.0f;
	SunGlare = 0.0f;
	// ----------------------------------------------------------------------

	Wind = InterpolationValue * Windyness[NewWeatherType] + (1.0f - InterpolationValue) * Windyness[OldWeatherType];
	WindClipped = Min(1.0f, Wind);

	if (CClock::GetHours() > 20) TrafficLightBrightness = 1.0f;
	else if (CClock::GetHours() > 19) TrafficLightBrightness = CClock::GetMinutes() / 60.0f;
	else if (CClock::GetHours() > 6) TrafficLightBrightness = 0.0f;
	else if (CClock::GetHours() > 5) TrafficLightBrightness = 1.0f - CClock::GetMinutes() / 60.0f;
	else TrafficLightBrightness = 1.0f;

	TrafficLightBrightness = Max(WetRoads, TrafficLightBrightness);
	TrafficLightBrightness = Max(Foggyness, TrafficLightBrightness);
	TrafficLightBrightness = Max(Rain, TrafficLightBrightness);
}

void CWeather::ForceWeather(int16 weather) { ForcedWeatherType = weather; }
void CWeather::ForceWeatherNow(int16 weather) { OldWeatherType = weather; NewWeatherType = weather; ForcedWeatherType = weather; }
void CWeather::ReleaseWeather() { ForcedWeatherType = -1; }

// ====================================================================
// ZONA DE LOBOTOMÍA GRÁFICA
// Todas las funciones generadoras de partículas, gotas, vapor y mosquitos 
// han sido vaciadas para aniquilar el uso de Immediate Mode y el fillrate.
// ====================================================================

void CWeather::AddHeatHaze() {}
void CWeather::AddBeastie() {}
void CWeather::AddSplashesDuringHurricane() {}
void CWeather::AddStreamAfterRain() {}
void CWeather::AddRain() {}
void RenderOneRainStreak(CVector pos, CVector unused, int intensity, bool scale, float distance) {}
void CWeather::RenderRainStreaks(void) {}

#ifdef SECUROM
void CWeather::ForceHurricaneWeather()
{
	for (int i = 0; i < ARRAY_SIZE(WeatherTypesList_WithHurricanes); i++)
	{
		WeatherTypesList[i] = WEATHER_HURRICANE;
		WeatherTypesList_WithHurricanes[i] = WEATHER_HURRICANE;
	}
	CWeather::OldWeatherType = WEATHER_HURRICANE;
	CWeather::NewWeatherType = WEATHER_HURRICANE;
	CWeather::ForcedWeatherType = WEATHER_HURRICANE;
}
#endif