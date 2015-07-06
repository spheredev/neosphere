#ifndef MINISPHERE__AUDIAL_H__INCLUDED
#define MINISPHERE__AUDIAL_H__INCLUDED

typedef struct stream stream_t;

void      initialize_audial (void);
void      shutdown_audial   (void);
void      update_audial     (void);
stream_t* create_stream     (int frequency, int bits);
stream_t* ref_stream        (stream_t* stream);
void      free_stream       (stream_t* stream);
void      feed_stream       (stream_t* stream, const void* data, size_t size);

void init_audial_api (void);

#endif // MINISPHERE__AUDIAL_H__INCLUDED
