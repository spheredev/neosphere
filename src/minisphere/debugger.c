#include "minisphere.h"
#include "sockets.h"

#include "debugger.h"

struct source
{
	char*      name;
	lstring_t* text;
};

enum appnotify
{
	APPNFY_NOP,
	APPNFY_DEBUG_PRINT,
};

enum apprequest
{
	APPREQ_NOP,
	APPREQ_GAME_INFO,
	APPREQ_SOURCE,
};

static const int TCP_DEBUG_PORT = 1208;

static bool       attach_debugger      (void);
static void       detach_debugger      (bool is_shutdown);
static void       duk_cb_debug_detach  (void* udata);
static duk_idx_t  duk_cb_debug_request (duk_context* ctx, void* udata, duk_idx_t nvalues);
static duk_size_t duk_cb_debug_peek    (void* udata);
static duk_size_t duk_cb_debug_read    (void* udata, char* buffer, duk_size_t bufsize);
static duk_size_t duk_cb_debug_write   (void* udata, const char* data, duk_size_t size);

static bool      s_is_attached = false;
static socket_t* s_client;
static bool      s_have_source_map;
static socket_t* s_server;
static vector_t* s_sources;
static bool      s_want_attach;

void
initialize_debugger(bool want_attach, bool allow_remote)
{
	void*         data;
	size_t        data_size;
	const path_t* game_path;
	const char*   hostname;

	s_sources = vector_new(sizeof(struct source));
	
	// load the source map, if one is available
	s_have_source_map = false;
	duk_push_global_stash(g_duk);
	duk_del_prop_string(g_duk, -1, "debugMap");
	game_path = get_game_path(g_fs);
	if (data = sfs_fslurp(g_fs, "sourcemap.json", NULL, &data_size)) {
		duk_push_lstring(g_duk, data, data_size);
		duk_json_decode(g_duk, -1);
		duk_put_prop_string(g_duk, -2, "debugMap");
		free(data);
		s_have_source_map = true;
	}
	else if (!path_is_file(game_path)) {
		duk_push_object(g_duk);
		duk_push_string(g_duk, path_cstr(game_path));
		duk_put_prop_string(g_duk, -2, "origin");
		duk_put_prop_string(g_duk, -2, "debugMap");
	}
	duk_pop(g_duk);
	
	// listen for SSJ connection on TCP port 1208. the listening socket will remain active
	// for the duration of the session, allowing a debugger to be attached at any time.
	console_log(1, "listening for SSJ on TCP %i", TCP_DEBUG_PORT);
	hostname = allow_remote ? NULL : "127.0.0.1";
	s_server = listen_on_port(hostname, TCP_DEBUG_PORT, 1024, 1);

	// if the engine was started in debug mode, wait for a debugger to connect before
	// beginning execution.
	s_want_attach = want_attach;
	if (s_want_attach && !attach_debugger())
		exit_game(true);
}

void
shutdown_debugger()
{
	iter_t iter;
	struct source* p_source;
	
	detach_debugger(true);
	free_socket(s_server);
	
	iter = vector_enum(s_sources);
	while (p_source = vector_next(&iter)) {
		lstr_free(p_source->text);
		free(p_source->name);
	}
	vector_free(s_sources);
}

void
update_debugger(void)
{
	socket_t* socket;

	if (socket = accept_next_socket(s_server)) {
		if (s_client != NULL) {
			console_log(2, "rejected connection from %s, debugger already attached",
				get_socket_host(socket));
			free_socket(socket);
		}
		else {
			console_log(1, "connected to debugger at %s", get_socket_host(socket));
			s_client = socket;
			duk_debugger_detach(g_duk);
			duk_debugger_attach_custom(g_duk,
				duk_cb_debug_read,
				duk_cb_debug_write,
				duk_cb_debug_peek,
				NULL,
				NULL,
				duk_cb_debug_request,
				duk_cb_debug_detach,
				NULL);
			s_is_attached = true;
		}
	}
}

bool
is_debugger_attached(void)
{
	return s_is_attached;
}

