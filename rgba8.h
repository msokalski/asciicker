#pragma once

#include <stdint.h>
#include "asciiid_platform.h"

void Convert_RGBA8(uint32_t* buf, A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf);
void ConvertLuminanceToAlpha_RGBA8(uint32_t* buf, const uint8_t rgb[3], A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf);
