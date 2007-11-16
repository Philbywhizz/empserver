/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2007, Dave Pare, Jeff Bailey, Thomas Ruschak,
 *                           Ken Stevens, Steve McClure
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  ---
 *
 *  See files README, COPYING and CREDITS in the root of the source
 *  tree for related information and legal notices.  It is expected
 *  that future projects/authors will amend these files as needed.
 *
 *  ---
 *
 *  main.c: client main function
 * 
 *  Known contributors to this file:
 *     Dave Pare, 1986
 *     Steve McClure, 1998
 *     Ron Koenderink, 2004-2005
 *     Markus Armbruster, 2005
 */

#include <config.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <io.h>
#else
#include <pwd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "ioqueue.h"
#include "misc.h"
#include "proto.h"
#include "tags.h"

#ifdef _WIN32
HANDLE hStdIn;
#endif

#define	RETRY	3

int eight_bit_clean;
int sock;

static volatile sig_atomic_t interrupt;
static void intr(int sig);
static int handleintr(int);

int
main(int ac, char **argv)
{
#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA WsaData;
    int err;
    fd_set readfds;
    struct timeval tm;
    DWORD stdinmode;
    SECURITY_ATTRIBUTES security;
    int bRedirected = 0;
    char unamebuf[128];
#else
    struct sigaction sa;
    fd_set mask;
    fd_set savemask;
    int retry = 0;
#endif
    struct ioqueue server;
    int i, j;
    char *ptr;
    char *auxout_fname;
    FILE *auxout_fp;
    int n;
    char *cname;
    char *pname;
    char *uname;
    char *host;
    char *port;
    int send_kill = 0;
    int utf8 = 0;

#ifdef _WIN32
    /*
     * stdout is unbuffered under Windows if connected to a character
     * device, and putchar() screws up when printing multibyte strings
     * bytewise to an unbuffered stream.  Switch stdout to line-
     * buffered mode.  Unfortunately, ISO C allows implementations to
     * screw that up, and of course Windows does.  Manual flushing
     * after each prompt is required.
     */
    setvbuf(stdout, NULL, _IOLBF, 4096);
    wVersionRequested = MAKEWORD(2, 0);
    err = WSAStartup(wVersionRequested, &WsaData);
    if (err != 0) {
	printf("WSAStartup Failed, error code %d\n", err);
	return FALSE;
    }
#else
    FD_ZERO(&mask);
    FD_ZERO(&savemask);
#endif
    auxout_fname = NULL;
    auxout_fp = NULL;
    for (i = j = 1; i < ac; ++i) {
	ptr = argv[i];
	if (strcmp(ptr, "-2") == 0) {
	    if (i + 1 >= ac) {
		fprintf(stderr, "-2: Missing filename!\n");
		exit(1);
	    }
	    auxout_fname = argv[i + 1];
	    ++i;
	    continue;
	} else if (strcmp(ptr, "-k") == 0) {
	    send_kill = 1;
	    continue;
	} else if (strcmp(ptr, "-u") == 0) {
	    utf8 = eight_bit_clean = 1;
	    continue;
	}
	argv[j] = argv[i];
	++j;
    }
    ac = j;
    if (auxout_fname && (auxout_fp = fopen(auxout_fname, "a")) == NULL) {
	fprintf(stderr, "Unable to open %s for append\n", auxout_fname);
	exit(1);
    }
    getsose();
    port = getenv("EMPIREPORT");
    if (!port)
	port = empireport;
    host = getenv("EMPIREHOST");
    if (!host)
	host = empirehost;
    sock = tcp_connect(host, port);
    cname = getenv("COUNTRY");
    if (ac > 1)
	cname = argv[1];
    pname = getenv("PLAYER");
    if (ac > 2)
	pname = argv[2];
    uname = getenv("LOGNAME");
    if (uname == NULL) {
#ifndef _WIN32
	struct passwd *pwd;

	pwd = getpwuid(getuid());
	if (pwd == NULL) {
	    fprintf(stderr, "You don't exist.  Go away\n");
	    exit(1);
	}
	uname = pwd->pw_name;
#else
	DWORD unamesize;

	unamesize = sizeof(unamebuf);
	if (GetUserName(unamebuf, &unamesize)) {
	    uname = unamebuf;
	    if ((unamesize <= 0 ) || (strlen(uname) <= 0))
		uname = "nobody";
	} else
	    uname = "nobody";
#endif
    }
    if (!login(sock, uname, cname, pname, send_kill, utf8)) {
#ifdef _WIN32
	closesocket(sock);
#else
	close(sock);
#endif
	exit(1);
    }
    ioq_init(&server, 2048);
    io_init();
#ifndef _WIN32
    FD_ZERO(&mask);
    FD_SET(0, &savemask);
    FD_SET(sock, &savemask);
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = intr;
    sigaction(SIGINT, &sa, NULL);
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
    while (FD_ISSET(sock, &savemask)) {
	mask = savemask;
	n = select(sock + 1, &mask, NULL, NULL, NULL);
	if (interrupt) {
	    if (!handleintr(sock))
		break;
	    errno = 0;
	}
	if (n <= 0) {
	    if (errno == EINTR) {
		perror("select");
		(void)close(sock);
		FD_CLR(sock, &savemask);
	    }
	} else {
	    if (FD_ISSET(0, &mask)) {
		if (!termio(0, sock, auxout_fp)) {
		    if (retry++ >= RETRY) {
			FD_CLR(0, &savemask);
		    }
		} else {
		    retry = 0;
		}
	    }
	    if (FD_ISSET(sock, &mask)) {
		if (!serverio(sock, &server))
		    FD_CLR(sock, &savemask);
		else
		    servercmd(&server, auxout_fp);
	    }
	}
    }
#else  /* _WIN32 */
    signal(SIGINT, intr);

    bRedirected = 0;
    tm.tv_sec = 0;
    tm.tv_usec = 1000;

    if (!isatty(fileno(stdin)))
	bRedirected = 1;
    else {
	security.nLength = sizeof(SECURITY_ATTRIBUTES);
	security.lpSecurityDescriptor = NULL;
	security.bInheritHandle = TRUE;
	hStdIn = CreateFile("CONIN$",
			    GENERIC_READ | GENERIC_WRITE,
			    FILE_SHARE_READ | FILE_SHARE_WRITE,
			    &security, OPEN_EXISTING, (DWORD) NULL, NULL);
	
	if (hStdIn == INVALID_HANDLE_VALUE) {
	    printf("Error getting hStdIn.\n");
	    fflush(stdout);
	    exit(-3);
	}
	
	err = GetConsoleMode(hStdIn, &stdinmode);
	if (!err) {
	    printf("Error getting console mode.\n");
	    fflush(stdout);
	    exit(-4);
	} else {
	    stdinmode |= ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT;
	    err = SetConsoleMode(hStdIn, stdinmode);
	    if (!err) {
		printf("Error setting console mode.\n");
		fflush(stdout);
		exit(-5);
	    }
	}
    }
    while (1) {
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	n = select(sock + 1, &readfds, NULL, NULL, &tm);
	if (interrupt) {
	    if (!handleintr(sock))
		break;
	    errno = 0;
	}
	if (n < 0) {
	    if (errno == EINTR) {
		errno = WSAGetLastError();
		perror("select");
		(void)closesocket(sock);
		break;
	    }
	} else {
	    if (bRedirected == 1) {
		if (!termio(0, sock, auxout_fp))
		    bRedirected = -1;
	    } else if (bRedirected == 0) {
		if (WaitForSingleObject(hStdIn, 10) != WAIT_TIMEOUT) {
		    termio(-1, sock, auxout_fp);
		    FlushConsoleInputBuffer(hStdIn);
		}
	    }
	    if (FD_ISSET(sock, &readfds)) {
		if (!serverio(sock, &server))
		    break;
		else
		    servercmd(&server, auxout_fp);
	    }
	}
    }
    if (bRedirected == 0)
	CloseHandle(hStdIn);
#endif /* _WIN32 */
    ioq_drain(&server);
#ifdef _WIN32
    (void)closesocket(sock);
#else
    (void)close(sock);
#endif
    return 0;			/* Shut the compiler up */
}

static void
intr(int sig)
{
    interrupt = 1;
#ifdef _WIN32
    signal(SIGINT, intr);
#endif
}

static int
handleintr(int s)
{
    if (interrupt) {
	/* tacky, but it works */
#if !defined(_WIN32)
	if (write(s, "\naborted\n", 1 + 7 + 1) <= 0)
#else
	if (send(s, "\naborted\n", 1 + 7 + 1, 0) <= 0)
#endif
	    return 0;
	interrupt = 0;
    }
    return 1;
}
