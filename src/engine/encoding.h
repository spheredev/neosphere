#ifndef MINISPHERE__ENCODING_H__INCLUDED
#define MINISPHERE__ENCODING_H__INCLUDED

typedef struct decoder decoder_t;
typedef struct encoder encoder_t;

decoder_t* decoder_new       (bool fatal, bool ignoreBOM);
decoder_t* decoder_ref       (decoder_t* decoder);
void       decoder_free      (decoder_t* decoder);
bool       decoder_fatal     (const decoder_t* decoder);
bool       decoder_ignoreBOM (const decoder_t* decoder);
lstring_t* decoder_decode    (decoder_t* decoder, const uint8_t* buffer, size_t size);

#endif // MINISPHERE__ENCODING_H__INCLUDED
