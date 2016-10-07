#ifndef MINISPHERE__ENCODING_H__INCLUDED
#define MINISPHERE__ENCODING_H__INCLUDED

typedef struct decoder decoder_t;
typedef struct encoder encoder_t;

decoder_t* decoder_new        (bool fatal, bool ignore_bom);
decoder_t* decoder_ref        (decoder_t* decoder);
void       decoder_free       (decoder_t* decoder);
bool       decoder_fatal      (const decoder_t* decoder);
bool       decoder_ignore_bom (const decoder_t* decoder);
lstring_t* decoder_finish     (decoder_t* decoder);
lstring_t* decoder_run        (decoder_t* decoder, const uint8_t* buffer, size_t size);
encoder_t* encoder_new        (void);
encoder_t* encoder_ref        (encoder_t* encoder);
void       encoder_free       (encoder_t* encoder);
uint8_t*   encoder_run        (encoder_t* encoder, const lstring_t* string, size_t *out_size);

#endif // MINISPHERE__ENCODING_H__INCLUDED
