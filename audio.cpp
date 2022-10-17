


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "fast_rand.h"
#include "audio.h"
#include "game.h" // SpriteReq for audio props

#include "stb_vorbis.h"

#define SEMI_TONE(x,y) x*pow(2.0,y/12.0)

#define PLY_TRACKS 16

void AudioWalk(int foot, int volume, const SpriteReq* req, int material)
{
    // remember previous foot timestamp
    // so we can (ex/in)terpolate sub events for req resonanse
    // add some rand delay to each sub event

    // get sample id for given: foot, req, material 

    static int land = 0;
    if (foot==0)
    {
        land ^= 1;
        foot = land;
    }

    // int sample = GetSampleID((AUDIO_FILE)(2*material+(foot&1))); // temp!
    int sample = GetSampleID(FOOTSTEPS);
    int sample_chunk = 2*material+(foot&1);

    static int track = 0;
	int32_t data[] = { track, sample, volume, sample_chunk };
    track++;
    if (track==PLY_TRACKS)
        track=0;

	CallAudio((uint8_t*)data, sizeof(data));
}

#define MAX_SAMPLES 64
// each sample data is prolonged with int32 array containing markers (first value is num of markers)
static int16_t* lib_sample_data[MAX_SAMPLES] = {0}; 
static int lib_sample_len[MAX_SAMPLES] = {0};

extern "C" void XOgg(int index, const uint8_t* data, int ogg_size)
{
    if (index>=MAX_SAMPLES)
        return;

    int err = 0;
    stb_vorbis* ogg = stb_vorbis_open_memory(data,ogg_size,&err,0);
    if (!ogg)
    {
        lib_sample_data[index]=0;
        lib_sample_len[index]=0;
        return;
    }

    int size = stb_vorbis_stream_length_in_samples(ogg);
    int offs = 0;

    stb_vorbis_info nfo = stb_vorbis_get_info(ogg);
    int freq = (int)nfo.sample_rate;
    const char* markers = stb_vorbis_get_markers(ogg);
    int num_markers = 0;
    if (markers)
    {
        const char* ptr = markers;
        while (1)
        {
            if (*ptr>32)
                num_markers++;
            ptr = strchr(ptr,'\n');
            if (!ptr)
                break;
            ptr++;
        }
    }

    int16_t* dec = (int16_t*)malloc(sizeof(int16_t)*2*size + sizeof(int32_t)*(num_markers*2+1));
    int32_t* mrk = (int32_t*)(dec + 2*size) + 1;

    mrk[-1] = num_markers;

    if (markers)
    {
        const char* ptr = markers;
        for (int i=0; i<num_markers; i++)
        {
            float a=0,b=0;
            sscanf(ptr,"%f\t%f", &a,&b);
            {
                ptr = strchr(ptr,'\n');
                ptr++;
            }
            
            mrk[2*i+0] = (int)floor(a*freq+0.5);
            mrk[2*i+1] = (int)floor(b*freq+0.5);
        }
    }

    float** ptr=0;
    int len;
    int chn;

    while ( ( len = stb_vorbis_get_frame_float(ogg, &chn, &ptr) ) )
    {
        if (chn==1)
        {
            // mono -> L=0, R=0
            for (int i=0; i<len; i++)
            {
                if (offs>=2*size)
                {
                    // clip!
                    i=len;
                    break;
                }

                int m = (int)(ptr[0][i]*32767);
                if (m<-32767)
                    m=-32767;
                else
                if (m>+32767)
                    m=+32767;

                dec[offs++] = m;
                dec[offs++] = m;
            }
        }
        else
        {
            // stereo L=0, R=1
            for (int i=0; i<len; i++)
            {
                if (offs>=2*size)
                {
                    // clip!
                    i=len;
                    break;
                }

                int l = (int)(ptr[0][i]*32767);
                if (l<-32767)
                    l=-32767;
                else
                if (l>+32767)
                    l=+32767;

                int r = (int)(ptr[1][i]*32767);
                if (r<-32767)
                    r=-32767;
                else
                if (r>+32767)
                    r=+32767;

                dec[offs++] = l;
                dec[offs++] = r;
            }
        }
    }

    stb_vorbis_close(ogg);

    lib_sample_data[index] = dec;
    lib_sample_len[index] = offs>>1;
}

