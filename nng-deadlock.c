/* nng-deadlock.c */

#include <sys/param.h> /* PATH_MAX */
#include <sys/types.h> /* pid_t */
#include <sys/wait.h>  /* wait */
#include <unistd.h>    /* fork */

#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/pair0/pair.h>
#include <nng/protocol/pair1/pair.h>

#include "mystuff.c"

#define MSGSIZE 128

struct {
	int  num_passes;
	char ctlname[PATH_MAX];
	int (*server_open)(nng_socket *);
	int (*client_open)(nng_socket *);
} conf;

void build_sockpath(char *sockpath, const char *basepath, int pass)
{
	if (snprintf(sockpath, PATH_MAX, "%s.%d", basepath, pass) < PATH_MAX)
		return;
	fprintf(stderr, "Error: %s is too long, must be < %u - len(%d)\n", basepath, PATH_MAX, pass);
	exit(1);
}

void run_server(void)
{
	char  sendbuf[MSGSIZE];
	int   pass;
	nng_socket ctlsock;
	nng_call("server:ctl:open",   nng_rep0_open(&ctlsock));
	nng_call("server:ctl:listen", nng_listen(ctlsock, conf.ctlname, NULL, 0));

	for (pass = 0; pass < conf.num_passes; pass++) {

		nng_socket loopsock = {0};

		size_t ctlsz;
		char *ctlbuf = NULL;

		char  *recvbuf = NULL;
		size_t recvsz;

		char sockpath[PATH_MAX];
		build_sockpath(sockpath, conf.ctlname, pass);

		/* handshake with client... */
		printf("server entered pass %d on %s\n", pass, sockpath);
		nng_call("server:ctl:recv",   nng_recv(ctlsock, &ctlbuf, &ctlsz, NNG_FLAG_ALLOC));
		nng_free(ctlbuf, ctlsz);
		nng_call("server:xxx:open",   (conf.server_open)(&loopsock));
		nng_call("server:xxx:listen", nng_listen(loopsock, sockpath, NULL, 0));
		nng_call("server:ctl:send",   nng_send(ctlsock, "OK", sizeof("OK"), 0));
		printf("server handshaked at pass %d\n", pass);

		nng_call("server:xxx:recv", nng_recv(loopsock, &recvbuf, &recvsz, NNG_FLAG_ALLOC));
		printf("server pass %d: recv done.\n", pass);

		snprintf(sendbuf, MSGSIZE, "%d", pass);
		nng_call("server:xxx:send", nng_send(loopsock, sendbuf, MSGSIZE, 0));
		printf("server pass %d: send done.\n", pass);

		nng_free(recvbuf, recvsz);
		printf("server pass %d: free done.\n", pass);

		nng_call("server:xxx:close",  nng_close(loopsock));
		printf("server finished pass %d\n", pass);
	}

	nng_call("server:ctl:close",  nng_close(ctlsock));
}

int try_dial(nng_socket sock, const char *url, unsigned num_attempts)
{
	int n, ret;
	for (n = 0; n < num_attempts; n++) {
		ret = nng_dial(sock, url, NULL, 0);
		if (ret) {
			sleep(1);
			continue;
		}
		/* todo: print attempt */
		printf("client dialed after attempt %d\n", n+1);
		return 0;
	}
	return ret;
}

void run_client(void)
{
	char  sendbuf[MSGSIZE];
	int   pass;
	int   ctlpass;
	nng_socket ctlsock;
	nng_call("client:ctl:open", nng_req0_open(&ctlsock));
	nng_call("client:ctl:dial", try_dial(ctlsock, conf.ctlname, 3));

	for (pass = 0; pass < conf.num_passes; pass++) {

		nng_socket loopsock = {0};

		size_t ctlsz;
		char *ctlbuf = NULL;

		char sockpath[PATH_MAX];
		build_sockpath(sockpath, conf.ctlname, pass);

		/* handshake with server... */
		printf("client entered pass %d on %s\n", pass, sockpath);
		nng_call("client:ctl:send",   nng_send(ctlsock, "OK", sizeof("OK"), 0));
		nng_call("client:ctl:recv",   nng_recv(ctlsock, &ctlbuf, &ctlsz, NNG_FLAG_ALLOC));
		nng_free(ctlbuf, ctlsz);
		nng_call("client:xxx:open",   (conf.client_open)(&loopsock));
		nng_call("client:xxx:dial",   nng_dial(loopsock, sockpath, NULL, 0));
		printf("client handshaked at pass %d\n", pass);

		char  *recvbuf = NULL;
		size_t recvsz;
		
		snprintf(sendbuf, MSGSIZE, "%d", pass);
		nng_call("client:xxx:send", nng_send(loopsock, sendbuf, MSGSIZE, 0));
		printf("client pass %d: send done.\n", pass);

		printf("client pass %d: recv wait...\n", pass);
		nng_call("client:xxx:recv", nng_recv(loopsock, &recvbuf, &recvsz, NNG_FLAG_ALLOC));
		printf("client pass %d: recv done.\n", pass);

		nng_free(recvbuf, recvsz);
		printf("client pass %d: free done.\n", pass);

		nng_call("client:xxx:close",  nng_close(loopsock));
		printf("client finished pass %d\n", pass);
	}

	nng_call("client:ctl:close",  nng_close(ctlsock));
}

int main(int argc, const char *argv[])
{
	pid_t pid;

	if (argc > 3) {
		fprintf(stderr, "Usage: %s [num-passes [mode]]\n", argv[0]);
		return 1;
	}

	conf.num_passes = argc > 1 ? atoi(argv[1]) : 999;

	if (argc < 3) {
		printf("Use reqrep mode\n");
		conf.server_open = nng_rep0_open;
		conf.client_open = nng_req0_open;
	} else if (0 == strcmp(argv[2], "pair0")) {
		printf("Use pair0 mode\n");
		conf.server_open = nng_pair0_open;
		conf.client_open = nng_pair0_open;
	} else if (0 == strcmp(argv[2], "pair1")) {
		printf("Use pair1 mode\n");
		conf.server_open = nng_pair1_open;
		conf.client_open = nng_pair1_open;
	} else {
		fprintf(stderr, "Error: unknown mode \"%s\", must be pair0 or pair1, default is reqrep.\n", argv[2]);
		return 1;
	}

	if (snprintf(conf.ctlname, PATH_MAX, "ipc:///tmp/repreq-bench.%d", (int)getpid()) >= PATH_MAX) {
		fprintf(stderr, "Error: %s is too long, must be < %u\n", argv[0], PATH_MAX);
		return 1;
	}

	pid = fork();
	if (pid < 0) {
		perror("fork");
		return 1;
	};

	if (pid > 0) {
		run_server();
		printf("SERVER SUCCESSFULLY FINISHED\n");
		wait(NULL);  /* ..wait child process finished */
	} else {
		run_client();
		printf("CLIENT SUCCESSFULLY FINISHED\n");
	}
	return 0;
}

/* END */
