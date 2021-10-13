#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

typedef void (*AudioCB)(void* userdata, int16_t stereo_buffer[], int samples);
bool InitAudio(AudioCB cb, void* userdata);
void FreeAudio();

int GetAudioLatency();


// is this right direction?
/*
void SetAudioSteps(int num_steps, int contact);
void SetAudioWingsFreq(int freq);
void SetAudioSwoosh(int speed, int contact);
void SetAudioJump(int contact, bool end);
void SetAudioShoot(int contact);
void SetAudioWater(int height, int speed);
*/

#endif
