#include "ssj.h"
#include "session.h"

#include "remote.h"

struct session
{
	char      command[65536];
	remote_t* remote;
	bool      is_paused;
};

static void do_command_line (session_t* sess);

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
		message_t* msg = receive_message(sess->remote);
		switch (get_message_class(msg)) {
		case MSG_CLASS_NFY:
			break;
		}
		if (sess->is_paused)
			do_command_line(sess);
	}
}

static void
do_command_line(session_t* sess)
{
	char ch;
	char *p;

	printf("SSJ> ");
	p = memset(sess->command, 0, sizeof sess->command);
	while ((ch = getchar()) != '\n')
		*p++ = ch;
}

static bool
do_message(session_t* sess)
{
}
