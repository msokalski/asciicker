#pragma once

#include <stdint.h>
#include "platform.h"

bool TermOpen(A3D_WND* share, float yaw, float pos[3]);
void TermCloseAll();
void TermResizeAll();