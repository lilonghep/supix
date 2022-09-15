/*******************************************************************//**
 * $Id: threads.cxx 1154 2020-06-29 05:53:38Z mwang $
 *
 * [delete]
 * test threads for parallel read and write.
 *   - main thread: read data into buffer
 *     child thread: write data out of buffer
 *   - IPC via reader-writer locks (rwlock)
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-18 14:26:50
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "common.h"
#include "util.h"
#include "Timer.h"
using namespace std;

#define FRAMESIZE	4096	// bytes
#define MAXDOTS		40	// ....
#define FAKE_RDTIME	10	// usec
#define	WAITTIME	100	// usec
#define TRIG_PERIOD	100	// periodic trigger

int m_fdout;
volatile int m_nchilds = 0;
volatile int m_run_status = 0;
enum run_status_t { RUN_START, RUN_STOP };

int max_pre_trigs = 3;
int max_post_trigs = 6;

Timer m_twriter("writer"), m_treader("reader");

// a cyclic pipeline as data buffer
struct pipeline_t {
   pthread_rwlock_t rwlock;	// for threads communcation
   // count of frames
   int	saved = 0;		// saved frames
   int	post_trigs = 0;		// left frames post-trigger to write out
   // frame positions
   int	last = 0;		// last saved frame
   int	trig = 0;		// current position for do_trig()
   int	next = 0;		// next position for saving frame
   int	maxframes = 1000;		// in frames, m_pre_triggers + 1
   unsigned char *buffer = NULL;	// head of pipeline
   //
   // functions
   //
   int get_pre_trigs()
   {
      return ( trig >= last ? trig - last : trig - last + maxframes );
   }
   bool has_data()		// check available data
   {
      pthread_rwlock_rdlock(&rwlock);
      bool yes = saved > get_pre_trigs() ;
      pthread_rwlock_unlock(&rwlock);
      return yes;
   }
   bool has_buffer()		// check availabel buffer
   {
      pthread_rwlock_rdlock(&rwlock);
      bool yes = saved < maxframes;
      pthread_rwlock_unlock(&rwlock);
      return yes;
   }
   void saved_plus()		// saved++
   {
      pthread_rwlock_wrlock(&rwlock);
      saved++;
      pthread_rwlock_unlock(&rwlock);
   }
   void saved_minus(int x=1)		// saved -= x
   {
      pthread_rwlock_wrlock(&rwlock);
      saved -= x;
      pthread_rwlock_unlock(&rwlock);
   }
   unsigned char * get_next()	// pointer to next position
   {
      return buffer + next * FRAMESIZE;
   }
   unsigned char * get_last()	// pointer to last position
   {
      return buffer + last * FRAMESIZE;
   }
   unsigned char * get_trig()	// pointer to trig position
   {
      return buffer + trig * FRAMESIZE;
   }
   void initialize();
   void finalize();
   void print(const char *tag = "main");
   void wait_in();		// wait available buffer to read-in data
   int wait_out();		// wait available data to write-out
   void wait_empty();		// wait all data processed
   // inline void step_index(int &idx)	// cyclic ++
   // {
   //    idx++;
   //    if (idx == maxframes) idx = 0;
   // }
   inline void step_next()	// cyclic next++
   {
      next++;
      if (next == maxframes) next = 0;
   }
   inline void step_trig()	// cyclic trig++
   {
      trig++;
      if (trig == maxframes) trig = 0;
   }
   inline void step_last()	// cyclic last++
   {
      last++;
      if (last == maxframes) last = 0;
   }
}
   m_pipeline;

void pipeline_t::initialize()
{
   // rwlock
   int err = pthread_rwlock_init(&rwlock, NULL);
   if (err != 0) {
      cout << "pthread_rwlock_init: " << strerror(err) << endl;
      exit(err);
   }
   // buffer
   buffer = (unsigned char*)malloc(maxframes * FRAMESIZE);
}

void pipeline_t::finalize()
{
   int err = pthread_rwlock_destroy(&m_pipeline.rwlock);
   if (err != 0) {
      cout << "pthread_rwlock_destroy: " << strerror(err) << endl;
      exit(err);
   }

   if (buffer != NULL)
      free(buffer);
}

void pipeline_t::print(const char *tag)
{
   ostringstream oss;
   oss << tag << " pipeline:"
       << " saved = " << saved
       << " post_trigs = " << post_trigs
       << " last = " << last
       << " trig = " << trig
       << " next = " << next
      // << " pre_trigs = " << pre_trigs
       << " maxframes = " << maxframes
       << " buffer = " << hex << static_cast<void*>(buffer) << dec
       << endl;
   fputs(oss.str().c_str(), stdout);
}


void pipeline_t::wait_in()
{
#ifdef DEBUG
   ostringstream oss;
   oss << __func__ << ": waiting for buffer available" ;
   long total_wait = 0;
#endif
   //while (next == last) {
   while (! has_buffer()) {
      usleep(WAITTIME);
#ifdef DEBUG
      if (total_wait < MAXDOTS) oss << "." ;
      total_wait++;
#endif
   }
#ifdef DEBUG
   oss << " " << total_wait*WAITTIME << " usec" << endl;
   // cout << oss.str() << endl << flush; // mixed
   fputs(oss.str().c_str(), stdout);
#endif

}

int pipeline_t::wait_out()
{
   int status = RUN_START;
#ifdef DEBUG
   ostringstream oss;
   oss << __func__ << ": waiting for data available" ;
   long total_wait = 0;
#endif
   while (! has_data()) {
      // stop run?
      if (m_run_status == RUN_STOP) {
	 status = RUN_STOP;
	 break;
      }

      // wait...
      usleep(WAITTIME);
#ifdef DEBUG
      if (total_wait < MAXDOTS) oss << "." ;
      total_wait++;
#endif
   }
#ifdef DEBUG
   oss << " " << total_wait*WAITTIME << " usec" << endl;
   //cout << oss.str() << endl << flush;
   fputs(oss.str().c_str(), stdout);
#endif
   return status;
}

void pipeline_t::wait_empty()
{
   ostringstream oss;
   oss << __func__ << ": waiting for buffer empty" ;
   long total_wait = 0;
   //while(trig != next) {
   while(has_data()) {
      usleep(WAITTIME);
      if (total_wait < 100) oss << "." ;
      total_wait++;
   }
   oss << " " << total_wait*WAITTIME << " usec";
   cout << oss.str() << endl << flush;
}


//~end of pipeline_t
//----------------------------------------------------------------------

int m_nwords = FRAMESIZE / sizeof(unsigned);

// read into buffer
void buffer_in(long totframes)
{
   while(totframes > 0) {
      m_pipeline.wait_in();
   
      unsigned *ptr = (unsigned*)m_pipeline.get_next();

      // one frame
      for (int i = 0; i < m_nwords; i++) {
	 *ptr++ = i % 100;
      }
      usleep(FAKE_RDTIME);
      
      //m_pipeline.step_next();
      m_pipeline.saved_plus();
#ifdef DEBUG
      m_pipeline.print("RD in");
#endif

      totframes--;
   }
}


// interface for writing out data in frames
int write_out(unsigned char *buf, int nframes=1)
{
   m_twriter.start();
   write_all(m_fdout, buf, nframes * FRAMESIZE);
   m_twriter.stop();
   m_twriter.stats(nframes);
   
   return nframes;
}

// do trigger
int do_trig()
{
   static unsigned ntrigs = 0;
   int nframes = 0;
   int old_saved = m_pipeline.saved;
   int old_last = m_pipeline.last;
   int old_trig = m_pipeline.trig;
   int old_next = m_pipeline.next;
   int old_post_trigs = m_pipeline.post_trigs;
   
   unsigned char *ptr = m_pipeline.get_trig();
   bool triged = ( ntrigs == 0 );
   if (triged) {		// new trigger
      // pre_trigs
      int pre_trigs = m_pipeline.get_pre_trigs();
      if (pre_trigs > 0) {
	 int nbytes = pre_trigs * FRAMESIZE;
	 //write_all(m_fdout, ptr - nbytes, nbytes);
	 write_out(ptr - nbytes, pre_trigs);
	 nframes += pre_trigs;
      }
      
      // this trig
      //write_all(m_fdout, ptr, FRAMESIZE);
      write_out(ptr);
      nframes++;
      
      // update trig & last
      m_pipeline.step_trig();
      m_pipeline.last = m_pipeline.trig;

      // update post_trigs
      if (m_pipeline.post_trigs == 0)
	 m_pipeline.post_trigs = max_post_trigs;

      // update saved for threads communication
      m_pipeline.saved_minus(nframes);
   }
   else {			// NO new trigger
      // post_trigs
      if (m_pipeline.post_trigs > 0) {
	 write_out(ptr);
	 //write_all(m_fdout, ptr, FRAMESIZE);
	 nframes++;
	 m_pipeline.post_trigs--;
	 // update trig & last
	 m_pipeline.step_trig();
	 m_pipeline.last = m_pipeline.trig;
	 
	 // update saved for threads communication
	 m_pipeline.saved_minus();
      }
      else {
	 // update trig
	 m_pipeline.step_trig();

	 // update last
	 if (m_pipeline.get_pre_trigs() > max_pre_trigs) {
	    m_pipeline.step_last();
	    // update saved for threads communication
	    m_pipeline.saved_minus();
	 }
      }
   }

   // period trigger
   ntrigs = (ntrigs + 1) % TRIG_PERIOD;

   //   if (nframes > 0) {
   ostringstream oss;
   oss << __func__ << ": #" << ntrigs
       << " triged " << triged
       << " nframes " << nframes
       << " (saved last trig next post_trigs) ="
       << " old"
       << " " << old_saved
       << " " << old_last
       << " " << old_trig
       << " " << old_next
       << " " << old_post_trigs
       << " new"
       << " " << m_pipeline.saved
       << " " << m_pipeline.last
       << " " << m_pipeline.trig
       << " " << m_pipeline.next
       << " " << m_pipeline.post_trigs
       << endl;
   fputs(oss.str().c_str(), stdout);
   //}
   
#ifdef DEBUG
   m_pipeline.print(__func__);
#endif
   
   return nframes;
}


// write out of buffer
void buffer_out()
{
   m_fdout = open_fd("test.dat");

   unsigned long totframes = 0;
   unsigned long wr_frames = 0;
   while(1) {
      // is data ready?
      int status = m_pipeline.wait_out();
      // stop run?
      if (status == RUN_STOP) break;

      int err = do_trig();
      wr_frames += err;
      
      totframes++;
   }
   cout << __func__ << ": frames"
	<< " total = " << totframes
	<< " written = " << wr_frames
	<< endl;
   
   close_fd(m_fdout);
}


// interface for a new thread
//______________________________________________________________________
void * thr_write(void *arg)
{
   (void)(arg);
   printids("child thread start:");
   m_nchilds++;
   buffer_out();
   m_nchilds--;
   printids("child thread return:");
   return ((void*)0);
}


void wait_childs()
{
   ostringstream oss;
   oss << __func__ << ": waiting for child threads exit" ;
   long total_wait = 0;
   while(m_nchilds > 0) {
      if (total_wait < 100) oss << "." ;
      usleep(WAITTIME);
      total_wait++;
   }
   oss << " " << total_wait*WAITTIME << " usec";
   cout << oss.str() << endl << flush;
}


//======================================================================
int
//main(void)
main(int argc, char **argv)
{
   if (argc != 2) {
      cout << "Usage: " << argv[0] << " nframes"
	   << endl;
      exit(0);
   }
   int nframes = atoi(argv[1]);
   
   int err;
   pthread_t ntid;

   m_pipeline.print();
   //m_pipeline.initialize();
   m_pipeline.print();

   err = pthread_create(&ntid, NULL, thr_write, NULL);
   if (err != 0) {
      cout << "cant't create thread: " << strerror(err) << endl;
      exit(err);
   }
   printids("main thread:");
   sleep(1);	// wait for thread start

   buffer_in(nframes);
   
   // wait for emptying pipeline
   m_pipeline.wait_empty();
   m_run_status = RUN_STOP;
   wait_childs();
   sleep(1);	// wait for thread exit
   
   //m_pipeline.finalize();
   m_pipeline.print();

   ostringstream oss;
   oss << "WR usec/frame (" << FRAMESIZE << " bytes)";
   m_twriter.print();
   
   return 0;
}
