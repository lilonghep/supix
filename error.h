/*******************************************************************//**
 * $Id: error.h 1146 2020-06-29 03:07:21Z mwang $
 *
 * 
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-23 01:40:15
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#ifndef error_h
#define error_h
#include	<stdarg.h>


/* Nonfatal error related to a system call.
 * Print a message and return. */
void
err_ret(const char *fmt, ...);


/* Fatal error related to a system call.
 * Print a message and terminate. */
void
err_sys(const char *fmt, ...);


/* Fatal error related to a system call.
 * Print a message, dump core, and terminate. */
void
err_dump(const char *fmt, ...);


/* Nonfatal error unrelated to a system call.
 * Print a message and return. */
void
err_msg(const char *fmt, ...);


/* Fatal error unrelated to a system call.
 * Print a message and terminate. */
void
err_quit(const char *fmt, ...);



#endif //~ error_h
