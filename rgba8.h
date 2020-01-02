#pragma once

#include <stdint.h>
#include "platform.h"

void Convert_UI32_AABBGGRR(uint32_t* buf, A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf);
void Convert_UI32_AARRGGBB(uint32_t* buf, A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf);
void Convert_UL_AARRGGBB(unsigned long* buf, A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf);

void ConvertLuminance_UI32_LLZZYYXX(uint32_t* buf, const uint8_t xyz[3], A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf);
