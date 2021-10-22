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

    FOREST,
    FOOTSTEPS,
    /*
    WALK_ROCK_L,
    WALK_ROCK_R,
    WALK_WOOD_L,
    WALK_WOOD_R,
    WALK_DIRT_L,
    WALK_DIRT_R,
    WALK_GRASS_L,
    WALK_GRASS_R,
    WALK_BUSH_L,
    WALK_BUSH_R,
    WALK_BLOOD_L,
    WALK_BLOOD_R,
    WALK_WATER_L,
    WALK_WATER_R,
    */


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
