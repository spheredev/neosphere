/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "ssj.h"
#include "host.h"

#include "path.h"
#include "utility.h"

#include <civetweb.h>
#include <signal.h>

static void handle_control_c   (int sig_num);
static int  handle_web_request (struct mg_connection* conn, void* udata);

static path_t*       s_engine_path;
static bool          s_exiting = false;
static const path_t* s_game_path;

bool
host_run(const path_t* game_path)
{
	const char* MG_OPTIONS[] = {
		"listening_ports", "+8080",
		NULL
	};

	struct mg_context* server;

	s_engine_path = path_new_self();
	path_strip(s_engine_path);
	path_append(s_engine_path, "system/oozaru/");
	s_game_path = game_path;

	printf("initializing a web server to host Oozaru...\n");
	mg_init_library(MG_FEATURES_IPV6);
	server = mg_start(NULL, NULL, MG_OPTIONS);
	mg_set_request_handler(server, "/", handle_web_request, NULL);
	printf("   navigate to http://localhost:8080/ to test\n");
	printf("   press Ctrl+C when done testing to exit SSj\n");
	s_exiting = false;
	signal(SIGINT, handle_control_c);
	signal(SIGTERM, handle_control_c);
	launch_url("http://localhost:8080/");
	while (!s_exiting) {
		stall(0.25);
	}
	printf("shutting down Oozaru web server and SSj...\n");
	mg_stop(server);
	mg_exit_library();
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	return true;
}

static void
handle_control_c(int sig_num)
{
	printf("\b \b\b \b");
	s_exiting = true;
}

static int
handle_web_request(struct mg_connection* conn, void* udata)
{
	void*                         data;
	size_t                        data_size;
	const char*                   file_ext;
	const char*                   mime_type;
	path_t*                       local_path;
	const struct mg_request_info* request;
	int                           status = 200;

	request = mg_get_request_info(conn);
	if (strstr(request->local_uri, "/dist/") == request->local_uri)
		local_path = path_rebase(path_new(&request->local_uri[6]), s_game_path);
	else
		local_path = path_rebase(path_new(&request->local_uri[1]), s_engine_path);
	if (!path_is_file(local_path))
		path_append(local_path, "index.html");
	if ((data = fslurp(path_cstr(local_path), &data_size))) {
		file_ext = path_extension(local_path);
		mime_type = strcmp(file_ext, ".css") == 0 ? "text/css"
			: strcmp(file_ext, ".html") == 0 ? "text/html"
			: strcmp(file_ext, ".htm") == 0 ? "text/html"
			: strcmp(file_ext, ".ico") == 0 ? "image/vnd.microsoft.icon"
			: strcmp(file_ext, ".js") == 0 ? "text/javascript"
			: strcmp(file_ext, ".json") == 0 ? "application/json"
			: strcmp(file_ext, ".mp3") == 0 ? "audio/mpeg"
			: strcmp(file_ext, ".ogg") == 0 ? "audio/ogg"
			: strcmp(file_ext, ".png") == 0 ? "image/png"
			: strcmp(file_ext, ".svg") == 0 ? "image/svg+xml"
			: "application/octet-stream";
		mg_send_http_ok(conn, mime_type, data_size);
		mg_write(conn, data, data_size);
		free(data);
	} else {
		status = 404;
		mg_send_http_error(conn, status, "an eaty pig ate it.");
	}
	if (status != 200)
		printf("\33[33;1mwarning\33[m: HTTP %d - %s\n", status, request->local_uri);
	path_free(local_path);
	return status;
}
