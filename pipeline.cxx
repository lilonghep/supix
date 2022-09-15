/*******************************************************************//**
 * $Id: pipeline.cxx 1193 2020-07-04 15:03:06Z mwang $
 *
 * implementation of a cyclic pipe with thread support
 * 
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-22 00:15:38
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "pipeline.h"

#include <limits.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>	// write(), open(), read(), ... getpid(), usleep()

#include <iostream>
#include <iomanip>
#include <sstream>

#define MAXLINE		4096


void pipeline_t::initialize()
{
   // rwlock
   int err = pthread_rwlock_init(&rwlock, NULL);
   if (err != 0) {
      std::cout << "pthread_rwlock_init: " << strerror(err) << std::endl;
      exit(err);
   }
   // buffer
   buffer = (unsigned char*)malloc(_max_ * _framesize);
}

void pipeline_t::finalize()
{
   if (buffer != NULL)		free(buffer);

   int err = pthread_rwlock_destroy(&rwlock);
   if (err != 0) {
      std::cout << "pthread_rwlock_destroy: " << strerror(err) << std::endl;
      exit(err);
   }

}

// reset write-out status for the first frame
void pipeline_t::reset_first()
{
   pthread_rwlock_wrlock(&rwlock);
   _first = -1;		// reset first
   _post = 0;		// reset _post
   _pre = 0;		// reset _pre
   pthread_rwlock_unlock(&rwlock);
}

void pipeline_t::print(const char *msg)
{
   // static char str[MAXLINE+1];
   // sprintf(str, "%s\n", sprint(msg).c_str() );
   std::ostringstream oss;
   oss << "\t" << sprint(msg)
       << " max=" << _max_
       << " framesize=" << _framesize
       << " buffer=" << (void*)buffer
       << std::endl;
   fputs(oss.str().c_str(), stdout);
}

//const char *
std::string
pipeline_t::sprint(const char *msg)
{
#ifdef LOCKALL
   int rv = pthread_rwlock_rdlock(&rwlock);
   if (rv) {
      err_ret("%s %d", __func__, rv);
      return "rwlock ERROR";
   }
#endif
   // static char str[MAXLINE];
   // sprintf(str, "PIPELINE %s: IN=%d saved=%d OUT=%d pre=%d/%d post=%d/%d first=%d/%d x %d %p"
   // 	   , msg, _in, _saved, _out, _pre, _pre_max, _post, _post_max, _first, _max_, _framesize, buffer);
   std::ostringstream oss;
   oss << "PIPELINE " << msg << ":"
       << " IN=" << _in
       << " saved=" << _saved
       << " OUT=" << _out
       << " pre=" << _pre << "/" << _pre_max
       << " post=" << _post << "/" << _post_max
       << " first=" << _first
      ;
#ifdef LOCKALL
   pthread_rwlock_unlock(&rwlock);
#endif
   return oss.str();
}

////////////////////////////////////////////////////////////////////////


// pop the top frame off
void ostack_t::print(const char *msg)
{
   char str[MAXLINE];
   sprintf(str, "\t%s osize=%d top=%p tmp=%p\n", sprint(msg), osize, top, tmp);
   fputs(str, stdout);
}

const char*
ostack_t::sprint(const char *msg)
{
   static char str[MAXLINE];
   sprintf(str, "STACK %s: depth=%d/%d", msg, depth, depth_max);
   return str;
}