#ifndef WORKLET
extern char base_path[];

struct SampleHash
{
    SampleHash* next;
    uint32_t hash;
    uint32_t id;
    char name[1];
};

#define HASH_MAKS (MAX_SAMPLES-1)
static SampleHash* sample_hash[HASH_MAKS+1]={0};
static int samples=0;

static int FindSample(const char* name, uint32_t* h, int* l)
{
    if (!name)
        return -1;

    uint32_t hash = 5381;
    const char* n = name;
    while (unsigned int c = *n++)
        hash = ((hash << 5) + hash) + c;

    if (h)
        *h = hash;
    if (l)
        *l = n-name;

    SampleHash* buck = sample_hash[hash&HASH_MAKS];
    while (buck)
    {
        if (buck->hash == hash && strcmp(name,buck->name)==0)
            return buck->id;
        buck = buck->next;
    }

    return -1;
}

#ifdef EMSCRIPTEN
extern "C" void Sample(const char* name)
{
    uint32_t hash;
    int len;
    if (FindSample(name,&hash,&len)<0)
    {
        SampleHash* item = (SampleHash*)malloc(sizeof(SampleHash)+len);
        SampleHash** base = sample_hash + (hash&HASH_MAKS);
        item->next = *base;
        *base = item;
        item->hash = hash;
        item->id = samples;
        strcpy(item->name,name);        
    }
    else
    {
        // name collision!!! how?
    }

    samples++;
}
#endif

static int LoadSample(const char* name)
{
    // if in hashmap, return its id
    uint32_t hash;
    int len;
    int id = FindSample(name,&hash,&len);
    if (id >= 0)
        return id;

    #ifdef EMSCRIPTEN
    return -1; // if not found in hashmap
    #else

    // load & dec file from ./samples/<name>
    char path[1024];
    sprintf(path,"%ssamples/%s",base_path,name);
    FILE* f = fopen(path,"rb");

    if (!f) // if file not found return -1
        return -1;

    if (samples == MAX_SAMPLES)
    {
        fclose(f);
        return -1;
    }

    fseek(f,0,SEEK_END);
    int size = (int)ftell(f);
    fseek(f,0,SEEK_SET);

    uint8_t* data = (uint8_t*)malloc(size);
    int r = fread(data,1,size,f);
    fclose(f);

    // decode ogg
    // if fails it will store lib_sample_data[samples]=0
    XOgg(samples, data, size);

    free(data);

    id = samples++;

    // add to hashmap and return new id
    SampleHash* item = (SampleHash*)malloc(sizeof(SampleHash)+len);
    SampleHash** base = sample_hash + (hash&HASH_MAKS);
    item->next = *base;
    *base = item;
    item->hash = hash;
    item->id = id;
    strcpy(item->name,name);

    return id;
    #endif
}

static const char* sample_names[] = // IN ORDER OF enum AUDIO_FILE
{
    "forest.ogg",
    "footsteps.ogg",
    0
};

static int sample_ids[AUDIO_FILES] = {-1};

static void LoadAllSamples()
{
    for (int i=0; sample_names[i]; i++)
        sample_ids[i] = LoadSample(sample_names[i]);

    int forest_id = GetSampleID(FOREST);
    CallAudio((uint8_t*)&forest_id,4);
}

int GetSampleID(AUDIO_FILE af)
{
    if (af<0 || af>=AUDIO_FILES)
        return -1;
    return sample_ids[af];
}
#endif // END OF NOT WORKLET

struct PlyTrack
{
    int sample_id;
    int sample_vol;
    int sample_pos;
    int sample_end;
};

static PlyTrack ply_track[PLY_TRACKS] = {-1};

static int ply_forest_id = -1;

