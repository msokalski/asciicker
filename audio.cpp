

#include <stdio.h>
#include <malloc.h>
#include "fast_rand.h"
#include "audio.h"

static AudioCB audio_cb = 0;
static void* audio_userdata = 0;

/////////////////////////////////////
#ifdef __linux__ 

#include <stdio.h>
#include <assert.h>
#include <pulse/pulseaudio.h>

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


int GetAudioLatency()
{
    pa_usec_t usec;
    int neg = 0;
    int error = pa_stream_get_latency(stream, &usec, &neg); 	
    if (error || neg || (usec>>32))
        return -1;
    return (int)usec;
}

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

bool InitAudio(AudioCB cb, void* userdata) 
{
    audio_cb = cb;
    audio_userdata = userdata;

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
    buffer_attr.maxlength = (uint32_t) -1;
    buffer_attr.tlength = (uint32_t) -1;
    buffer_attr.prebuf = (uint32_t) -1;
    buffer_attr.minreq = (uint32_t) -1;

    // Settings copied as per the chromium browser source
    pa_stream_flags_t stream_flags;
    stream_flags = (pa_stream_flags_t)(PA_STREAM_START_CORKED | PA_STREAM_INTERPOLATE_TIMING | 
        PA_STREAM_NOT_MONOTONIC | PA_STREAM_AUTO_TIMING_UPDATE |
        PA_STREAM_ADJUST_LATENCY);

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
        size_t bytes_to_fill = (size_t)-1;

        pa_stream_begin_write(stream, (void**) &buffer, &bytes_to_fill);

        int samples = bytes_to_fill/4;

        #if READY_TO_CALL
        audio_cb(audio_userdata, (int16_t*)buffer, bytes_to_fill/4);
        #else
        for (size_t i = 0; i < samples; i++) 
        {
            buffer[2*i] = (fast_rand()*2 - 32767) >> 4;
            buffer[2*i+1] = (fast_rand()*2 - 32767) >> 4;
        }
        #endif

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

#ifdef EMSCRIPTEN
#include <emscripten.h>

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

        #if READY_TO_CALL
        audio_cb(audio_userdata, buffer, samples);
        #else
        for (size_t i = 0; i < samples; i++) 
        {
            buffer[2*i] = (fast_rand()*2 - 32767) >> 4;
            buffer[2*i+1] = (fast_rand()*2 - 32767) >> 4;
        }
        #endif

        return buffer;
    }
}

int GetAudioLatency()
{
    return 0;
}

void FreeAudio()
{
    EM_ASM_INT(
    {
        audio_ctx.close();

        audio_cb = null;
        audio_ctx = null;
        audio_node = null;
    }    

    audio_cb = 0;
    audio_userdata = 0;
}

bool InitAudio(AudioCB cb, void* userdata) 
{
    audio_cb = cb;
    audio_userdata = userdata;

    EM_ASM_INT(
    {
        var audioContext = window.AudioContext || window.webkitAudioContext;

        audio_cb = Module.cwrap("Audio", null, ["number"]);

        if (!audioContext || !audio_cb)
            return 0;
        
        audio_ctx = new audioContext({sampleRate: 44100, latencyHint: "interactive"});
        if (!audio_ctx)
            return 0;
        
        var bufsize = 512;
        audio_node = audio_ctx.createScriptProcessor(bufsize, 0, 2);

        audio_node.onaudioprocess = function(ev)
        {
            var samples = ev.outputBuffer.length;
            
            var audio_ptr = audio_cb(samples);

            var idx = audio_ptr >> 1;
            
            var left = ev.outputBuffer.getChannelData(0);
            var right = ev.outputBuffer.getChannelData(1);			
                
            for (var i=0; i<samples; i++)
            {
                left[i] = Module.HEAP16[idx + 2*i] >> 15;
                right[i] = Module.HEAP16[idx + 2*i + 1] >> 15;
            }
        };

        audio_node.connect(audio_ctx.destination);
        audio_ctx.resume();

        return audio_ctx.sampleRate | 0;
    }, userdata);

    return false;
}

#define HAS_AUDIO

#endif

#ifdef USE_SDL
#ifndef HAS_AUDIO
#include <SDL.h>

int GetAudioLatency()
{
    return 0;
}

void FreeAudio()
{
    SDL_CloseAudio();
}

void SDLAudioCB(void* userdata, Uint8* stream, int len) 
{
    int samples = len/4;

    int16_t* buffer = (int16_t*)stream;

    // SDL_MixAudio ???

    #if READY_TO_CALL
    audio_cb(audio_userdata, (int16_t*)buffer, bytes_to_fill/4);
    #else
    for (size_t i = 0; i < samples; i++) 
    {
        buffer[2*i] = (fast_rand()*2 - 32767) >> 4;
        buffer[2*i+1] = (fast_rand()*2 - 32767) >> 4;
    }
    #endif
}

bool InitAudio(AudioCB cb, void* userdata) 
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

#ifndef HAS_AUDIO

int GetAudioLatency()
{
	return 0;
}

void FreeAudio()
{
}

bool InitAudio(AudioCB cb, void* userdata)
{
	return false;
}

#endif