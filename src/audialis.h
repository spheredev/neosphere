#ifndef MINISPHERE__AUDIAL_H__INCLUDED
#define MINISPHERE__AUDIAL_H__INCLUDED

typedef struct stream stream_t;

void      initialize_audialis (void);
void      shutdown_audialis   (void);
void      update_audialis     (void);
stream_t* create_stream       (int frequency, int bits);
stream_t* ref_stream          (stream_t* stream);
void      free_stream         (stream_t* stream);
void      feed_stream         (stream_t* stream, const void* data, size_t size);
void      pause_stream        (stream_t* stream);
void      play_stream         (stream_t* stream);
void      stop_stream         (stream_t* stream);

void init_audialis_api (void);

#endif // MINISPHERE__AUDIAL_H__INCLUDED