void DriverAudioCmd(void* userdata, const uint8_t* data, int size)
{
    // testing samples on single track
    // { sample_id, volume }

    if (size==4)
    {
        // very first command
        ply_forest_id = *(int32_t*)data;
        return;
    }

    if (size>=12) // track, sample, vol
    {
        const int* msg = (const int*)data;
        if (msg[0]>=0 && msg[0]<PLY_TRACKS)
        {
            PlyTrack* tr = ply_track + msg[0];
            tr->sample_id = msg[1];
            tr->sample_vol = msg[2];
            tr->sample_pos = 0;
            tr->sample_end = 0;
            
            if (tr->sample_id>=0 && tr->sample_id<MAX_SAMPLES)
                tr->sample_end = lib_sample_len[tr->sample_id];
            else
                tr->sample_id = -1;

            if (tr->sample_id >= 0)
            {
                if (size>=16 && msg[3]>=0)
                {
                    int32_t* marker = (int32_t*)(lib_sample_data[tr->sample_id] + 2*tr->sample_end);
                    if (*marker > msg[3])
                    {
                        marker = marker + 1 + 2 * msg[3];
                        tr->sample_pos = marker[0];
                        tr->sample_end = marker[1];
                    }
                }
            }
        }
    }

    /////////////////////////////////////
    // animables:

    // pan \
    // rot  }-- 2x2 mix-matrix
    // vol /
    // frq

    // replace track sample
    // { 0, sample_id>=0, track_idx, play_from, play_to, loop_a, loop_b>=loop_a, loops, pan, rot, vol, frq, paused}

    // replace transition on track
    // { 1, track_idx, pan_to, rot_to, vol_to, frq_to, time_to }

    // subtract num of remaining loops (clamp to 0)
    // { 2, track_idx, sub_loops }

    // pause track
    // { 3, track_idx }

    // resume track
    // { 4, track_idx }

    // clear track
    // { 5, track_idx }

    // set event to listen if track finishes
    // { 6. track_idx, event_idx, add/remove/replace }

    // set event to listen track's pos & loops
    // { 7. track_idx, event_idx, pos, loops, add/remove/replace }

    // clear all event listenings (on given track or all tracks)
    // { 8. track_idx(-1==ALL), event_idx(-1==ALL) }

    // set event script, script is able to access all internals like sample position, current vol/pan, etc ...
    // { 9. event_idx, [script] (can be null to clear) } 

    // manually trigger event
    // {10. event_idx }

    // pause renderer (ALIAS!)
    // { 3, -1}

    // resume renderer (ALIAS!)
    // { 4, -1}

    // set renderer transition  (ALIAS!)
    // { 1, -1, pan_to, rot_to, vol_to, frq_to, time_to }


    // SCRIPT 'ASSEMBLY'

    // VAR i
    // VAR i=1
    // VAR i,j
    // VAR i=1,j
    // VAR i,j=2
    // VAR i=1,j=2
    // ...

    // FOR(i in TR)
    // FOR(i in EV)

    // TR[i] : sample_id,pos,from,to,loop_a,loop_b,loops,vol,pan,rot,frq
    // EV[i] : listens, listen[j]

    // i=VAL

}

void DriverAudioCB(void* userdata, int16_t buffer[], int frames)
{
    memset(buffer,0,4*frames);

    if (ply_forest_id>=0)
    {
        static int forest_pos = 0;
        int16_t* data = lib_sample_data[ply_forest_id];
        int pos = forest_pos;
        int end = lib_sample_len[ply_forest_id];
        for (int i = 0; i < frames; i++) 
        {
            buffer[2*i] = data[pos*2];
            buffer[2*i+1] = data[pos*2+1];

            pos++;
            if (pos == end)
                pos=0;
        }
        forest_pos = pos;
    }
 
    for (int t=0; t<PLY_TRACKS; t++)
    {
        PlyTrack* tr = ply_track + t;
        if (tr->sample_id < 0)
            continue;
        const int16_t* data = lib_sample_data[tr->sample_id];
        int len = tr->sample_end; //lib_sample_len[tr->sample_id];
        int pos = tr->sample_pos;
        int vol = tr->sample_vol;
        for (int i = 0; i < frames; i++) 
        {
            if (pos==len)
            {
                tr->sample_id=-1;
                break;
            }

            int L = buffer[2*i] + (data[pos*2] * vol) / 65535;
            int R = buffer[2*i+1] + (data[pos*2+1] * vol) / 65535;

            if (L<-32767)
                L=-32767;
            if (L>+32767)
                L=+32767;

            if (R<-32767)
                R=-32767;
            if (R>+32767)
                R=+32767;

            buffer[2*i] = L;
            buffer[2*i+1] = R;
            pos++;
        }
        tr->sample_pos = pos;
    }
}

