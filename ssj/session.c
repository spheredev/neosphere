#include "ssj.h"
#include "session.h"

#include "remote.h"

struct session
{
	remote_t* remote;
	bool      is_paused;
};

static void do_command_line (session_t* sess);
static void skip_to_eom     (session_t* sess);

session_t*
new_session(const char* hostname, int port)
{
	session_t* sess;

	sess = calloc(1, sizeof(session_t));
	if (!(sess->remote = connect_remote(hostname, port)))
		goto on_error;
	return sess;

on_error:
	free(sess);
	return NULL;
}

void
run_session(session_t* sess)
{
	while (true) {
		if (sess->is_paused)
			do_command_line(sess);
		skip_to_eom(sess);
	}
}

static void
do_command_line(session_t* sess)
{
	char line[65536];
	char ch;
	char *p;

	printf("SSJ> ");
	p = memset(line, 0, sizeof line);
	while ((ch = getchar()) != '\n')
		*p++ = ch;
}

static void
skip_to_eom(session_t* sess)
{
	bool      have_eom;
	dvalue_t* dvalue;
	
	do {
		dvalue = receive_dvalue(sess->remote);
		have_eom = dvalue_get_tag(dvalue) == DVALUE_TAG_EOM;
		dvalue_free(dvalue);
	} while (!have_eom);
}
