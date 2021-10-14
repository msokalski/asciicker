


#include <stdio.h>
#include <malloc.h>
#include <string.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "fast_rand.h"
#include "audio.h"

static int test_amp = 0;
static int test_frq = 100;
void TestAudioCmd(void* userdata, const uint8_t* data, int size)
{
    if (size == 4)
    {
        test_amp = 65536;
        test_frq = *(const int*)data;
    }
}

void TestAudioCB(void* userdata, int16_t buffer[], int samples)
{
    static int phase = 0;
    for (size_t i = 0; i < samples; i++) 
    {
        int wave = (int)(32767 * sinf(phase*2*M_PI / test_frq));
        phase++;
        if (phase>=test_frq)
            phase=0;

        buffer[2*i] = (wave * test_amp) >> (4+16);
        buffer[2*i+1] = (wave * test_amp) >> (4+16);

        if (test_amp)
            test_amp--;

        //buffer[2*i] = (fast_rand()*2 - 32767) >> 4;
        //buffer[2*i+1] = (fast_rand()*2 - 32767) >> 4;
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
            TestAudioCmd(0,qc->data,qc->size);
            // free it
            CallQueue* n = qc->next;
            free(qc);
            qc = n;
        }

        pa_stream_begin_write(stream, (void**) &buffer, &bytes_to_fill);

        int samples = bytes_to_fill/4;
        TestAudioCB(0, (int16_t*)buffer, bytes_to_fill/4);

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
        TestAudioCmd(0,qc->data,qc->size);
        // free it
        CallQueue* n = qc->next;
        free(qc);
        qc = n;
    }

    int samples = len/4;
    int16_t* buffer = (int16_t*)stream;
    TestAudioCB(0, (int16_t*)buffer, samples);
}

bool InitAudio() 
{
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
static uint8_t call_buffer[1024];

extern "C"
{
    uint8_t* Init()
    {
        return call_buffer;
    }

    int16_t* Proc()
    {
        TestAudioCB(0, proc_buffer, 128);
        return proc_buffer;
    }

    void Call(int size)
    {
        // all data are stored in call_buffer already
        TestAudioCmd(0,call_buffer,size);
    }
}

#else

static int audio_mode = 0;

extern "C"
{
    const int16_t* Audio(int samples)
    {
        static int16_t* buffer = 0;
        static int buflen = 0;

        if (!buffer)
        {
            int alloc = 2*samples;
            buffer = (int16_t*)malloc(4 * alloc);
            buflen = alloc;
        }
        else
        if(samples > buflen)
        {
            free(buffer);
            int alloc = 2*samples;
            buffer = (int16_t*)malloc(4 * alloc);
            buflen = alloc;
        }

        TestAudioCB(0, buffer, samples);

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
        TestAudioCmd(0, data, size);
    }
    else
    {
        // WORKLET
        // copy data to new Uint8Array
        // send it to worklet's audio_port    
        EM_ASM(
        {
            let view = new Uint8Array(Module.HEAPU8.buffer, Module.HEAPU8.byteOffset + $0, $1);
            audio_port.postMessage(new Uint8Array(view));
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
        var audioContext = window.AudioContext || window.webkitAudioContext;

        audio_cb = Module.cwrap("Audio", null, ["number"]);

        if (!audioContext || !audio_cb)
            return 0;
        
        audio_ctx = new audioContext({sampleRate: 44100, latencyHint: "interactive"});
        if (!audio_ctx)
            return 0;

        if (audio_ctx.audioWorklet) // DISABLED !!! 
        {
            audio_ctx.audioWorklet.addModule('audio.js').then(
            function(e)
            {
                let cfg =
                {
                    numberOfInputs:0,
                    numberOfOutputs:1,
                    outputChannelCount:[2],
                };

                audio_node = new AudioWorkletNode(audio_ctx, 'asciicker-audio',cfg);
                audio_port = audio_node.port;
                audio_port.onmessage = function(e)
                {
                    // process msg from worklet to app
                    // ...
                };

                audio_node.connect(audio_ctx.destination);

                audio_ctx.resume();
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

            audio_ctx.resume();
            return audio_ctx.sampleRate | 0;
        }
    });

    audio_mode = ret;
    return ret!=0;
}

#define HAS_AUDIO
#endif
#endif

#ifndef HAS_AUDIO

void CallAudio(const uint8_t* data, int size)
{
}

void FreeAudio()
{
}

bool InitAudio()
{
	return false;
}

#endif