/////////////////////////////////////

#ifndef EMSCRIPTEN

// for all native builds use mutex synced queue
#include <mutex>

static std::mutex call_mutex;
struct CallQueue
{
    CallQueue* next;
    int size;
    uint8_t data[1];
};

static CallQueue* call_head=0;
static CallQueue* call_tail=0;

static CallQueue* OnAudioCall()
{
    CallQueue* head;
    {
        std::lock_guard<std::mutex> guard(call_mutex);
        head = call_head;
        call_head = 0;
        call_tail = 0;
    }

    return head;
}

void CallAudio(const uint8_t* data, int size)
{
    CallQueue* cq = (CallQueue*)malloc(sizeof(CallQueue)+size-1);
    cq->next = 0;
    cq->size = size;
    memcpy(cq->data,data,size);

    {
        std::lock_guard<std::mutex> guard(call_mutex);
        if (call_tail)
            call_tail->next = cq;
        else
            call_head = cq;
        call_tail = cq;
    }
}
#endif

#ifdef __APPLE__
#ifndef HAS_AUDIO

#include <AudioToolbox/AudioQueue.h>
#include <CoreAudio/CoreAudioTypes.h>

#define NUM_BUFFERS 2
#define BUFFER_SIZE (2048) // full latency 2x512 samples
static AudioQueueRef coreaudio_queue = 0;

void coreaudio_cb(void* userdata, AudioQueueRef queue, AudioQueueBufferRef buffer);

void FreeAudio()
{
    AudioQueueStop(coreaudio_queue, false);
    AudioQueueDispose(coreaudio_queue, false); // deletes its buffers as well
    coreaudio_queue = 0;
}

bool InitAudio()
{
    LoadAllSamples();
    AudioStreamBasicDescription format;

    format.mSampleRate       = 44100;
    format.mFormatID         = kAudioFormatLinearPCM;
    format.mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    format.mBitsPerChannel   = 8 * sizeof(int16_t);
    format.mChannelsPerFrame = 2;
    format.mBytesPerFrame    = sizeof(int16_t) * 2;
    format.mFramesPerPacket  = 1;
    format.mBytesPerPacket   = format.mBytesPerFrame * format.mFramesPerPacket;
    format.mReserved         = 0;

    if (AudioQueueNewOutput(&format, coreaudio_cb, 0, 0, kCFRunLoopCommonModes, 0, &coreaudio_queue))
        return false;

    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        AudioQueueBufferRef buffer;
        if (AudioQueueAllocateBuffer(coreaudio_queue, BUFFER_SIZE, &buffer))
        {
            AudioQueueDispose(coreaudio_queue, true);
            return false;
        }

        buffer->mAudioDataByteSize = BUFFER_SIZE; // why?
        coreaudio_cb(0, coreaudio_queue, buffer);
    }

    if (AudioQueueStart(coreaudio_queue, 0))
    {
        AudioQueueDispose(coreaudio_queue, true);
        return false;
    }
    return true;
}

void coreaudio_cb(void* userdata, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
    int16_t* buf = (int16_t*)buffer->mAudioData;
    int len = BUFFER_SIZE / (sizeof(int16_t)*2);
    
    CallQueue* qc = OnAudioCall();
    while (qc)
    {
        DriverAudioCmd(0,qc->data,qc->size);
        // free it
        CallQueue* n = qc->next;
        free(qc);
        qc = n;
    }
        
    DriverAudioCB(0, buf, len);

    AudioQueueEnqueueBuffer(queue, buffer, 0, 0);
}

#define HAS_AUDIO

#endif
#endif

#ifdef __linux__ 
#ifndef HAS_AUDIO

