#include "minisphere.h"
#include "api.h"
#include "bytearray.h"

#include "audialis.h"

static duk_ret_t js_new_SoundStream      (duk_context* ctx);
static duk_ret_t js_SoundStream_finalize (duk_context* ctx);
static duk_ret_t js_SoundStream_feed     (duk_context* ctx);

static void update_stream (stream_t* stream);

struct stream
{
	unsigned int          refcount;
	ALLEGRO_AUDIO_STREAM* al_stream;
	size_t                fragment_size;
	unsigned char*        buffer;
	size_t                feed_size;
};

static vector_t* s_streams;

void
initialize_audialis(void)
{
	console_log(1, "Initializing Audialis\n");
	s_streams = new_vector(sizeof(stream_t*));
}

void
shutdown_audialis(void)
{
	console_log(1, "Shutting down Audialis\n");
	free_vector(s_streams);
}

void
update_audialis(void)
{
	stream_t* *p_stream;
	
	iter_t iter;

	iter = iterate_vector(s_streams);
	while (p_stream = next_vector_item(&iter))
		update_stream(*p_stream);
}

stream_t*
create_stream(int frequency, int bits)
{
	ALLEGRO_AUDIO_DEPTH depth_flag;
	stream_t*           stream;

	depth_flag = bits == 8 ? ALLEGRO_AUDIO_DEPTH_INT8
		: bits == 24 ? ALLEGRO_AUDIO_DEPTH_INT24
		: ALLEGRO_AUDIO_DEPTH_INT16;
	stream = calloc(1, sizeof(stream_t));
	stream->al_stream = al_create_audio_stream(4, 1024, frequency, depth_flag, ALLEGRO_CHANNEL_CONF_2);
	al_attach_audio_stream_to_mixer(stream->al_stream, al_get_default_mixer());
	push_back_vector(s_streams, &stream);
	return ref_stream(stream);
}

stream_t*
ref_stream(stream_t* stream)
{
	++stream->refcount;
	return stream;
}

void
free_stream(stream_t* stream)
{
	stream_t** p_stream;
	
	iter_t iter;
	
	if (stream == NULL || --stream->refcount > 0)
		return;
	al_destroy_audio_stream(stream->al_stream);
	free(stream->buffer);
	free(stream);
	iter = iterate_vector(s_streams);
	while (p_stream = next_vector_item(&iter)) {
		if (*p_stream == stream) {
			remove_vector_item(s_streams, iter.index);
			break;
		}
	}
}

void
feed_stream(stream_t* stream, const void* data, size_t size)
{
	stream->buffer = realloc(stream->buffer, stream->feed_size + size);
	memcpy(stream->buffer + stream->feed_size, data, size);
	stream->feed_size += size;
}

static void
update_stream(stream_t* stream)
{
	void* buffer;
	
	if (stream->feed_size <= 2048) return;
	if (!(buffer = al_get_audio_stream_fragment(stream->al_stream)))
		return;
	memcpy(buffer, stream->buffer, 2048);
	stream->feed_size -= 2048;
	memmove(stream->buffer, stream->buffer + 2048, stream->feed_size);
	al_set_audio_stream_fragment(stream->al_stream, buffer);
}

void
init_audialis_api(void)
{
	register_api_ctor(g_duk, "SoundStream", js_new_SoundStream, js_SoundStream_finalize);
	register_api_function(g_duk, "SoundStream", "feed", js_SoundStream_feed);
}

static duk_ret_t
js_new_SoundStream(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int frequency = n_args >= 1 ? duk_require_int(ctx, 0) : 44100;
	int bits = n_args >= 2 ? duk_require_int(ctx, 1) : 16;

	stream_t* stream;

	if (bits != 8 && bits != 16 && bits != 24)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SoundStream(): Invalid bit depth (%i)", bits);
	if (!(stream = create_stream(frequency, bits)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SoundStream(): Stream creation failed");
	duk_push_sphere_obj(ctx, "SoundStream", stream);
	return 1;
}

static duk_ret_t
js_SoundStream_finalize(duk_context* ctx)
{
	stream_t* stream;
	
	stream = duk_require_sphere_obj(ctx, 0, "SoundStream");
	free_stream(stream);
	return 0;
}

static duk_ret_t
js_SoundStream_feed(duk_context* ctx)
{
	bytearray_t* array = duk_require_sphere_bytearray(ctx, 0);

	const void* buffer;
	size_t      size;
	stream_t*   stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");
	duk_pop(ctx);
	buffer = get_bytearray_buffer(array);
	size = get_bytearray_size(array);
	feed_stream(stream, buffer, size);
	return 0;
}
