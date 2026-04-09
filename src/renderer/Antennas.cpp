#include "common.h"
#include "main.h"
#include "Antennas.h"

CAntenna CAntennas::aAntennas[NUMANTENNAS];

void CAntennas::Init(void) {}
void CAntennas::Update(void) {}
void CAntennas::RegisterOne(uint32, CVector, CVector, float) {}
void CAntennas::Render(void) {}
void CAntenna::Update(CVector, CVector) {}