#include <stdio.h>
#include <assert.h>
#include <pulse/pulseaudio.h>

#include <pthread.h>
#include <sched.h>

#define FORMAT PA_SAMPLE_S16LE
#define RATE 44100
#define CHANNELS 2

void context_state_cb(pa_context* context, void* mainloop);
void stream_state_cb(pa_stream *s, void *mainloop);
void stream_success_cb(pa_stream *stream, int success, void *userdata);
void stream_write_cb(pa_stream *stream, size_t requested_bytes, void *userdata);

static pa_threaded_mainloop *mainloop = 0;
static pa_mainloop_api *mainloop_api = 0;
static pa_context *context = 0;
static pa_stream *stream = 0;

void FreeAudio()
{
    if (mainloop)
    {
        pa_threaded_mainloop_stop(mainloop);
        if (context)
        {
            pa_context_disconnect(context);
            pa_context_unref(context);
            context = 0;
            if (stream)
            {
                pa_stream_unref(stream);
                stream = 0;
            }
        }
        pa_threaded_mainloop_free(mainloop);
        mainloop = 0;
    }
}

bool InitAudio() 
{
    LoadAllSamples();

    // Get a mainloop and its context
    mainloop = pa_threaded_mainloop_new();
    assert(mainloop);
    mainloop_api = pa_threaded_mainloop_get_api(mainloop);
    context = pa_context_new(mainloop_api, "pcm-playback");
    assert(context);

    // Set a callback so we can wait for the context to be ready
    pa_context_set_state_callback(context, &context_state_cb, mainloop);

    // Lock the mainloop so that it does not run and crash before the context is ready
    pa_threaded_mainloop_lock(mainloop);

    // Start the mainloop
    assert(pa_threaded_mainloop_start(mainloop) == 0);
    assert(pa_context_connect(context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) == 0);

    // Wait for the context to be ready
    for(;;) 
    {
        pa_context_state_t context_state = pa_context_get_state(context);
        assert(PA_CONTEXT_IS_GOOD(context_state));
        if (context_state == PA_CONTEXT_READY) 
            break;
        pa_threaded_mainloop_wait(mainloop);
    }

    // Create a playback stream
    pa_sample_spec sample_specifications;
    sample_specifications.format = FORMAT;
    sample_specifications.rate = RATE;
    sample_specifications.channels = CHANNELS;

    pa_channel_map map;
    pa_channel_map_init_stereo(&map);

    stream = pa_stream_new(context, "Playback", &sample_specifications, &map);
    pa_stream_set_state_callback(stream, stream_state_cb, mainloop);
    pa_stream_set_write_callback(stream, stream_write_cb, mainloop);

    // recommended settings, i.e. server uses sensible values
    pa_buffer_attr buffer_attr; 

    int stress = 1; // max no glitch = 7
    buffer_attr.maxlength = 4096 >> stress;
    buffer_attr.tlength = 2048 >> stress;
    buffer_attr.prebuf = 1024 >> stress;
    buffer_attr.minreq = 1024 >> stress;
    buffer_attr.fragsize = (uint32_t)-1; // rec only

    // Settings copied as per the chromium browser source
    pa_stream_flags_t stream_flags;
    stream_flags = (pa_stream_flags_t)(PA_STREAM_START_CORKED /* | PA_STREAM_INTERPOLATE_TIMING | 
        PA_STREAM_NOT_MONOTONIC | PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_ADJUST_LATENCY*/);

    // Connect stream to the default audio output sink
    assert(pa_stream_connect_playback(stream, NULL, &buffer_attr, stream_flags, NULL, NULL) == 0);

    // Wait for the stream to be ready
    for(;;) 
    {
        pa_stream_state_t stream_state = pa_stream_get_state(stream);
        assert(PA_STREAM_IS_GOOD(stream_state));
        if (stream_state == PA_STREAM_READY) 
            break;
        pa_threaded_mainloop_wait(mainloop);
    }

    pa_threaded_mainloop_unlock(mainloop);

    // Uncork the stream so it will start playing
    pa_stream_cork(stream, 0, stream_success_cb, mainloop);

    return true;
}

