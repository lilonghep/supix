/*******************************************************************//**
 * $Id: test_pipeline.cxx 1145 2020-06-29 02:56:16Z mwang $
 *
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
#include "pipeline.h"
using namespace std;

#define FAKE_RDTIME	10	// usec
#define	WAITTIME	100	// usec
#define TRIG_PERIOD	100	// periodic trigger

int m_fdout;
volatile int m_nchilds = 0;
volatile int m_run_status = 0;
enum run_status_t { RUN_START, RUN_STOP };

pipeline_t m_pipeline;
int max_pre_trigs = 3;
int max_post_trigs = 6;

Timer m_twriter, m_treader;

void wait_in()
{
#ifdef DEBUG
   ostringstream oss;
   oss << __func__ << ": waiting for buffer available" ;
   long total_wait = 0;
#endif
   //while (next == last) {
   while (! m_pipeline.has_buffer()) {
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

int wait_out()
{
   int status = RUN_START;
#ifdef DEBUG
   ostringstream oss;
   oss << __func__ << ": waiting for data available" ;
   long total_wait = 0;
#endif
   while (! m_pipeline.has_data()) {
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

void wait_empty()
{
   ostringstream oss;
   oss << __func__ << ": waiting for buffer empty" ;
   long total_wait = 0;
   //while(trig != next) {
   while(m_pipeline.has_data()) {
      usleep(WAITTIME);
      if (total_wait < 100) oss << "." ;
      total_wait++;
   }
   oss << " " << total_wait*WAITTIME << " usec";
   cout << oss.str() << endl << flush;
}


int m_nwords = m_pipeline.framesize / sizeof(unsigned);

// read into buffer
void buffer_in(long totframes)
{
   while(totframes > 0) {
      wait_in();
   
      unsigned *ptr = (unsigned*)m_pipeline.get_buffer_next();

      // one frame
      for (int i = 0; i < m_nwords; i++) {
	 *ptr++ = i % 100;
      }
      usleep(FAKE_RDTIME);
      
      m_pipeline.step_next();
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
   write_all(m_fdout, buf, nframes * m_pipeline.framesize);
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
   
   unsigned char *ptr = m_pipeline.get_buffer_trig();
   bool triged = ( ntrigs == 0 );
   if (triged) {		// new trigger
      // pre_trigs
      int pre_trigs = m_pipeline.get_pre_trigs();
      if (pre_trigs > 0) {
	 int nbytes = pre_trigs * m_pipeline.framesize;
	 //write_all(m_fdout, ptr - nbytes, nbytes);
	 write_out(ptr - nbytes, pre_trigs);
	 nframes += pre_trigs;
      }
      
      // this trig
      //write_all(m_fdout, ptr, m_pipeline.framesize);
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
	 //write_all(m_fdout, ptr, m_pipeline.framesize);
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
       << " old ("
       << " " << old_saved
       << " " << old_last
       << " " << old_trig
       << " " << old_next
       << " " << old_post_trigs
       << " ) new ("
       << " " << m_pipeline.saved
       << " " << m_pipeline.last
       << " " << m_pipeline.trig
       << " " << m_pipeline.next
       << " " << m_pipeline.post_trigs
       << " )"
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
      int status = wait_out();
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
   m_pipeline.initialize();
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
   wait_empty();
   m_run_status = RUN_STOP;
   wait_childs();
   sleep(1);	// wait for thread exit
   
   m_pipeline.finalize();
   m_pipeline.print();

   ostringstream oss;
   oss << "WR usec/frame (" << m_pipeline.framesize << " bytes)";
   cout << m_twriter.print(oss.str().c_str()) << endl;
   
   return 0;
}
