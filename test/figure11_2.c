#include "apue.h"
#include <pthread.h>

int buff[10];
pthread_t ntid;

void
printids(const char *s)
{
   pid_t	pid;
   pthread_t	tid;

   pid = getpid();
   tid = pthread_self();
   printf("%s pid %u tid %u (0x%x)\n", s, (unsigned)pid, (unsigned)tid, (unsigned)tid);
}

void *
thr_fn(void *arg)
{
   printids("new thread:");
   sleep(2);
   
   // read buffer
   for (int i=0; i < 10; i++) {
      //buff[i] = i*i;
      printf("new %d: %d\n", i, buff[i]);
      sleep(1);
   }
   
   return ((void*)0);
}

int
main(void)
{
   int	err;

   for (int i=0; i < 10; i++)
      buff[i] = -1;
   
   err = pthread_create(&ntid, NULL, thr_fn, NULL);
   if (err != 0)
      err_quit("cant't create thread: %s\n", strerror(err));
   printids("main thread:");
   sleep(1);

   // write buffer
   for (int i=0; i < 10; i++) {
      buff[i] = 2*i;
      printf("main %d: %d\n", i, buff[i]);
      sleep(1);
   }

   printf("sizeof(pthread_t) = %lu\n", sizeof(pthread_t));
   
   exit(0);
}