void context_state_cb(pa_context* context, void* mainloop) 
{
    pa_threaded_mainloop_signal((pa_threaded_mainloop*)mainloop, 0);
}

void stream_state_cb(pa_stream *s, void *mainloop) 
{
    pa_threaded_mainloop_signal((pa_threaded_mainloop*)mainloop, 0);
}

void stream_write_cb(pa_stream *stream, size_t requested_bytes, void *userdata) 
{
    int bytes_remaining = requested_bytes;
    while (bytes_remaining > 0) 
    {
        uint16_t *buffer = NULL;
        size_t bytes_to_fill = bytes_remaining;

        CallQueue* qc = OnAudioCall();
        while (qc)
        {
            DriverAudioCmd(0,qc->data,qc->size);
            // free it
            CallQueue* n = qc->next;
            free(qc);
            qc = n;
        }

        pa_stream_begin_write(stream, (void**) &buffer, &bytes_to_fill);

        int frames = bytes_to_fill/4;
        DriverAudioCB(0, (int16_t*)buffer, frames);

        pa_stream_write(stream, buffer, bytes_to_fill, NULL, 0LL, PA_SEEK_RELATIVE);

        bytes_remaining -= bytes_to_fill;
    }
}

void stream_success_cb(pa_stream *stream, int success, void *userdata) 
{
    return;
}

#define HAS_AUDIO

#endif
#endif

#ifdef USE_SDL
#ifndef HAS_AUDIO

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

void FreeAudio()
{
    SDL_CloseAudio();
}

void SDLAudioCB(void* userdata, Uint8* stream, int len) 
{
    CallQueue* qc = OnAudioCall();
    while (qc)
    {
        DriverAudioCmd(0,qc->data,qc->size);
        // free it
        CallQueue* n = qc->next;
        free(qc);
        qc = n;
    }

    int frames = len/4;
    int16_t* buffer = (int16_t*)stream;
    DriverAudioCB(0, (int16_t*)buffer, frames);
}

bool InitAudio() 
{
    LoadAllSamples();

    SDL_AudioSpec wanted;
    wanted.freq = 44100;
    wanted.format = AUDIO_S16;
    wanted.channels = 2;
    wanted.samples = 1024;
    wanted.callback = SDLAudioCB;
    wanted.userdata = NULL;

    if (SDL_OpenAudio(&wanted,0) < 0)
        return false;

    SDL_PauseAudio(0);
    return true;
}

#define HAS_AUDIO

#endif
#endif

#ifdef EMSCRIPTEN
#include <emscripten.h>

#ifdef WORKLET

static int16_t proc_buffer[2*128];
static uint8_t call_buffer[4096];

extern "C"
{
    uint8_t* Init(int num)
    {
        // num is snumber of samples we will be feeded with to decode
        return call_buffer;
    }

    int16_t* Proc()
    {
        DriverAudioCB(0, proc_buffer, 128);
        return proc_buffer;
    }

    void Call(uint8_t* data, int size)
    {
        DriverAudioCmd(0,data,size);
    }
}

#else

static int audio_mode = 0;

extern "C"
{
    const int16_t* Audio(int frames)
    {
        static int16_t* buffer = 0;
        static int buflen = 0;

        if (!buffer)
        {
            int alloc = 2*frames;
            buffer = (int16_t*)malloc(4 * alloc);
            buflen = alloc;
        }
        else
        if(frames > buflen)
        {
            free(buffer);
            int alloc = 2*frames;
            buffer = (int16_t*)malloc(4 * alloc);
            buflen = alloc;
        }

        DriverAudioCB(0, buffer, frames);

        return buffer;
    }
}

