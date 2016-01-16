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
	const char* filename;
	const char* func_name;
	int32_t     line_no;
	message_t*  msg;
	int32_t     notify_id;
	int32_t     state;
	
	while (true) {
		msg = msg_receive(sess->remote);
		switch (msg_get_class(msg)) {
		case MSG_CLASS_NFY:
			msg_get_int(msg, 0, &notify_id);
			switch (notify_id) {
			case 0x01:
				msg_get_int(msg, 1, &state);
				msg_get_string(msg, 2, &filename);
				msg_get_string(msg, 3, &func_name);
				msg_get_int(msg, 4, &line_no);
				if (sess->is_paused = state != 0) {
					printf("STOPPED in function '%s'\n", func_name);
					printf("  script: %s\n", filename);
					printf("  line number: %d\n\n", line_no);
				}
				break;
			}
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
