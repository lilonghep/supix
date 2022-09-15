/*******************************************************************//**
 * $Id: util.cxx 1193 2020-07-04 15:03:06Z mwang $
 *
 * utilities
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-18 16:21:20
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "util.h"
#include "error.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <unistd.h>


// interface to open(2)
// chmod(2)
//   #define S_IRWXU 0000700    /* RWX mask for owner */
//   #define S_IRUSR 0000400    /* R for owner */
//   #define S_IWUSR 0000200    /* W for owner */
//   #define S_IXUSR 0000100    /* X for owner */
//  
//   #define S_IRWXG 0000070    /* RWX mask for group */
//   #define S_IRGRP 0000040    /* R for group */
//   #define S_IWGRP 0000020    /* W for group */
//   #define S_IXGRP 0000010    /* X for group */
//  
//   #define S_IRWXO 0000007    /* RWX mask for other */
//   #define S_IROTH 0000004    /* R for other */
//   #define S_IWOTH 0000002    /* W for other */
//   #define S_IXOTH 0000001    /* X for other */
//  
//   #define S_ISUID 0004000    /* set user id on execution */
//   #define S_ISGID 0002000    /* set group id on execution */
//   #define S_ISVTX 0001000    /* save swapped text even after use */
//
//______________________________________________________________________
int open_fd(const char *path, int oflag)
{
   mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ;

   int fd = open(path, oflag, mode);
   if (fd == -1) {
      if (errno == ENODEV)
	 fprintf(stderr, "(Maybe %s a read-only file?)\n", path);
      err_sys("open(\"%s\", ...)", path);
   }

   return fd;
}

// interface to close(2)
//______________________________________________________________________
int close_fd(int fd, const char *msg)
{
   int retval = close(fd);
   if (retval == -1) {
      err_sys("close %s", msg);
   }
   return retval;
}

/*
   Plain read() may not read all bytes requested in the buffer, so
   read_all() loops until all data was indeed read, or exits in
   case of failure, except for EINTR. The way the EINTR condition is
   handled is the standard way of making sure the process can be suspended
   with CTRL-Z and then continue running properly.

   The function has no return value, because it always succeeds (or exits
   instead of returning).

   The function doesn't expect to reach EOF either.
*/
//int read_all(int fd, unsigned char *buf, int nbytes)
ssize_t read_all(int fd, unsigned char *buf, size_t nbytes)
{
   size_t received = 0;
   int rc;

   while (received < nbytes) {
      rc = read(fd, buf + received, nbytes - received);

      // if ((rc < 0) && (errno == EINTR))
      // 	 continue;
      if ((rc < 0) && (errno == EINTR)) {
	 err_ret("%s EINTR", __func__);
	 break;
      }
      
      if (rc < 0) {
	 perror("read_all() failed to read");
	 exit(1);
      }

      if (rc == 0) {
	 fprintf(stderr, "Reached read EOF (?!): %zu bytes read\n", received);
	 // exit(1);
	 // return rc;
	 break;
      }

      received += rc;
   }

   return received;
}

// write raw date into files
//______________________________________________________________________
ssize_t write_all(int fd, unsigned char *buf, size_t nbytes)
{
   size_t sent = 0;
   ssize_t rc;

   while (sent < nbytes) {
      rc = write(fd, buf + sent, nbytes - sent);

      if ((rc < 0) && (errno == EINTR))
	 continue;

      if (rc < 0) {
	 perror("write_all() failed to write");
	 exit(1);
      }

      if (rc == 0) {
	 fprintf(stderr, "Reached write EOF (?!)\n");
	 exit(1);
      }

      sent += rc;
   }

   return sent;
}

// threads
//======================================================================

// print thread information
void printids(const char *s)
{
   pid_t	pid;
   pthread_t	tid;

   pid = getpid();
   tid = pthread_self();
   printf("THREAD %s pid %u tid %lu (0x%lx)\n", s, (unsigned)pid, (unsigned long)tid, (unsigned long)tid);
}

//======================================================================
//
// math
//
//======================================================================

// y = (x1 + x2)/(x0 + x2)
void variance_ratio3(double x0, double V0, double x1, double V1, double x2, double V2, double& y, double& Vy)
{
   if (x0 + x2 == 0) {
      y = -1;
      Vy = 0;
      return;
   }
   y = (x1 + x2) / (x0 + x2);
   double d0 = -(x1 + x2) / (x0 + x2) / (x0 + x2);	// d_i = dy/dx_i
   double d1 = 1./(x0 + x2);
   double d2 = (x0 - x1) / (x0 + x2) / (x0 + x2);
   Vy = d0*d0*V0 + d1*d1*V1 + d2*d2*V2;
#ifdef DEBUG
   printf("%s: (xi, Vi) = (%f, %f) (%f, %f) (%f, %f), (y, Vy) = (%f, %f)\n"
	  , __func__, x0, V0, x1, V1, x2, V2, y, Vy);
#endif
}



// y = x / (1 + eps*(alpha - 1))
//   (x, eps, alpha) = (x0, x1, x2)
void variance_xea(double x0, double V0, double x1, double V1, double x2, double V2, double& y, double& Vy)
{
   double z = 1 + x1 * (x2 - 1);
   if (z <= 0) {
      y = -1;
      Vy = 0;
      return;
   }
   y = x0 / z;
   double d0 = 1/z;
   double d1 = -x0 * (x2 - 1) / (z*z);
   double d2 = -x0 * x1 / (z*z);
   Vy = d0*d0*V0 + d1*d1*V1 + d2*d2*V2;
}