void CallAudio(const uint8_t* data, int size)
{
    if (!audio_mode)
        return;

    if (audio_mode>0)
    {
        // SCRIPTNODE
        // just exec it right now and here
        DriverAudioCmd(0, data, size);
    }
    else
    {
        // WORKLET
        // copy data to new Uint8Array
        // send it to worklet's audio_port    
        EM_ASM(
        {
            if (audio_port)
            {
                let view = new Uint8Array(Module.HEAPU8.buffer, Module.HEAPU8.byteOffset + $0, $1);
                audio_port.postMessage(new Uint8Array(view));
            }
            else
            {
                // very first audio call is essencial
                // cache it
                if (!audio_call_cache)
                {
                    let view = new Uint8Array(Module.HEAPU8.buffer, Module.HEAPU8.byteOffset + $0, $1);
                    audio_call_cache = new Uint8Array(view);
                }
            }
        },data,size);
    }
}

void FreeAudio()
{
    EM_ASM(
    {
        audio_ctx.close();

        audio_cb = null;
        audio_ctx = null;
        audio_node = null;
    });
}

bool InitAudio()
{
    int ret = EM_ASM_INT(
    {
        //console.log("Initializing Audio ", performance.now());
        var audioContext = window.AudioContext || window.webkitAudioContext;

        audio_cb = Module.cwrap("Audio", null, ["number"]);

        if (!audioContext || !audio_cb)
            return 0;
        
        audio_ctx = new audioContext({sampleRate: 44100, latencyHint: "interactive"});
        if (!audio_ctx)
            return 0;

        let samples = [];
        const enc = new TextEncoder();
        const c = FS.root.contents["samples"].contents;

        let Sample = Module.cwrap("Sample", null, ["string"]);
        let i = 0;
        let max_size = 0;
        for (const s in c)
        {
            if (max_size < c[s].contents.length)
                max_size = c[s].contents.length;
            samples[i++] = c[s].contents;
            Sample(s);
        }

        if (audio_ctx.audioWorklet)
        {
            audio_ctx.audioWorklet.addModule('audio.js').then(
            function(e)
            {
                let cfg =
                {
                    numberOfInputs:0,
                    numberOfOutputs:1,
                    outputChannelCount:[2],
                    processorOptions : samples
                };

                audio_node = new AudioWorkletNode(audio_ctx, 'asciicker-audio',cfg);
                audio_port = audio_node.port;
                audio_port.onmessage = function(e)
                {
                    // process msg from worklet to app
                    // ...
                    console.log(e.data, performance.now());
                };

                audio_node.connect(audio_ctx.destination);

                audio_ctx.resume();

                if (audio_call_cache)
                {
                    audio_port.postMessage(audio_call_cache);
                    audio_call_cache = null;
                }                
            });

            return ~(audio_ctx.sampleRate | 0);
        }
        else
        {
            var bufsize = 1024;
            audio_node = audio_ctx.createScriptProcessor(bufsize, 0, 2);

            audio_node.onaudioprocess = function(ev)
            {
                var samples = ev.outputBuffer.length;
                
                var audio_ptr = audio_cb(samples);

                var idx = audio_ptr >> 1;
                
                var left = ev.outputBuffer.getChannelData(0);
                var right = ev.outputBuffer.getChannelData(1);	

                const norm = 1.0/32767;		
                    
                for (var i=0; i<samples; i++)
                {
                    left[i] = Module.HEAP16[idx + 2*i] * norm;
                    right[i] = Module.HEAP16[idx + 2*i + 1] * norm;
                }
            };

            audio_node.connect(audio_ctx.destination);

            let data = 0;
            if (max_size)
                data = Module._malloc(max_size);

	    let XOgg = Module.cwrap('XOgg', null, ['number','number','number']);
	    console.log("WAHT ?????" + XOgg);
        let s_idx = 0;
            for (const s in c)
            {
                console.log(s);
                if (c[s].contents.length)
                    Module.HEAPU8.set(c[s].contents, data);
                XOgg(s_idx, data, c[s].contents.length);
                s_idx++;
            }

            if (max_size)
                Module._free(data);

            audio_ctx.resume();
            return audio_ctx.sampleRate | 0;
        }
    });

    audio_mode = ret;
    LoadAllSamples();
    return ret!=0;
}

#define HAS_AUDIO
#endif
#endif

#ifndef HAS_AUDIO
/*
void CallAudio(const uint8_t* data, int size)
{
}
*/

void FreeAudio()
{
}

bool InitAudio()
{
	return false;
}

#endif
