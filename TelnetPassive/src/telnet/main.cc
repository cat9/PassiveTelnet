/*
 * Copyright (c) 1988, 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

char copyright[] =
  "@(#) Copyright (c) 1988, 1990 Regents of the University of California.\n"
  "All rights reserved.\n";

/*
 * From: @(#)main.c	5.4 (Berkeley) 3/22/91
 */
char main_rcsid[] = 
  "$Id: main.cc,v 1.14 1999/08/01 05:06:37 dholland Exp $";

#include "../version.h"

#include <sys/types.h>
#include <getopt.h>
#include <string.h>

#include "ring.h"
#include "externs.h"
#include "defines.h"
#include "proto.h"
#include <cstdlib>
#include <string.h>

/*
 * Initialize variables.
 */
void
tninit(void)
{
    init_terminal();

    init_network();
    
    init_telnet();

    init_sys();

#if defined(TN3270)
    init_3270();
#endif
}

/*
 * note: -x should mean use encryption
 *       -k <realm> to set kerberos realm
 *       -K don't auto-login
 *       -X <atype> disable specified auth type
 */ 
void usage(void) {
    fprintf(stderr, "Usage: %s %s%s%s%s\n",
	    prompt,
	    " [-8] [-E] [-L] [-a] [-d] [-e char] [-l user] [-n tracefile]",
	    "\n\t",
#ifdef TN3270
	    "[-noasynch] [-noasynctty] [-noasyncnet] [-r] [-t transcom]\n\t",
#else
	    "[-r] ",
#endif
	    "[host-name [port]]"
	);
	exit(1);
}

/*
 * main.  Parse arguments, invoke the protocol or command parser.
 */

int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int ch;
	char *user;

	tninit();		/* Clear out things */
#if	defined(CRAY) && !defined(__STDC__)
	_setlist_init();	/* Work around compiler bug */
#endif

	TerminalSaveState();

	if ((prompt = strrchr(argv[0], '/'))!=NULL)
		++prompt;
	else
		prompt = argv[0];

	user = NULL;

	rlogin = (strncmp(prompt, "rlog", 4) == 0) ? '~' : _POSIX_VDISABLE;
	autologin = -1;

	if (autologin == -1)
		autologin = (rlogin == _POSIX_VDISABLE) ? 0 : 1;

	argc -= optind;
	argv += optind;

	if (argc) {
		const char *args[7];
		const char **volatile argp = args;

		if (argc > 2)
			usage();
		*argp++ = prompt;
		*argp++ = argv[0];		/* host */
		if (argc > 1)
			*argp++ = argv[1];	/* port */
		*argp = 0;

		if (sigsetjmp(toplevel, 1) != 0)
			Exit(0);
		if (tn(argp - args, args) == 1)
			return (0);
		else
			return (1);
	}
	(void)sigsetjmp(toplevel, 1);
	for (;;) {
#ifdef TN3270
		if (shell_active)
			shell_continue();
		else
#endif
			command(1, 0, 0);
	}
}
