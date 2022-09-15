/*******************************************************************//**
 * $Id: error.cxx 1177 2020-06-30 07:05:39Z mwang $
 *
 * 
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-23 01:38:34
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "error.h"

#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/types.h>
#include	<unistd.h>

#define	MAXLINE	4096			/* max line length */

//char	*pname = NULL;		/* caller can set this from argv[0] */


/* Print a message and return to caller.
 * Caller specifies "errnoflag". */
static void
err_doit(int errnoflag, const char *fmt, va_list ap);


/* Nonfatal error related to a system call.
 * Print a message and return. */

void
err_ret(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, fmt, ap);
	va_end(ap);
	return;
}

/* Fatal error related to a system call.
 * Print a message and terminate. */

void
err_sys(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, fmt, ap);
	va_end(ap);
	exit(1);
}

/* Fatal error related to a system call.
 * Print a message, dump core, and terminate. */

void
err_dump(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, fmt, ap);
	va_end(ap);
	abort();		/* dump core and terminate */
	exit(1);		/* shouldn't get here */
}

/* Nonfatal error unrelated to a system call.
 * Print a message and return. */

void
err_msg(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(0, fmt, ap);
	va_end(ap);
	return;
}

/* Fatal error unrelated to a system call.
 * Print a message and terminate. */

void
err_quit(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(0, fmt, ap);
	va_end(ap);
	exit(1);
}

/* Print a message and return to caller.
 * Caller specifies "errnoflag". */

static void
err_doit(int errnoflag, const char *fmt, va_list ap)
{
   int		errno_save;
   char	buf[MAXLINE];

   errno_save = errno;		/* value caller might want printed */
   vsprintf(buf, fmt, ap);
   if (errnoflag)
      sprintf(buf+strlen(buf), " : [%d] %s", errno_save, strerror(errno_save));
   strcat(buf, "\n");
   fflush(stdout);		/* in case stdout and stderr are the same */
   fputs(buf, stderr);
   fflush(NULL);		/* flushes all stdio output streams */
   return;
}
