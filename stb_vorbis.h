#ifndef STB_VORBIS_H
#define STB_VORBIS_H

typedef struct stb_vorbis stb_vorbis;

extern unsigned int stb_vorbis_stream_length_in_samples(stb_vorbis *f);

typedef struct
{
   char *alloc_buffer;
   int   alloc_buffer_length_in_bytes;
} stb_vorbis_alloc;

extern stb_vorbis * stb_vorbis_open_memory(const unsigned char *data, int len,
                                  int *error, const stb_vorbis_alloc *alloc_buffer);

extern void stb_vorbis_close(stb_vorbis *f);

extern int stb_vorbis_get_frame_float(stb_vorbis *f, int *channels, float ***output);

typedef struct
{
   unsigned int sample_rate;
   int channels;

   unsigned int setup_memory_required;
   unsigned int setup_temp_memory_required;
   unsigned int temp_memory_required;

   int max_frame_size;
} stb_vorbis_info;

extern stb_vorbis_info stb_vorbis_get_info(stb_vorbis *f);

extern const char* stb_vorbis_get_markers(stb_vorbis *f);
extern char* stb_vorbis_extract_markers(stb_vorbis *f);

#endif
