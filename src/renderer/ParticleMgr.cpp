#include "common.h"

#include "main.h"
#include "FileMgr.h"
#include "ParticleMgr.h"

cParticleSystemMgr mod_ParticleSystemManager;

const char *ParticleFilename = "PARTICLE.CFG";

cParticleSystemMgr::cParticleSystemMgr()
{
	memset(this, 0, sizeof(*this));
}

void cParticleSystemMgr::Initialise()
{
	LoadParticleData();

	for (int32 i = 0; i < MAX_PARTICLES; i++)
		m_aParticles[i].m_pParticles = nil;
}

void cParticleSystemMgr::LoadParticleData()
{
	CFileMgr::SetDir("DATA");
	CFileMgr::LoadFile(ParticleFilename, work_buff, ARRAY_SIZE(work_buff), "r");
	CFileMgr::SetDir("");

	tParticleSystemData *entry = nil;
	int32 type = PARTICLE_FIRST;

	char *lineStart = (char *)work_buff;
	char *lineEnd = lineStart + 1;

	char line[500];
	char delims[4];

	while (true)
	{
		while (*lineEnd != '\n' && *lineEnd != '\0')
			++lineEnd;

		int32 lineLength = lineEnd - lineStart;

		if (lineLength >= 500) lineLength = 499;

		strncpy(line, lineStart, lineLength);
		line[lineLength] = '\0';

		if (!strcmp(line, ";the end") || *lineEnd == '\0')
			break;

		if (*line != ';' && lineLength > 3)
		{
			int32 param = CFG_PARAM_FIRST;
			strcpy(delims, " \t");
			char *value = strtok(line, delims);

			if (value)
			{
				do
				{
					switch (param)
					{
					case CFG_PARAM_PARTICLE_TYPE_NAME:
						entry = &m_aParticles[type];
						entry->m_Type = (tParticleType)type++;
						strcpy(entry->m_aName, value);
						break;

					case CFG_PARAM_RENDER_COLOURING_R: entry->m_RenderColouring.red = atoi(value); break;
					case CFG_PARAM_RENDER_COLOURING_G: entry->m_RenderColouring.green = atoi(value); break;
					case CFG_PARAM_RENDER_COLOURING_B: entry->m_RenderColouring.blue = atoi(value); break;

					case CFG_PARAM_INITIAL_COLOR_VARIATION:
						// Optimización: Forzamos 0 variación para evitar cálculos de Random() en render
						entry->m_InitialColorVariation = 0;
						break;

					case CFG_PARAM_FADE_DESTINATION_COLOR_R: entry->m_FadeDestinationColor.red = atoi(value); break;
					case CFG_PARAM_FADE_DESTINATION_COLOR_G: entry->m_FadeDestinationColor.green = atoi(value); break;
					case CFG_PARAM_FADE_DESTINATION_COLOR_B: entry->m_FadeDestinationColor.blue = atoi(value); break;

					case CFG_PARAM_COLOR_FADE_TIME: entry->m_ColorFadeTime = atoi(value); break;
					case CFG_PARAM_DEFAULT_INITIAL_RADIUS: entry->m_fDefaultInitialRadius = (float)atof(value); break;
					case CFG_PARAM_EXPANSION_RATE: entry->m_fExpansionRate = (float)atof(value); break;
					case CFG_PARAM_INITIAL_INTENSITY: entry->m_nFadeToBlackInitialIntensity = atoi(value); break;
					case CFG_PARAM_FADE_TIME: entry->m_nFadeToBlackTime = atoi(value); break;
					case CFG_PARAM_FADE_AMOUNT: entry->m_nFadeToBlackAmount = atoi(value); break;
					case CFG_PARAM_INITIAL_ALPHA_INTENSITY: entry->m_nFadeAlphaInitialIntensity = atoi(value); break;
					case CFG_PARAM_FADE_ALPHA_TIME: entry->m_nFadeAlphaTime = atoi(value); break;
					case CFG_PARAM_FADE_ALPHA_AMOUNT: entry->m_nFadeAlphaAmount = atoi(value); break;
					case CFG_PARAM_INITIAL_ANGLE: entry->m_nZRotationInitialAngle = atoi(value); break;
					case CFG_PARAM_CHANGE_TIME: entry->m_nZRotationChangeTime = atoi(value); break;
					case CFG_PARAM_ANGLE_CHANGE_AMOUNT: entry->m_nZRotationAngleChangeAmount = atoi(value); break;
					case CFG_PARAM_INITIAL_Z_RADIUS: entry->m_fInitialZRadius = (float)atof(value); break;
					case CFG_PARAM_Z_RADIUS_CHANGE_TIME: entry->m_nZRadiusChangeTime = atoi(value); break;
					case CFG_PARAM_Z_RADIUS_CHANGE_AMOUNT: entry->m_fZRadiusChangeAmount = (float)atof(value); break;
					case CFG_PARAM_ANIMATION_SPEED: entry->m_nAnimationSpeed = atoi(value); break;
					case CFG_PARAM_START_ANIMATION_FRAME: entry->m_nStartAnimationFrame = atoi(value); break;
					case CFG_PARAM_FINAL_ANIMATION_FRAME: entry->m_nFinalAnimationFrame = atoi(value); break;
					case CFG_PARAM_ROTATION_SPEED: entry->m_nRotationSpeed = atoi(value); break;
					case CFG_PARAM_GRAVITATIONAL_ACCELERATION: entry->m_fGravitationalAcceleration = (float)atof(value); break;
					case CFG_PARAM_FRICTION_DECCELERATION: entry->m_nFrictionDecceleration = atoi(value); break;
					case CFG_PARAM_LIFE_SPAN: entry->m_nLifeSpan = atoi(value); break;

					case CFG_PARAM_POSITION_RANDOM_ERROR:
					case CFG_PARAM_VELOCITY_RANDOM_ERROR:
					case CFG_PARAM_EXPANSION_RATE_ERROR:
					case CFG_PARAM_ROTATION_RATE_ERROR:
						// Optimización Extrema: Eliminamos el factor de error aleatorio para CPU antigua
						*((float*)&entry->m_fPositionRandomError + (param - CFG_PARAM_POSITION_RANDOM_ERROR)) = 0.0f;
						break;

					case CFG_PARAM_LIFE_SPAN_ERROR_SHAPE: entry->m_nLifeSpanErrorShape = atoi(value); break;
					case CFG_PARAM_TRAIL_LENGTH_MULTIPLIER: entry->m_fTrailLengthMultiplier = (float)atof(value); break;
					case CFG_PARAM_STRETCH_VALUE_X: entry->m_vecTextureStretch.x = (float)atof(value); break;
					case CFG_PARAM_STRETCH_VALUE_Y: entry->m_vecTextureStretch.y = (float)atof(value); break;

					case CFG_PARAM_WIND_FACTOR:
						// Muerte al factor de viento localizado para partículas
						entry->m_fWindFactor = 0.0f;
						break;

					case CFG_PARAM_PARTICLE_CREATE_RANGE:
						entry->m_fCreateRange = SQR((float)atof(value));
						break;

					case CFG_PARAM_FLAGS:
						entry->Flags = atoi(value);
						break;
					}

					value = strtok(nil, delims);
					param++;
					if (param > CFG_PARAM_LAST) param = CFG_PARAM_FIRST;

				} while (value != nil);
			}
		}

		lineEnd++;
		lineStart = lineEnd;
		lineEnd++;
	}
}