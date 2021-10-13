#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

typedef void (*AudioCB)(void* userdata, int16_t stereo_buffer[], int samples);
bool InitAudio(AudioCB cb, void* userdata);
void FreeAudio();

int GetAudioLatency();

#endif
