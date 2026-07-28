#pragma once
#include "types.h"

bool spg_Init();
void spg_Term();
void spg_Reset(bool Manual);

void CalculateSync();
void read_lightgun_position(int x, int y);
u32 spg_vblank_count();
struct TA_context;
void SetREP(TA_context* cntx);
