/*******************************************************************//**
 * $Id: pipeline.h 1206 2020-07-10 09:59:49Z mwang $
 *
 * definition of a cyclic pipe with thread support
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-21 23:16:03
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#ifndef pipeline_h
#define pipeline_h
#include "error.h"

#include <stdio.h>
#include <stdlib.h>	// malloc(), free()
#include <string.h>
#include <pthread.h>

#include <string>

// T: trig > 0, P: post > 0
enum OUT_MODE_t { O_T0P0, O_T1P0, O_T0P1, O_T1P1, O_NOISE };

//
// a thread-safe cyclic pipeline as data buffer
//   RD = read-in thread
//   WR = write-out thread
//______________________________________________________________________
typedef struct pipeline_t
{
   pipeline_t(int fs=4096, int mx=1000, int prmx=0, int pomx=0)
      : _framesize(fs)
      , _max_(mx), _pre_max(prmx), _post_max(pomx)
   {
      if  (_max_ <= _pre_max)	_max_ = 1 + _pre_max;
      _in = _out = _saved = _pre = _post = 0;
      _first = -1;
      initialize();
   };

   ~pipeline_t()
   {  finalize();  }

   bool is_full();		// RD: is pipeline full?
   bool next_in(bool);		// RD: update for next in frame
   
   bool is_new();		// WR: is new data available?
   bool is_first();		// WR: is the first of consecutive frames?
   void reset_first();		// WR: reset WR status
   bool is_post()		// WR: is _out a post frame?
   { return _post > 0; }
   int get_pre()		// WR: return number of pre-frames
   { return _pre; }
   void next_out(OUT_MODE_t x=O_NOISE);	// WR: update next out according to mode

   unsigned char * get_in_ptr();	// RD: get pointer to in-index
   unsigned char * get_out_ptr();	// WR: get pointer to out-index
   unsigned char * get_pre_ptr(int n);	// WR: get pointer to n-th frame in pre-frames

   void initialize();
   void finalize();
   //const char * sprint(const char *msg = "");
   std::string sprint(const char *msg = "");	// status information
   void print(const char *msg = "");		// all information

private:
   int	_in;
   int	_out;
   int	_saved;
   int	_pre;
   int	_post;
   int	_first;		// -1 = non-first, non-negative = first-frame
   int	_framesize;	// bytes per frame
   int	_max_;
   int	_pre_max;
   int	_post_max;
   pthread_rwlock_t rwlock;	// for threads communcation
   unsigned char* buffer;	// head of pipeline
   
}
   pipeline_t
   ;

//----------------------------------------------------------------------

// NOT need to lock as
//   RD: true to wait, false to read
//   WR: true -> false, false -> false
inline bool pipeline_t::is_full()
{
   //#ifdef LOCKALL
   pthread_rwlock_rdlock(&rwlock);
   bool yes = _saved + _pre == _max_;
   pthread_rwlock_unlock(&rwlock);
   return yes;
// #else
//    return _saved + _pre == _max_;
// #endif
}

// NO need to lock
//   WR: true to write, false to wait
//   RD: true -> true, false -> true
inline bool pipeline_t::is_new()
{
// #ifdef LOCKALL
   pthread_rwlock_rdlock(&rwlock);
   bool yes = _saved > 0;
   pthread_rwlock_unlock(&rwlock);
   return yes;
// #else
//    return _saved > 0;
// #endif
}

// RD: update
// - set the first frame index if true
// return
//   true if the last first frame not yet processed by WR
inline bool pipeline_t::next_in(bool is_1st)
{
   bool wait = false;
   pthread_rwlock_wrlock(&rwlock);
   if (is_1st) {			// before _in++
      if (_first >= 0) {		// wait WR finishing last first
	 wait = true;
#ifdef DEBUG
	 printf("%s last_first=%d -> %d\n", __PRETTY_FUNCTION__, _first, _in);
#endif
      }
      else
	 _first = _in;
   }
   if (!wait) {
      _in++;
      if (_in == _max_) _in = 0;
      _saved++;
   }
   pthread_rwlock_unlock(&rwlock);
   return wait;
}

//----------------------------------------------------------------------

// WR: reset <first> if true
inline bool pipeline_t::is_first()
{
   pthread_rwlock_wrlock(&rwlock);
   bool yes = _first == _out;
   // if (! yes) return false;
   // if (_first != _out)	return false;
   // reset_first();
   // return true;
   if (yes) {		// flag processed
#ifdef DEBUG
      printf("%s first=%d\n", __PRETTY_FUNCTION__, _first);
#endif
      _first = -1;		// reset first
      _post = 0;		// reset _post
      _pre = 0;		// reset _pre
   }
   pthread_rwlock_unlock(&rwlock);
   return yes;
}

inline void pipeline_t::next_out(OUT_MODE_t mode)
{
   pthread_rwlock_wrlock(&rwlock);

   _saved--;
   if (_saved < 0)
      printf("%s: ERROR!!! saved=%d\n", __PRETTY_FUNCTION__, _saved);

   // after saved
   if (! (_saved == 0 && _out == _in) ) {
      _out++;
      if (_out == _max_) _out = 0;
   }
   
   // update _pre & _post
   switch (mode) {
   case O_T0P0:		// 0
      if (_pre < _pre_max)	_pre++;
      break;
   case O_T1P0:		// 1
      _pre = 0;
      _post = _post_max;
      break;
   case O_T0P1:		// 2
      _post--;
      break;
   case O_T1P1:		// 3
   default:		// noise, ...
      // do nothing
      break;
   }

   pthread_rwlock_unlock(&rwlock);
}

//----------------------------------------------------------------------

inline unsigned char * pipeline_t::get_in_ptr()	// pointer to next position
{
   pthread_rwlock_rdlock(&rwlock);
   unsigned char *rv = buffer + _in * _framesize;
   pthread_rwlock_unlock(&rwlock);
   return rv;
   //return buffer + _in * _framesize;
}

inline unsigned char * pipeline_t::get_out_ptr()	// pointer to trig position
{
   pthread_rwlock_rdlock(&rwlock);
   unsigned char *rv = buffer + _out * _framesize;
   pthread_rwlock_unlock(&rwlock);
   return rv;
   // return buffer + _out * _framesize;
}

// return n-th frame address in pre frames.
inline unsigned char * pipeline_t::get_pre_ptr(int n)
{
   pthread_rwlock_rdlock(&rwlock);
   int pre = _out - _pre + n;
   if (pre < 0) pre += _max_;
   unsigned char* rv = buffer + pre * _framesize;
   pthread_rwlock_unlock(&rwlock);
   return rv;
   // int pre = _out - _pre + n;
   // if (pre < 0) pre += _max_;
   // return buffer + pre * _framesize;
}

////////////////////////////////////////////////////////////////////////

// a stack-like buffer for saving objects.
//   ostack_t(osize, depth_max)
//______________________________________________________________________
typedef struct ostack_t {
   // constructor
   ostack_t(int os, int n)
      : osize(os)	// object size in byte
      , depth_max(n)	// max number of objects
   {
      depth	= 0;	// #objects saved in buffer
      top = (unsigned char*)malloc(depth_max * osize);
      tmp = (unsigned char*)malloc(osize);
   }

   // destructor
   ~ostack_t()
   {
      free(top);
      free(tmp);
   }

   unsigned char * get_top()	// top address of the stack
   { return top; }

   int push();		// push saved objects down for a new object
   int pop();		// pop the top object out
   int bubble();	// move the bottom object to top
   int get_depth();	// number of objects saved
   void print(const char *msg="");
   const char* sprint(const char *msg="");
   
private:
   int	depth;		// frames in buffer
   int	osize;		// bytes per frame
   int	depth_max;	// capability in frames
   unsigned char *top;
   unsigned char *tmp;	// temporary buffer of a frame size
}
   ostack_t;

/* man
     void *
     memmove(void *dst, const void *src, size_t len);

 */

inline int ostack_t::get_depth()
{
   return depth;
}

// push saved frames down to make free buffer for a new frame.
inline int ostack_t::push()
{
   if (depth == 0) {
      depth++;
      return depth;
   }
   if (depth == depth_max)	depth--;
   memmove(top+osize, top, depth*osize);
   depth++;
   return depth;
}

// move the bottom object to the top
inline int ostack_t::bubble()
{
   if (depth < 2)	return depth;
   // move the bottom frame to tmp
   memmove(tmp, top+osize*(depth-1), osize);
   // push other frames down
   memmove(top+osize, top, (depth-1)*osize);
   // move tmp to the top
   memmove(top, tmp, osize);
   return depth;
}


// pop the top frame off
inline int ostack_t::pop()
{
   //printf("%s -- start %d\n", __PRETTY_FUNCTION__, depth);
   if (depth == 0) {
      printf("%s -- ERR empty stack!!!\n", __PRETTY_FUNCTION__);
      // empty stack!!!
      return -1;
   }
   depth--;
   if (depth > 0)
      memmove(top, top+osize, depth*osize);
   //printf("%s -- end %d\n", __PRETTY_FUNCTION__, depth);
   return depth;
}


#endif //~ pipeline_h
