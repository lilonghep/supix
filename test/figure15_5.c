#include "apue.h"

#include <sys/time.h>	/* gettimeofday() */


void
err_sys(const char *fmt, ...)
{
   /*
   va_list ap;
   va_start(ap, fmt);
   err_doit(1, errno, fmt, ap);
   va_end(ap);
   exit(1);
   */
}


int
main(void)
{
   int n;
   int fd[2];
   pid_t pid;
   char line[MAXLINE];
   long val;
   int rval;
   struct timeval tv;

   int buff[4096];

   for (int i=0; i < 4096; i++)
      buff[i] = -1;

   system("date");

   if (pipe(fd) < 0)
      err_sys("pipe error");

   if ((pid = fork()) < 0) {
      err_sys("fork error");
   }
   else if (pid > 0) {		/* parent */
      close(fd[0]);
      write(fd[1], "hello pipe\n", 11);

      val = fpathconf(fd[1], _PC_PIPE_BUF);
      printf("PIPE_BUF %ld\n", val);
      val = sysconf(_SC_CLK_TCK);
      printf("CLK_TCK %ld\n", val);

      for (int i=0; i < 10; i++)
	 buff[i] = i*i;
   }
   else {			/* child */
      close(fd[1]);
      n = read(fd[0], line, MAXLINE);
      write(STDOUT_FILENO, line, n);

      rval = gettimeofday(&tv, NULL);
      printf("sec %ld usec %ld\n", tv.tv_sec, (long)tv.tv_usec);

      sleep(1);
      for (int i=0; i < 10; i++)
	 printf("%d: %d\n", i, buff[i]);
      
   }

   
   exit(0);
}