const char*
get_compiled_name(const char* source_name)
{
	// perform a reverse lookup on the source map to find the compiled name
	// of an asset based on its name in the source tree.  this is needed to
	// support SSJ source code download, since SSJ only knows the source names.

	static char retval[SPHERE_PATH_MAX];

	const char* this_source;

	strncpy(retval, source_name, SPHERE_PATH_MAX - 1);
	retval[SPHERE_PATH_MAX - 1] = '\0';
	if (!s_have_source_map)
		return retval;
	duk_push_global_stash(g_duk);
	duk_get_prop_string(g_duk, -1, "debugMap");
	if (!duk_get_prop_string(g_duk, -1, "fileMap"))
		duk_pop_3(g_duk);
	else {
		duk_enum(g_duk, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
		while (duk_next(g_duk, -1, true)) {
			this_source = duk_get_string(g_duk, -1);
			if (strcmp(this_source, source_name) == 0)
				strncpy(retval, duk_get_string(g_duk, -2), SPHERE_PATH_MAX - 1);
			duk_pop_2(g_duk);
		}
		duk_pop_n(g_duk, 4);
	}
	return retval;
}

const char*
get_source_name(const char* compiled_name)
{
	// note: pathname must be canonicalized using make_sfs_path() otherwise
	//       the source map lookup will fail.

	static char retval[SPHERE_PATH_MAX];

	strncpy(retval, compiled_name, SPHERE_PATH_MAX - 1);
	retval[SPHERE_PATH_MAX - 1] = '\0';
	if (!s_have_source_map)
		return retval;
	duk_push_global_stash(g_duk);
	duk_get_prop_string(g_duk, -1, "debugMap");
	if (!duk_get_prop_string(g_duk, -1, "fileMap"))
		duk_pop_3(g_duk);
	else {
		duk_get_prop_string(g_duk, -1, compiled_name);
		if (duk_is_string(g_duk, -1))
			strncpy(retval, duk_get_string(g_duk, -1), SPHERE_PATH_MAX - 1);
		duk_pop_n(g_duk, 4);
	}
	return retval;
}

void
cache_source(const char* name, const lstring_t* text)
{
	struct source cache_entry;
	
	iter_t iter;
	struct source* p_source;
	
	if (s_sources == NULL)
		return;
	
	iter = vector_enum(s_sources);
	while (p_source = vector_next(&iter)) {
		if (strcmp(name, p_source->name) == 0) {
			lstr_free(p_source->text);
			p_source->text = lstr_dup(text);
			return;
		}
	}
	
	cache_entry.name = strdup(name);
	cache_entry.text = lstr_dup(text);
	vector_push(s_sources, &cache_entry);
}

void
debug_print(const char* text)
{
	duk_push_int(g_duk, APPNFY_DEBUG_PRINT);
	duk_push_string(g_duk, text);
	duk_debugger_notify(g_duk, 2);
}

static bool
attach_debugger(void)
{
	double timeout;

	printf("waiting for SSJ... ");
	fflush(stdout);
	timeout = al_get_time() + 30.0;
	while (s_client == NULL && al_get_time() < timeout) {
		update_debugger();
		delay(0.05);
	}
	if (s_client == NULL)  // did we time out?
		printf("timed out!\n");
	else
		printf("OK.\n");
	return s_client != NULL;
}

static void
detach_debugger(bool is_shutdown)
{
	if (!s_is_attached) return;
	
	// detach the debugger
	console_log(1, "detaching debugger");
	s_is_attached = false;
	duk_debugger_detach(g_duk);
	if (s_client != NULL) {
		shutdown_socket(s_client);
		while (is_socket_live(s_client))
			delay(0.05);
	}
	free_socket(s_client);
	s_client = NULL;
	if (s_want_attach && !is_shutdown)
		exit_game(true);  // clean detach, exit
}

static void
duk_cb_debug_detach(void* udata)
{
	// note: if s_client is null, a TCP reset was detected by one of the I/O callbacks.
	// if this is the case, wait a bit for the client to reconnect.
	if (s_client != NULL || !attach_debugger())
		detach_debugger(false);
}

static duk_idx_t
duk_cb_debug_request(duk_context* ctx, void* udata, duk_idx_t nvalues)
{
	void*       file_data;
	const char* name;
	int         request_id;
	size_t      size;
	int         x_size;
	int         y_size;

	iter_t iter;
	struct source* p_source;
	
	request_id = duk_get_int(ctx, -nvalues + 0);
	switch (request_id) {
	case APPREQ_GAME_INFO:
		get_sgm_resolution(g_fs, &x_size, &y_size);
		duk_push_string(ctx, get_sgm_name(g_fs));
		duk_push_string(ctx, get_sgm_author(g_fs));
		duk_push_string(ctx, get_sgm_summary(g_fs));
		duk_push_int(ctx, x_size);
		duk_push_int(ctx, y_size);
		return 5;
	case APPREQ_SOURCE:
		name = duk_get_string(ctx, -nvalues + 1);
		name = get_compiled_name(name);

		// check if the data is in the source cache
		iter = vector_enum(s_sources);
		while (p_source = vector_next(&iter)) {
			if (strcmp(name, p_source->name) == 0) {
				duk_push_lstring_t(ctx, p_source->text);
				return 1;
			}
		}
		
		// no cache entry, try loading the file via SphereFS
		if ((file_data = sfs_fslurp(g_fs, name, NULL, &size))) {
			duk_push_lstring(ctx, file_data, size);
			free(file_data);
			return 1;
		}
		
		duk_push_sprintf(ctx, "no source available for '%s'", name);
		return -1;
	default:
		return 0;
	}
}

static duk_size_t
duk_cb_debug_peek(void* udata)
{
	return peek_socket(s_client);
}

static duk_size_t
duk_cb_debug_read(void* udata, char* buffer, duk_size_t bufsize)
{
	size_t n_bytes;

	if (s_client == NULL) return 0;

	// if we return zero, Duktape will drop the session. thus we're forced
	// to block until we can read >= 1 byte.
	while ((n_bytes = peek_socket(s_client)) == 0) {
		if (!is_socket_live(s_client)) {  // did a pig eat it?
			console_log(1, "TCP connection reset while debugging");
			free_socket(s_client); s_client = NULL;
			return 0;  // stupid pig
		}

		// so the system doesn't think we locked up...
		delay(0.05);
	}

	// let's not overflow the buffer, alright?
	if (n_bytes > bufsize) n_bytes = bufsize;
	read_socket(s_client, buffer, n_bytes);
	return n_bytes;
}

static duk_size_t
duk_cb_debug_write(void* udata, const char* data, duk_size_t size)
{
	if (s_client == NULL) return 0;

	// make sure we're still connected
	if (!is_socket_live(s_client)) {
		console_log(1, "TCP connection reset while debugging");
		free_socket(s_client); s_client = NULL;
		return 0;
	}

	// send out the data
	write_socket(s_client, data, size);
	return size;
}
