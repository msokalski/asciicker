#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

struct SpriteReq;

bool InitAudio();
void FreeAudio();

void CallAudio(const uint8_t* data, int size);
void AudioWalk(int foot, int volume, const SpriteReq* req, int material);

enum AUDIO_FILE
{

    // merge them all into single file, use in file markers
    // prepare similar files for several armor levels

    JUMP_LAND_DIRT,
    WALK_LEFT_DIRT,
    WALK_RIGHT_DIRT,

    AUDIO_FILES
};

int GetSampleID(AUDIO_FILE af);


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
