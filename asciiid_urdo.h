#pragma once

#include  <stdint.h>
#include "terrain.h"


bool URDO_CanUndo();
bool URDO_CanRedo();

size_t URDO_Bytes();

void URDO_Purge();

void URDO_Undo(int max_depth);
void URDO_Redo(int max_depth);

// groupping
void URDO_Open();
void URDO_Close();

// patches
void URDO_Patch(Patch* p);
void URDO_Diag(Patch* p);
