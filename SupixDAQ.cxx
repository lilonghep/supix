/*******************************************************************//**
 * $Id: SupixDAQ.cxx 1219 2020-07-15 08:56:47Z mwang $
 *
 * 
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-22 12:39:44
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "SupixDAQ.h"
#include "error.h"

#define hexout(x)	showbase << hex << (x) << dec << noshowbase

// ROOT headers
#include "TFile.h"

// C headers
//
#include <unistd.h>	// write(), open(), read(), ... getpid()
#include <fcntl.h>	// O_WRONLY
#include <signal.h>	// signal
#include <errno.h>	// errno
#include <stdio.h>	// perror
#include <sys/stat.h>	// mode_t
#include <time.h>

// C++ headers
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>
using namespace std;

#define MAXLINE	4096

// DAQ error numbers
enum ERROR_t { E_OK, E_INTEGRITY, E_CONTINUE,	// placeholder for continue run
	       					// following to RUN_STOP
	       E_LASTPIX, E_ISFULL, E_EOF, E_LAST1ST,
	       E_RUNSTOP
};

enum VERBOSITY_t { V_NORM, V_MORE, V_DEBUG };

// counts
enum counts_t
   { LASTPIX, SETFIRST, NONINTEGRITY, WAITFIRST, WAITNEW, ISFULL, TIMEOUT
     , NCOUNTS
   };
const char *x_counts_name[NCOUNTS] =
   { "LASTPIX", "SETFIRST", "NONINTEGRITY", "WAITFIRST", "WAITNEW", "ISFULL", "TIMEOUT"
   };
unsigned long x_counts[NCOUNTS] = { 0 };


// timers
enum timers_t
   {
    Trd_fifo, Twr_raw, Twr_root
    , Tstart_run, Tread
    , Tchild_run, Tprocd, Tdecode_frame, Tis_first
    , Tdo_trig, Ttriged, Twrite, Tskip, Tnext_out
    , NTIMERS
   };
const char *timers_name[NTIMERS] =
   {
    "read FIFO", "write RAW", "write ROOT"
    // reader
    , "start_run", "read"
    // writer
    , "child_run", "process", "decode_frame", "is_first"
    , "do_trig", "@triged", "@write", "@skip", "@next_out"
   };
Timer *x_timers[NTIMERS] = { NULL };
// usage:
//   Timer m_timer("\tread FIFO");
//   m_timer.start();
//   m_timer.stop();


//
// signals
// - static members = global
//======================================================================

volatile run_status_t SupixDAQ::m_run_status = RUN_FIRST ;
int SupixDAQ::signals[] = { SIGINT, SIGQUIT, SIGTERM };
// 4 EINTR Interrupted function call.  An asynchronous signal (such as SIGINT or SIGQUIT)
// keep the handler as simple as possible.
// NOT put i/o in it!!
void SupixDAQ::sig_handler(int sig)
{  TRACE;
   (void)(sig);	// avoid unused parameter warning
   m_run_status = RUN_STOP;
   printf("%s signal=%d run_status=%d\n", __func__, sig, m_run_status);
}

/***********************************************************************/

//
// constructor(s)
//______________________________________________________________________
SupixDAQ::SupixDAQ()
   : m_lock_file("/tmp/supix_lock")
{  TRACE;

   // register signals
   int nsignals = sizeof(signals) / sizeof(int);
   for (int i=0; i < nsignals; i++) {
      errno = 0;	// reset errno for signal()
      signal(signals[i], sig_handler);
      CERR << "register signal " << signals[i] << ": " << errno << endl;
      if (errno) {
	 char msg[80];
	 sprintf(msg, "register signal %d\n", signals[i]);
	 perror(msg);
   	 // string msg = "register signal ";
   	 // msg += to_string(signals[i]);
   	 // perror(msg.c_str() );
      }
   }

   // default configuration
   //----------------------
   m_dev_mem	= (char*)"/dev/xillybus_mem_8"; // Xillybus dual port memory --LongLI
   m_dev_fifo	= (char*)"/dev/xillybus_read_32"; // Xillybus FIFO 32-bits

   m_runinfo.daq_mode	= M_NORMAL;
   // m_noise_run	= false;
   m_verbosity	= 0;
   m_fd_mem	= -1;
   m_fd_fifo	= -1;
   m_fd_raw	= -1;		// raw data file
   m_write_raw	= false;
   m_write_root	= false;
   m_filesize_max	= (long)2*GB;	// max bytes/file
   m_filesize_raw	= 0;
   m_filesize_root	= 0;
   m_tfile	= NULL;
   m_tree	= NULL;
   m_frame		= 0;	// frame id, starting 0
   m_trig	= 0;		// trigger pattern
   m_frame_1st	= true;		// default be first frame
   m_pre_adc	= NULL;
   m_pixel_adc	= NULL;
   m_pre_cds	= NULL;
   m_pixel_cds	= NULL;
   m_mode_debug	= false;	// debug mode
   m_timewait	= 10;		// usec
   m_timeout	= 1000000;	// sec
   m_maxframe	= 0;		// 0 = infinity
   m_pipeline	= NULL;
   m_pipeline_max = 10000;	// total frames
   m_buffer = NULL;		// temporary for a frame
   m_childs	= 0;		// number of child threads
   m_npixs	= 0;

   m_threshold	= (double*)(m_runinfo.trig_cds);
}


//
// destructor
//______________________________________________________________________
SupixDAQ::~SupixDAQ()
{  TRACE;
   LOG << "THE END" << endl;
}


//______________________________________________________________________
void SupixDAQ::initialize()
{  TRACE;
   if (m_runinfo.trig_cds_x < 1 || m_runinfo.trig_period <= 1) {
	// m_consecutive = true;
	m_runinfo.daq_mode = M_CONTINUOUS;
   }
   // else {
   // 	m_consecutive = false;
   // }

   // daq mode
   if (m_runinfo.daq_mode == M_NOISE) {
      m_cds_noise = new RecurStats[NPIXS];
      m_adc_noise = new RecurStats[NPIXS];
   }
   else if (m_runinfo.daq_mode == M_CONTINUOUS) {
      m_runinfo.pre_trigs	= 0;
      m_runinfo.post_trigs	= 0;
   }
   
   // lock before any action
   int rv = 0;
   if (m_mode_debug)
      rv = getpid();
   else
      rv = lock_run();
   m_pid = rv;
   
   m_fd_mem	= open_fd(m_dev_mem,  O_WRONLY);
   m_fd_fifo	= open_fd(m_dev_fifo, O_RDONLY);

   // after nrow and ncol set
   m_pipeline = new pipeline_t(FRAMESIZE, m_pipeline_max,
			       m_runinfo.pre_trigs, m_runinfo.post_trigs);
   
   m_buffer = (unsigned char*)malloc(FRAMESIZE);

   // stacks for decoded data
   int maxfs	= m_runinfo.pre_trigs + 1;
   if (maxfs <= 1)
      maxfs = 2;	// at least 2 from cds calculation
   int adc_size	= sizeof(adc_t) * NROWS * NCOLS;	// object size
   m_pre_adc	= new ostack_t(adc_size, maxfs);
   m_pixel_adc	= (adc_t*)(m_pre_adc->get_top());
   m_pixel_last	= (adc_t*)(m_pre_adc->get_top() + adc_size);
   
   int cds_size	= sizeof(cds_t) * NROWS * NCOLS;
   m_pre_cds	= new ostack_t(cds_size, maxfs);
   m_pixel_cds	= (cds_t*)(m_pre_cds->get_top());

   LOG << "STACK pointers:"
       << " [pre_adc] " << (void*)(m_pre_adc->get_top())
       << " pixel_adc=" << (void*)m_pixel_adc
       << " pixel_last=" << (void*)m_pixel_last
       << " adc_size=" << adc_size
       << " [pre_cds] " << (void*)(m_pre_cds->get_top())
       << " pixel_cds=" << (void*)m_pixel_cds
       << " cds_size=" << cds_size
       << endl;

   // instantiate timers
   for (int i=0; i < NTIMERS; i++) {
      x_timers[i] = new Timer(timers_name[i]);
   }
   
   // RunInfo timing start
   m_runinfo.set_time_start();
   build_pathbase();		// after m_runinfo.set_time_start()

   print(__PRETTY_FUNCTION__);
   
}


//______________________________________________________________________
void SupixDAQ::finalize()
{  TRACE;

   // RunInfo timing stop
   m_runinfo.set_time_stop();
   //if (! m_write_root)		m_runinfo.Print();

   // if (m_noise_run) {
   if (m_runinfo.daq_mode == M_NOISE) {
      write_noise();
      delete[] m_cds_noise;
      delete[] m_adc_noise;
   }
   
   if (m_fd_mem >= 0)		close_fd(m_fd_mem, m_dev_mem);
   if (m_fd_fifo >= 0)		close_fd(m_fd_fifo, m_dev_fifo);

   if (m_fd_raw >= 0)		close_raw();
   if (m_tfile)			close_root();

   print(__PRETTY_FUNCTION__);
   if (m_buffer)		free(m_buffer);
   if (m_pre_adc)		delete m_pre_adc;
   if (m_pre_cds)		delete m_pre_cds;
   if (m_pipeline)		delete m_pipeline;

   if (m_runinfo.daq_mode != M_NOISE) {
      // timing
      recommend();		// before x_timers[] deleted
      LOG << "timing..." << endl;
      for (int i=0; i < NTIMERS; i++) {
	 x_timers[i]->print();
	 delete x_timers[i];
      }
      //COUT << endl;
   }
   
   // unlock after all actions
   if (! m_mode_debug) {
      // remove the lock file
      string shcmd = "rm ";
      shcmd += m_lock_file;
      int rv = system(shcmd.c_str() );
      if (rv) {
	 perror(m_lock_file);
      }
   }

}

#ifdef DEBUG
#define DBG_RUN(x)						\
   if (m_verbosity >= V_DEBUG)					\
      COUT << "DBG#" << nloop << " " << __func__ <<":" << (x)	\
	   << " run_status=" << m_run_status			\
	   << " nreads=" << m_runinfo.nreads			\
	   << " nsaved=" << m_runinfo.nsaved			\
	   << " wr_mode=" << m_wr_mode				\
	   << " " << m_pipeline->sprint()			\
	   << endl
#else
#define DBG_RUN(x)
#endif

// run control of main thread
//______________________________________________________________________
void SupixDAQ::start_run()
{  TRACE;

   // select the pixel array to read
   select_chip_addr();

   // after initialize() to ensure trig_cds_x and chip_addr having been set.
   set_trig_cds();

   // open...
   new_outfiles();
   
   // main loop
   alt_run_status(RUN_FIRST);
   unsigned long nloop = 0;
   int rv;
   do {
      nloop++;
      DBG_RUN("loop-head");

      x_timers[Tstart_run]->start();

      rv = reader_run();
      if (rv > E_CONTINUE) {
	 alt_run_status(RUN_STOP);
      }
      
      // information
      if (nloop%1000 == 0)
	 LOG << "#" << nloop << " " << sprint() << endl;
      x_timers[Tstart_run]->stop();	// for whole loop
      DBG_RUN("loop-tail");
   }
   while (! is_run_stop() );		//~main loop
   
   LOG << sprint("RETURN") << endl;
}

// read FIFO a frame into pipeline
//______________________________________________________________________
int SupixDAQ::reader_run()
{  TRACE;
      
   // locate FIFO to the beginning of a frame
   //wm: to test, 1 enough?
   if (m_run_status == RUN_FIRST) {
      DBG_RUN("RUN_FIRST");
      //if (m_locate_last_pixel) {
      x_counts[LASTPIX]++;
      int rv = locate_last_pixel();
      if (rv < 0) {
	 CERR << "locate_last_pixel: " << rv << endl;
	 alt_run_status(RUN_STOP);
	 return E_LASTPIX;
      }
      // else
      //    m_locate_last_pixel = false;
   }

   // is pipeline full?
   while(m_pipeline->is_full() ) {
      DBG_RUN("is_full");
      x_counts[ISFULL]++;
      if (wait_timeout("ISFULL") ) {
	 alt_run_status(RUN_STOP);
	 CERR << sprint("TIMEOUT in waiting pipeline non-full") << endl;
	 // m_runinfo.Print();
	 // break;
	 return E_ISFULL;
      }
   }
   // if (is_run_stop() )	break;

   x_timers[Tread]->start();		// for saved frames
      
   // read fifo a frame
   // - when reading end-of-file from a file...
   int rv = read_fifo(m_pipeline->get_in_ptr());
   //if (read_fifo(m_pipeline->get_in_ptr()) == 0) {
   if (rv == 0) {
      DBG_RUN("EOF");
      alt_run_status(RUN_STOP);
      return E_EOF;
   }
   m_runinfo.nreads++;

   // check data integrity
   if (check_integrity() > 0) {
      x_counts[NONINTEGRITY]++;
      alt_run_status(RUN_FIRST);	// to locate_last_pixel
      DBG_RUN("integrity");
      //continue;	// discard current frame
      return E_INTEGRITY;
   }

   // save the frame
   // - update pipeline
   //DBG_RUN("pre-saved");
   bool yes = m_run_status == RUN_FIRST;
   while (m_pipeline->next_in(yes) ) {
      // wait the last first frame being processed by writer.
      x_counts[WAITFIRST]++;
      if (wait_timeout("WAITFIRST") ) {
	 x_counts[TIMEOUT]++;
	 alt_run_status(RUN_STOP);
	 CERR << sprint("TIMEOUT in waiting last-first frame done") << endl;
	 return E_LAST1ST;
      }
   }

   if (yes /* m_run_status == RUN_FIRST */) {
      x_counts[SETFIRST]++;
      alt_run_status(RUN_START);
      LOG << sprint("SETFIRST") << endl;
   }
   // else {
   // 	 m_pipeline->saved_plus();	// head or tail?
   // }
   DBG_RUN("post-saved");

   m_runinfo.nsaved++;		// before RUN_STOP condition of maxframe
   // stop run?
   if ( m_maxframe > 0 && m_runinfo.nreads == m_maxframe ) {
      DBG_RUN("RUN_STOP");
      alt_run_status(RUN_STOP);
   }
      
   x_timers[Tread]->stop();		// for saved frames
   return E_OK;
}

// write a frame out of pipeline
//______________________________________________________________________
int SupixDAQ::writer_run()
{  TRACE;

   // wait for new data
   while (! m_pipeline->is_new() ) {
      DBG_RUN("is_new");
      x_counts[WAITNEW]++;
      if (is_run_stop() ) {
	 LOG << sprint("RETURN is_new()") << endl;
	 return E_RUNSTOP;
      }
      usleep(m_timewait);
   }

   x_timers[Tprocd]->start();		// for processed frames

   // is first? reset trig
   DBG_RUN("is_first");
   x_timers[Tis_first]->start();
   if (m_pipeline->is_first())
      m_frame_1st = true;
   else if (m_frame_1st)
      m_frame_1st = false;
   x_timers[Tis_first]->stop();
      
   // decode data
   DBG_RUN("decode_frame");
   x_timers[Tdecode_frame]->start();
   decode_frame();
   x_timers[Tdecode_frame]->stop();

   // trigger & data acquisition
   DBG_RUN("do_trig");
   x_timers[Tdo_trig]->start();
   do_trig();
   x_timers[Tdo_trig]->stop();

   // global frame id, at the end of processing a frame.
   m_frame++;
   m_runinfo.nprocs++;
   x_timers[Tprocd]->stop();		// for processed frames

   return E_OK;
}

// run control of child thread
//______________________________________________________________________
void SupixDAQ::noise_run()
{  TRACE;

   // select the pixel array to read
   select_chip_addr();
   
   // main loop
   alt_run_status(RUN_FIRST);
   unsigned long nloop = 0;
   int rv;
   do {
      nloop++;
      // LOG << nloop << " HEAD" << endl;

      rv = reader_run();
      if (rv > E_CONTINUE) {
	 alt_run_status(RUN_STOP);
      }
      else if (rv == E_OK) {	// good frame
	 m_runinfo.nprocs++;

	 if (m_pipeline->is_first())
	    m_frame_1st = true;
	 else if (m_frame_1st)
	    m_frame_1st = false;

	 decode_frame();		// after checking frame_1st
	 do_noise();
      }
      
   }
   while (! is_run_stop() );		//~main loop
   
   LOG << endl;
}

// run control of child thread
//______________________________________________________________________
void SupixDAQ::write_noise()
{  TRACE;

   // write out noises to a file
   string fnoise = m_pathbase;
   fnoise += ".txt";
   ofstream ofs(fnoise.c_str(), ofstream::trunc);
   // per row: cds_mean cds_sigma adc_mean adc_sigma ...
   
   double mean, sigma;
   RecurStats*	pcds_noise = m_cds_noise;
   double*	pcds_mean = (double*)(m_runinfo.cds_mean);
   double*	pcds_sigma = (double*)(m_runinfo.cds_sigma);
   RecurStats*	padc_noise = m_adc_noise;
   double*	padc_mean = (double*)(m_runinfo.adc_mean);
   double*	padc_sigma = (double*)(m_runinfo.adc_sigma);
   for (int ir=0; ir < NROWS; ir++) {
      for (int ic=0; ic < NCOLS; ic++) {
	 pcds_noise->get_results(mean, sigma);
	 *pcds_mean = mean;
	 *pcds_sigma = sigma;
	 ofs << " " << mean << " " << sigma;	// cds
	 padc_noise->get_results(mean, sigma);
	 *padc_mean = mean;
	 *padc_sigma = sigma;
	 ofs << " " << mean << " " << sigma;	// adc
	 // next
	 pcds_noise++;
	 pcds_mean++;
	 pcds_sigma++;
	 padc_noise++;
	 padc_mean++;
	 padc_sigma++;
      }
      ofs << endl;
      // LOG << ir << ": " << mean << " " << sigma << endl;
   }
   ofs.close();
   m_runinfo.Print_noise_cds();
   m_runinfo.Print_noise_adc();
   
   // normalized noise file name
   ostringstream oss;
   oss << "ln -fv " << fnoise << " " << noise_file();
   int rv = system(oss.str().c_str() );
   LOG << "system(\"" << oss.str() << "\") : " << rv << endl;

   // LOG << "noise file: " << fnoise << endl;
}

// normalized noise file name
//______________________________________________________________________
string SupixDAQ::noise_file()
{  TRACE;
   ostringstream oss;
   oss << m_datadir << "/noise_a" << m_runinfo.chip_addr << ".txt";
   return oss.str();
}

// run control of child thread
//______________________________________________________________________
void SupixDAQ::do_noise()
{  TRACE;

   RecurStats*	pcds_noise = m_cds_noise;
   cds_t *	pcds = m_pixel_cds;
   RecurStats*	padc_noise = m_adc_noise;
   adc_t *	padc = m_pixel_adc;
   for (int ir=0; ir < NROWS; ir++) {
      for (int ic=0; ic < NCOLS; ic++) {
	 pcds_noise->add(*pcds);
	 padc_noise->add(*padc);
	 // next
	 pcds_noise++;
	 pcds++;
	 padc_noise++;
	 padc++;
      }
   }
   
   m_wr_mode = O_NOISE;
   m_pipeline->next_out();		// after data processed
   // LOG
   //    << " " << m_pipeline->sprint()
   //    << " " << m_pre_adc->sprint("adc")
   //    << " " << m_pre_cds->sprint("cds")
   //    << endl;

}

// run control of child thread
//______________________________________________________________________
void SupixDAQ::child_run()
{  TRACE;
   // static bool reset_frame_1st = false;
   
   // write-out main loop
   unsigned long nloop = 0;
   int rv;
   while(1) {
      nloop++;
      DBG_RUN("loop-head");
      x_timers[Tchild_run]->start();

      rv = writer_run();
      if (rv == E_RUNSTOP) {
	 break;
      }
      
      x_timers[Tchild_run]->stop();
      DBG_RUN("loop-tail");
   }	//~ write-out main loop

   LOG << sprint("RETURN") << endl;

}


// interface of a child thread
//______________________________________________________________________
void SupixDAQ::new_thread()
{  TRACE;
   m_childs++;
   child_run();
   //sleep(1);
   m_childs--;
}


void SupixDAQ::stop_run()
{  TRACE;

   // wait for ending of child thread
   while(m_childs > 0) {
      usleep(m_timewait);
   }

   LOG << sprint("RETURN") << endl;
   
}


// check lock_file
// return: pid
//______________________________________________________________________
int SupixDAQ::lock_run()
{  TRACE;
   int rv = 0;
   string shcmd;

   // make sure only ONE process is running.
   shcmd = "test -f ";
   shcmd += m_lock_file;
   rv = system(shcmd.c_str() );
   if (rv == 0) {
      LOG << m_lock_file << " exists: "
	  << "Probably another DAQ process is running." << endl;
      exit(1);
   }

   // create a lock file.
   shcmd = "touch ";
   shcmd += m_lock_file;
   rv = system(shcmd.c_str() );
   if (rv != 0) {
      LOG << "Lock file " << m_lock_file << " can NOT be created!!!" << endl;
      exit(1);
   }

   // write pid into the lock file.
   pid_t pid = getpid();
   shcmd = "echo ";
   shcmd += to_string(pid);
   shcmd += " > ";
   shcmd += m_lock_file;
   rv = system(shcmd.c_str() );
   if (rv) {
      perror(m_lock_file);
      exit(1);
   }

   return pid;
}


// decode a frame and save in stack
//______________________________________________________________________
void SupixDAQ::decode_frame()
{  TRACE;
   // static char str[MAXLINE];

   pixel_t	data;
   ushort	adc, col, row, fid;
   pixel_t *	ptr = (pixel_t*)(m_pipeline->get_out_ptr());
   adc_t *	padc = m_pixel_adc;
   adc_t *	plast = m_pixel_last;
   cds_t *	pcds = m_pixel_cds;
   // make free buffer for a new frame
   m_pre_adc->push();
   m_pre_cds->push();
   if (m_verbosity >= V_DEBUG && ( m_pre_adc->get_depth() < 2 || m_frame_1st ))
      LOG << sprint("ERROR") << endl;

   for (ushort ir = 0; ir < NROWS; ir++) {
      for (ushort ic = 0; ic < NCOLS; ic++) {
	 //fpga_decode( *(ptr + ir*NROWS + ic) );
	 // fpga_decode( *ptr++ );		// decode data of a pixel

	 data = *ptr++;
	 fpga_decode(data, fid, row, col, adc);
   	 // sprintf(str, "%s: data=%X fid=%X row=%X col=%X adc=%X\n", __func__
	 // 	 , data, fid, row, col, adc);
	 // fputs(str, stderr);

	 // the same in a frame
	 if (ir == 0 && ic == 0) {
	    m_fid = fid;
	 }
	 
	 if (m_frame_1st)
	    *pcds	= 0;
	 else {
	    *pcds	= adc - *plast;
	 }
	 *padc	= adc;
	 padc++;
	 pcds++;
	 plast++;
      }
   }

   if (m_verbosity >= V_DEBUG)
      LOG  << "ptr=" << ptr << " " << sprint() << endl;
   
}

// return trigger result
//   0   : no trigger
//   > 0 : have triggers
//______________________________________________________________________
trig_t SupixDAQ::triged()
{  TRACE;
   x_timers[Ttriged]->start();
   m_trig = 0;

   if ( (m_frame+1) % m_runinfo.trig_period == 0) {
      m_trig |= TRIG_PERIOD ;
      m_runinfo.ntrigs_period++;
   }
   
   if (trig_cds() > 0) {
      m_trig |= TRIG_CDS ;
      m_runinfo.ntrigs_cds++;
   }

   if (m_trig)
      m_runinfo.ntrigs++;

   x_timers[Ttriged]->stop();
   return m_trig;
}

// return number of pixels exceeding CDS thresholds.
//______________________________________________________________________
int SupixDAQ::trig_cds()
{  TRACE;
   int npixs = 0;	// Npixels fired

   cds_t *	pcds = m_pixel_cds;
   double*	pthr = m_threshold;
   ostringstream oss;
   oss << " (row col cds thr):";
   for (int ir=0; ir < NROWS; ir++) {
      for (int ic=0; ic < NCOLS; ic++) {
	 //if (*pcds > *pthr) {		// positive pulse
	 if ( *pcds < *pthr ) {		// negative pulse
	    m_pixid[npixs] = (ir << NBITS_COL) + ic;
	    npixs++;
	    if (m_verbosity >= V_DEBUG)
	       oss << " " << npixs << "=(" << ir << " " << ic << " " << *pcds << " " << *pthr << ")";
	 }
	 // next
	 pcds++;
	 pthr++;
      }
   }

   m_npixs = npixs;		// updated ONLY when having fired pixels for waveform analysis
   if (npixs) {
      // run information
      if (m_runinfo.ntrigs % 1000 == 0) {
	 LOG << "#triged=" << m_runinfo.ntrigs + 1	// .ntirgs updated later
	     << " frame=" << m_frame
	     << " npixs=" << npixs
	     << ( m_verbosity >= V_DEBUG ?  oss.str() : "" )
	     << endl;
      }
   }
   
   return npixs;
}

// trigger and write out frames
//______________________________________________________________________
void SupixDAQ::do_trig()
{  TRACE;
   // OUT_MODE_t mode = O_T0P0;	// pipeline_t out action
   int nframe_wr = 0;
   trig_t trig = triged();	// do it explicitly
   if (m_runinfo.daq_mode == M_CONTINUOUS || trig > 0) {	// new trigger
      x_timers[Twrite]->start();		// for recorded frames

      // pre_trigs
      int pre_trigs = m_pipeline->get_pre();
      if (pre_trigs > 0) {
	 write_out(pre_trigs);
	 nframe_wr += pre_trigs;
      }
      
      // this trig
      write_out();
      nframe_wr++;

      if (m_pipeline->is_post()) {
	 m_wr_mode = O_T1P1;
      }
      else {
	 m_wr_mode = O_T1P0;
      }
      
      x_timers[Twrite]->stop(nframe_wr);		// for recorded frames
   }
   else {				// NO new trigger
      if (m_pipeline->is_post()) {	// ... having post_trigs
	 m_wr_mode = O_T0P1;
	 x_timers[Twrite]->start();		// for recorded frames
	 write_out();
	 nframe_wr++;
	 x_timers[Twrite]->stop();		// for recorded frames
      }
      else {				// ... no post_trigs
	 x_timers[Tskip]->start();		// for non-recorded frames
	 m_wr_mode = O_T0P0;
	 x_timers[Tskip]->stop();		// for non-recorded frames
      }
   }

   x_timers[Tnext_out]->start();		// for non-recorded frames
   m_runinfo.nrecords += nframe_wr;
   m_pipeline->next_out(m_wr_mode);		// after data processed
   x_timers[Tnext_out]->stop();		// for non-recorded frames

   // after pipeline updated
   // check filesize limits after a whole waveform write-out.
   if ((m_runinfo.daq_mode == M_CONTINUOUS || (m_wr_mode == O_T0P1 && ! m_pipeline->is_post() ) ) &&
       (m_filesize_raw > m_filesize_max || m_filesize_root > m_filesize_max)
       ) {
      new_outfiles();
   }

   if (m_verbosity >= V_DEBUG)
      LOG << "nframe_wr=" << nframe_wr
	  << " " << sprint()
	  << endl;

}


// writing out a frame at pipeline_t::_out
void SupixDAQ::write_out()
{  TRACE;

   // raw data
   if (m_write_raw) {
      unsigned char *buf = m_pipeline->get_out_ptr();
      x_timers[Twr_raw]->start();
      m_filesize_raw += write_all(m_fd_raw, buf, FRAMESIZE);
      x_timers[Twr_raw]->stop();
   }

   // root files
   if (m_write_root) {
      x_timers[Twr_root]->start();
      m_filesize_root += m_tree->Fill();
      x_timers[Twr_root]->stop();
   }

   if (m_verbosity >= V_DEBUG)
      LOG << "filesize: raw=" << m_filesize_raw << " root=" << m_filesize_root
	  << " " << sprint()
	  << endl;

}

// write out pre_trigs of frames
//   nframes = number of pre-trigs
void SupixDAQ::write_out(int nframes)
{  TRACE;

   // raw data
   if (m_write_raw) {
      for (int i=0; i < nframes; i++) {
	 unsigned char *buf = m_pipeline->get_pre_ptr(i);
	 x_timers[Twr_raw]->start();
	 m_filesize_raw += write_all(m_fd_raw, buf, FRAMESIZE);
	 x_timers[Twr_raw]->stop();
      }
   }

   // root files
   if (m_write_root) {
      // roll-back & restore:
      //   - m_trig
      //   - m_frame and m_fid will do it automatically
      trig_t	trig_save = m_trig;
      m_trig = TRIG_PRE;		// pseudo-trig as a mark
      m_frame -= nframes;
      
      // fid in pre-trigs should be consecutive by reader's check_integrity
      int nframes_x = nframes % FID_MAX;
      m_fid = m_fid >= nframes_x ? m_fid - nframes_x : FID_MAX + m_fid - nframes_x ;

      for (int i = 0; i < nframes; i++) {
	 m_pre_adc->bubble();
	 m_pre_cds->bubble();
	 x_timers[Twr_root]->start();
	 m_filesize_root += m_tree->Fill();
	 x_timers[Twr_root]->stop();
	 m_pre_adc->pop();
	 m_pre_cds->pop();
	 // next
	 m_frame++;
	 m_fid = (m_fid + 1) % FID_MAX;
      }
      m_trig = trig_save;
   }

   if (m_verbosity >= V_DEBUG)
      LOG << "nframes=" << nframes
	  << " filesize: raw=" << m_filesize_raw << " root=" << m_filesize_root
	  << " " << sprint()
	  << endl;
   
}


// return: -1 = NOT found
//______________________________________________________________________
int SupixDAQ::locate_last_pixel()
{  TRACE;
   const ushort pixel_addr_last = 0x40F;
   // row infromation changed: 1~64, 40F For full matix --LongLI 2020-05-28 

   // read a frame
   //read_fifo(m_buffer, 400);	// test
   int rv = read_fifo(m_buffer);
   if (is_run_stop() ) {
      return rv;
   }
   
   // search last pixel in the frame
   pixel_t *ptr = (pixel_t*)m_buffer;		// word = 4 bytes
   pixel_t data;
   ushort adc, col, row, fid;
   int npix = 0;
   while (npix < NPIXS) {
      data = *(ptr + npix);
      fpga_decode(data, fid, row, col, adc);
      ushort pixel_addr = (row << 4) + col;
      if (pixel_addr == pixel_addr_last) {
	 break;
      }
      // npix < NPIXS when found
      npix++;
   }

   int nbyte = -1;
   if (npix < NPIXS) {
      // read fifo to the beginning of next frame
      nbyte = sizeof(pixel_t) * (NPIXS - npix - 1) ;
      rv = read_fifo(m_buffer, nbyte);
   }

   LOG << "last_pix=" << npix << " data=" << hexout(data)
       << " forward " << nbyte
       << " return=" << rv << " bytes"
       << " " << sprint()
       << endl;
   return rv;
}

// check data integrity of a pixel by checking ids of (frame, row, col)
//   ir	: row id expected
//   ic	: col id expected
// ids encoded in data
//   frame : should be consecutive
//   row == ir - 1
//   col == ic
//______________________________________________________________________

int SupixDAQ::check_integrity()
{  TRACE;
   int rv = 0;
   const int wrong_col	  = 0x1;
   const int wrong_row    = 0x2;
   const int wrong_fsame  = 0x4;	// not the same frame id in a frame
   const int wrong_fcons  = 0x8;	// not consecutive frame id
   const int fid_max    = 16;		// max frame id

   static ushort fid_last = fid_max;
   ushort fid_now=0, ir, ic;
   pixel_t *ptr = (pixel_t*)(m_pipeline->get_in_ptr());
   pixel_t data;
   ushort adc, col, row, fid;
   for (ir = 0; ir < NROWS; ir++) {
      for (ic = 0; ic < NCOLS; ic++) {
	 // fpga_decode( *ptr++ );		// decode data of a pixel

	 data = *ptr++;
	 fpga_decode(data, fid, row, col, adc);

	 if (ir == 0 && ic == 0) {
	    fid_now = fid;	// 1st pixel
	 }
	 else {
	    if (fid != fid_now)	rv |= wrong_fsame;
	 }
	 if ( (ir + 1) != row)	rv |= wrong_row;
	 if (ic != col)		rv |= wrong_col;

	 if (fid_last < fid_max) {	// valid last frame id
	    ushort diff = fid > fid_last
	       ? fid - fid_last : fid_max + fid - fid_last ;
	    if (diff != 1)		rv |= wrong_fcons;
	 }

	 if (rv)	break;
      }
      if (rv)		break;
   }

   // update for a good frame
   if (rv == 0)		fid_last = fid_now;
   else {
      static char str[MAXLINE];
      sprintf(str, "ERROR: read=%#8X (fid=%X row=%X col=%X adc=%4X) expected=(%X %X %X) "
      	      , data, fid, row, col, adc, (fid_last+1)%fid_max, ir+1, ic);
      CERR << str << sprint() << endl;
      
      fid_last = fid_max;		// reset last frame id
   }
   
   return rv;
}

// decode FPGA word (4 bytes)
// DATA RACE -- not using member variables !!!
//______________________________________________________________________
void SupixDAQ::fpga_decode(pixel_t data, ushort &fid, ushort &row, ushort &col, ushort &adc)
{  //TRACE;
   // const pixel_t
   //    mask_adc	 = 0x0000FFFF,
   //    mask_col	 = 0x000F0000,
   //    mask_row	 = 0x07F00000,    //row information changed: 1~64  --LongLI 2020-05-28
   //    mask_frame = 0xF0000000
   //    ;
   // adc	= ( data & mask_adc );
   // col	= ( data & mask_col ) >> 16;
   // row	= ( data & mask_row ) >> 20;
   // fid	= ( data & mask_frame ) >> 28;

   adc = data & MASK_ADC;	data >>= NBITS_ADC;
   col = data & MASK_COL;	data >>= NBITS_COL;
   row = data & MASK_ROW;	data >>= NBITS_ROW;
   fid = data;
}

// // decode FPGA word (4 bytes)
// // ushort NOT work!???
// //______________________________________________________________________
// void SupixDAQ::fpga_decode(pixel_t data)
// {  //TRACE;
//    const pixel_t
//       mask_adc	 = 0x0000FFFF,
//       mask_col	 = 0x000F0000,
//       mask_row	 = 0x07F00000,    //row information changed: 1~64  --LongLI 2020-05-28
//       mask_frame = 0xF0000000
//       ;
//    m_pixel.adc	= ( data & mask_adc );
//    m_pixel.col	= ( data & mask_col ) >> 16;
//    m_pixel.row	= ( data & mask_row ) >> 20;
//    m_pixel.fid	= ( data & mask_frame ) >> 28;

// }

//======================================================================
//
// I/O
//
//======================================================================



// // open a new raw file.
// void SupixDAQ::new_raw()
// {  TRACE;
//    close_raw();
//    open_raw();
//    LOG << "last " << m_filesize_raw << " bytes"
//        << " new: " << m_rawfn
//        << endl;
//    m_filesize_raw = 0;
// }


int SupixDAQ::read_fifo(unsigned char *buf, int nbyte)
{  TRACE;
   //if (nbyte == 0) 	return 0;	// used for EOF
   if (nbyte == 0) 	nbyte = FRAMESIZE;
   x_timers[Trd_fifo]->start();
   int rv = read_all(m_fd_fifo, buf, nbyte);
   x_timers[Trd_fifo]->stop();
   return rv;
}

int SupixDAQ::test_fifo()
{  TRACE;
   int rv = read_fifo(m_buffer);
   LOG << rv << endl;
   
   return rv;
}


// build pathname base
// - after  m_runinfo.set_time_start();
//______________________________________________________________________
void SupixDAQ::build_pathbase()
{  TRACE;
   static string pathbase;	// for const char*
   ostringstream oss;
   char fmt[] = "%y%m%d_%H%M%S";
   char timetag[sizeof(fmt)];
   strftime(timetag, sizeof(fmt), fmt, localtime(&m_runinfo.time_start) );
   oss << m_datadir << "/" << m_datatag
       << "_a" << m_runinfo.chip_addr
       << "_" << timetag;
   pathbase = oss.str();
   m_pathbase = pathbase.c_str();
}


// open new files for writing out
// - close last opened files
//______________________________________________________________________
void SupixDAQ::new_outfiles()
{  TRACE;
   static unsigned nn = 0;
   static string fn_raw, fn_root;	// necessary for const char* after return
   ostringstream oss;

   // time tag
   // if (nn == 0) {
   //    oss << m_pathbase;
   // }
   // else {
   oss << m_pathbase << "_" << nn;
   // }

   LOG << nn << " " << oss.str()
       << " filesize_raw=" << m_filesize_raw
       << " filesize_root=" << m_filesize_root
       << endl;
   
   // raw data files
   if (m_write_raw) {
      if (m_fd_raw >= 0) {
	 close_raw();
      }
      
      fn_raw	= oss.str() + ".data";
      m_filename_raw	= fn_raw.c_str();
      m_fd_raw = open_fd(m_filename_raw);
      LOG << "[" << m_fd_raw << "]" << m_filename_raw << " opened" << endl;
      m_filesize_raw = 0;	// reset count
   }
   
   if (m_write_root) {	// ROOT files
      if (m_tfile) {
	 close_root();
      }

      fn_root	= oss.str() + ".root";
      m_filename_root	= fn_root.c_str();
      m_tfile = new TFile(m_filename_root, "NEW");
      open_tree();
      LOG << m_filename_root << " opened"
	  << ": tree=" << m_tree->GetName()
	  << endl;

      m_filesize_root = 0;	// reset count
   }
   
   nn++;

}

// //______________________________________________________________________
// int SupixDAQ::open_raw()
// {  TRACE;
//    m_rawfn = m_datadir + "/" + m_datatag + "_";
//    m_rawfn += time_tag();
//    m_rawfn += ".data";
//    m_fd_raw = open_fd(m_rawfn.c_str() );

//    LOG << "fd-" << m_fd_raw
//        << " " << m_rawfn << " ... opened for writing"
//        << endl;

//    return m_fd_raw;
// }

//______________________________________________________________________
int SupixDAQ::close_raw()
{  TRACE;
   int rv = close_fd(m_fd_raw, m_filename_raw);
   LOG << "[" << m_fd_raw << "]" << m_filename_raw << " closed"
       << " " << m_filesize_raw << " bytes"
       << endl;
   // int rv = close_fd(m_fd_raw, m_rawfn.c_str() );
   // LOG << "fd-" << m_fd_raw
   //     << " " << m_rawfn << " ... closed"
   //     << endl;
   return rv;
}


// // ROOT file and TTree.
// //______________________________________________________________________
// int SupixDAQ::open_root()
// {  TRACE;
//    m_rootfn = m_datadir + "/" + m_datatag + "_";
//    m_rootfn += time_tag(false);
//    m_rootfn += ".root";

//    // This file is now the current directory.
//    m_tfile = new TFile(m_rootfn.c_str(), "NEW");
//    LOG << m_rootfn << " ... opened: " << m_tfile << endl;
//    open_tree();
//    return 0;
// }


// close the current ROOT file.
//______________________________________________________________________
int SupixDAQ::close_root()
{  TRACE;
   // renew current file due to TTree::SetMaxTreeSize().
   m_tfile = gDirectory->GetFile();
   // LOG << m_tfile->GetName() << endl;

   int rv = 0;
   // m_tree->FlushBaskets();
   // m_tree->Write();
   // m_tree->Print();

   // save all objects in the file
   m_tfile->Write();
   m_tree->Print();
   m_tree->GetUserInfo()->First()->Print();
   m_tfile->ls();
   
   // save run info
   // - has to Clear() before deleting the tree.
   // - get it
   //   m_tree->GetUserInfo()->First()->Print();
   m_tree->GetUserInfo()->Clear();
   // m_runinfo.Write("runinfo");
   m_tfile->Close();	// directory emptied and all objects deleted

   LOG << m_tfile->GetName() << " closed" << endl;
   
   // m_tfile->ls();
   // LOG << "pre  m_file=" << m_tfile
   //    //<< " " << m_tfile->IsOpen()
   //     << " tree=" << m_tree << endl;
   
   delete m_tfile;
   
   // LOG << "post m_file=" << m_tfile
   //    //<< " " << m_tfile->GetName()
   //     << " tree=" << m_tree << endl;

   return rv;
}

// book tree
//______________________________________________________________________
int SupixDAQ::open_tree()
{  TRACE;
   m_tree = new TTree("supix", "test chip");

   // if the file size reaches TTree::GetMaxTreeSize(), the current
   // file is closed and a new file is created as filename_N.root.
   // TTree::SetMaxTreeSize(m_filesize_max);

   // per second
   //m_tree->SetAutoSave(GB);


   // [obsolete] keep the same order as defined in header file.
   // leafs in decreased order of size
   //
   // b = UChar_t	B = Char_t
   // s = UShort_t	S = Short_t
   // i = UInt_t	I = Int_t
   // l = ULong64_t
   //--------------------------
   
   // string leaflist;
   // leaflist = "pixel_adc[" + to_string(NROWS) + "][" + to_string(NCOLS) + "]/s";
   // m_tree->Branch("pixel_adc", m_pixel_adc, leaflist.c_str() );
   // leaflist = "pixel_cds[" + to_string(NROWS) + "][" + to_string(NCOLS) + "]/I";
   // m_tree->Branch("pixel_cds", m_pixel_cds, leaflist.c_str() );

   ostringstream oss;
   oss.str("");
   oss << "pixel_cds[" << NROWS << "][" << NCOLS << "]/I";
   m_tree->Branch("pixel_cds", m_pixel_cds, oss.str().c_str());	// CDS = frame_now - frame_prev
   oss.str("");
   oss << "pixel_adc[" << NROWS << "][" << NCOLS << "]/s";
   m_tree->Branch("pixel_adc", m_pixel_adc, oss.str().c_str());	// ADC of frame_now
   // m_tree->Branch("pixid", m_pixid, "pixid[npixs]/s" );	// NON-applicable for all possibility
   oss.str("");
   oss << "pixid[" << NPIXS << "]/s";
   m_tree->Branch("pixid", m_pixid, oss.str().c_str());
   m_tree->Branch("frame", &m_frame, "frame/l" );		// global frame id
   m_tree->Branch("npixs", &m_npixs, "npixs/s" );
   //m_tree->Branch("frame_1st", &m_frame_1st, "frame_1st/O" );	// first frame flag [removed]
   m_tree->Branch("trig", &m_trig, "trig/b" );			// trigger pattern
   m_tree->Branch("fid", &m_fid, "fid/B" );			// local frame id

   // LOG << "TTree::SetMaxTreeSize(" << m_filesize_max << ")"
   //     << " SetAutoSave(" << GB << ")"
   //     << endl;

   // attach RunInfo
   m_tree->GetUserInfo()->Add(&m_runinfo);

   LOG << "Tree " << m_tree->GetName()
       << " AutoSave=" << m_tree->GetAutoSave()
       << " AutoFlush=" << m_tree->GetAutoFlush()
       << " (< 0 in bytes, > 0 in entries)"
       << endl;
   
   return 0;
}


////////////////////////////////////////////////////////////////////////


// wait until timeout
bool SupixDAQ::wait_timeout(const char* msg)
{  TRACE;
   const  int maxtry = 10;
   static int ntry = 0;
   static int timeout = 0;		// accumulated wait time
   static int timewait = m_timewait;	// time per wait
   usleep(timewait);
   if (m_run_status == RUN_STOP) {
      LOG << sprint() << " RUN_STOP " << endl;
      return true;
   }
   timeout += timewait;
   if (timeout > m_timeout) {		// exceeding assumed time limie
      x_counts[TIMEOUT]++;
      if (++ntry < maxtry) {
	 timeout = 0;
	 // timewait *= 10;
	 if (m_timeout < 10000000)	// 10 sec
	    m_timeout *= 10;	// prolong timeout
	 LOG << sprint(msg)
	     << " [try-" << ntry << "]"
	     << " prolong waiting time to " << (int)(m_timeout/1e6) << " sec"
	     << endl;
      }
      else
	 return true;
   }
   return false;
}

// get time tag for file name.
//______________________________________________________________________
std::string SupixDAQ::time_tag(bool count)
{  TRACE;
   static unsigned nn = 0;
   time_t rawtime;
   static time_t lasttime = 0;
   struct tm *timeinfo;
   char fmt[] = "%y%m%d_%H%M%S";
   int size = sizeof(fmt);
   char buffer[size];
   string tag;

   time (&rawtime);
   timeinfo = localtime(&rawtime);

   strftime(buffer, size, fmt, timeinfo);
   tag = buffer;

   // add "_N" to filename if necessary.
   if (count && rawtime == lasttime) {
      nn++;
      tag += "_" + to_string(nn);
   }
   else {
      nn = 0;
      lasttime = rawtime;
   }

#ifdef DEBUG
   LOG << rawtime << " " << tag << endl;
#endif

   return tag;
}

// select a sub-matrix on the chip
//______________________________________________________________________
int SupixDAQ::select_chip_addr()
{  TRACE;

   // encode command to send
   unsigned char cmd = 0xF8 | (m_runinfo.chip_addr >>1);
   // 11111xxx 11111 is default, xxx chip selset from 000 to 100 --LongLI
   
   int rv = write_mem(0,cmd); //address is 0 by default
   LOG << "write_mem(" << hexout((int)cmd) << "): " << rv
       << endl;
   return rv;
}

// write Xillybus dual port memory.
// 1 byte
// need NOT address
int SupixDAQ::write_mem(unsigned char data)
{  TRACE;

   int rv = write(m_fd_mem, &data, 1);

   LOG << "return = " << rv << ": data " << hexout((unsigned)data) << endl;

   // if ((rv < 0) && (errno == EINTR))
   //    continue;

   if (rv < 0) {
      perror("write_mem() failed to write");
      exit(1);
   }

   if (rv == 0) {
      fprintf(stderr, "Reached write EOF (?!)\n");
      exit(1);
   }

   return rv;
}

/* write_mem.c -- Demonstrate write to a Xillybus dual port memory

This simple command-line application is given three arguments: The
device file to write to, the address (a decimal number between 0 and 31)
and data to be written (a decimal number between 0 and 255).

The application sends a seek command to the device file and writes the
character. These two operations create a write operation on the given address
on the FPGA's dual port RAM.

The use of write_all() is an overkill when a single byte is written. For
longer writes it's the adequate way to do what most programmers expect:
Write all data, or never return.

See http://www.xillybus.com/doc/ for usage examples an information.

input:
  address	: a decimal number in [0, 31]
  data		: 1 byte, a decimal number in [0, 255]
*/
int SupixDAQ::write_mem(int address, unsigned char data)
{  TRACE;
  if (lseek(m_fd_mem, address, SEEK_SET) < 0) {
    perror("Failed to seek");
    exit(1);
  }

  int rv = write_all(m_fd_mem, &data, 1);
  if (rv != 1) {
     LOG << "something WRONG!" << endl;
  }

  return rv;
}

// CDS noise and threshold for each pixel.
// - saved in RunInfo
//______________________________________________________________________
void SupixDAQ::set_trig_cds()
{  TRACE;

   double cds_mean, cds_sigma;
   double adc_mean, adc_sigma;
   double* pthrs	= m_threshold;
   double* pcds_mean	= (double*)(m_runinfo.cds_mean);
   double* pcds_sigma	= (double*)(m_runinfo.cds_sigma);
   double* padc_mean	= (double*)(m_runinfo.adc_mean);
   double* padc_sigma	= (double*)(m_runinfo.adc_sigma);
   string fnoise	= noise_file();
   ifstream ifs(fnoise.c_str() );
   if (! ifs.is_open() ) {
      CERR << "FATAL opening file: " << fnoise << endl;
      exit(-1);
   }
   else {
      LOG << "noise file opened: " << fnoise << endl;
   }
   
   for (int ir=0; ir < NROWS; ir++) {
      for (int ic=0; ic < NCOLS; ic++) {
	 ifs
	    >> cds_mean >> cds_sigma
	    >> adc_mean >> adc_sigma
	    ;
	 *pcds_mean = cds_mean;
	 *pcds_sigma = cds_sigma;
	 *padc_mean = adc_mean;
	 *padc_sigma = adc_sigma;
	 //*pthrs = cds_mean + cds_sigma * m_runinfo.trig_cds_x;	// positive pulse
	 //*pthrs = (cds_t)(cds_mean - cds_sigma * m_runinfo.trig_cds_x -0.5);		// negative pulse
	 *pthrs = cds_mean - cds_sigma * m_runinfo.trig_cds_x;		// negative pulse
	 // next
	 pcds_mean++;
	 pcds_sigma++;
	 padc_mean++;
	 padc_sigma++;
	 pthrs++;
	 //cout << setw(6) << setprecision(1) << fixed << cds_mean << "/" << cds_sigma;
      }
      //cout << endl;
   }
   ifs.close();
   m_runinfo.Print_threshold();
   
   //[obsolete]
   // // map of noise files
   // static std::map<int, const char*> x_noise_files
   //    //=c++11
   //    // {
   //    //  {0, "PixelNoise_A0.txt"} ,
   //    //  {2, "PixelNoise_A2.txt"} ,
   //    //  {5, "PixelNoise_A5.txt"} ,
   //    //  {7, "PixelNoise_A7.txt"} ,
   //    //  {8, "PixelNoise_A8.txt"}
   //    // }
   //    ;
   // if (x_noise_files.size() == 0) {
   //    x_noise_files[0] = "PixelNoise_A0.txt";
   //    x_noise_files[2] = "PixelNoise_A2.txt";
   //    x_noise_files[5] = "PixelNoise_A5.txt";
   //    x_noise_files[7] = "PixelNoise_A7.txt";
   //    x_noise_files[8] = "PixelNoise_A8.txt";
   // }
   
   // const char *noise_file = x_noise_files.find(m_runinfo.chip_addr)->second;
   // ifstream fin;
   // fin.open(noise_file);
   // if(!fin.is_open()){
   //    std::cout<<"Failed to open the file " << noise_file << " !!!" <<std::endl;
   //    exit(-1);
   // }

   // float noise;
   // for (int ir=0; ir < NROWS; ir++) {
   //    for (int ic=0; ic < NCOLS; ic++) {
   // 	 fin >> noise;
   // 	 //m_runinfo.trig_cds[ir][ic] = (int)(m_runinfo.trig_cds_x * noise + 0.5);
   // 	 m_runinfo.trig_cds[ir][ic] = m_runinfo.trig_cds_x * noise;
   //    }
   // }
   // fin.close();

   // LOG << noise_file << endl;
}

void SupixDAQ::recommend()		// recommend daq configuration
{

   // pipeline_max
   double t0	= x_timers[Tread]->get_mean();		// read-in
   double t1	= x_timers[Tprocd]->get_mean();		// process
   double twrite= x_timers[Twrite]->get_mean();		// write-out
   double tskip	= x_timers[Tskip]->get_mean();		// skip
   double tcomm	= x_timers[Tdecode_frame]->get_mean()	// common in process
      + x_timers[Tis_first]->get_mean()
      + x_timers[Ttriged]->get_mean()
      + x_timers[Tnext_out]->get_mean()
      ;

   // rough method
   int N0 = 1e6 / t0;		// 31250 - fifo frames per sec
   int pipeline_sec = calc_pipeline(N0, t0, t1);
   int pipeline_this = calc_pipeline(m_runinfo.nsaved, t0, t1);

   double Vt0	= x_timers[Tread]->get_variance();		// read-in
   double Vt1	= x_timers[Tprocd]->get_variance();		// process
   double Vtwrite= x_timers[Twrite]->get_variance();		// write-out
   double Vtskip	= x_timers[Tskip]->get_variance();		// skip
   double Vtcomm	= x_timers[Tdecode_frame]->get_variance()	// common in process
      + x_timers[Tis_first]->get_variance()
      + x_timers[Ttriged]->get_variance()
      + x_timers[Tnext_out]->get_variance()
      ;

   // precise method
   //
   // based on: T0 > eps*T1 + (1-eps)*T2 = Tprocd
   // have:	eps < (T0 - T2) / (T1 - T2)
   // then:	trig_ratio = eps / Nwave
   //		trig_freq = 1 / trig_ratio	-- frames read per trig
   // consider:	errors propagation
   // pipeline:	at lease Nwave * (trig_freq + sigma)
   //
   // where:
   //   alpha	- T1/T2 (measured)
   //   eps	- write-out ratio = N1 / N0 (measured)
   //   T0 (N0)	- read-in time per frame, N0 = N1 + N2
   //   T1 (N1)	- write-out time per frame
   //   T2 (N2)	- skipped frame
   //   Nwave	- frames per waveform
   // use measured alpha, eps and Tprocd to estimate T1 and T2.
   // 
   //-------------------------------------------------------------------
   // double alpha = (twrite + tcomm) / (tskip + tcomm);		// >1
   double alpha, Valpha;
   variance_ratio3(tcomm, Vtcomm, twrite, Vtwrite, tskip, Vtskip, alpha, Valpha);
   
   double eps = (double)m_runinfo.nrecords / m_runinfo.nsaved;	// <1
   double Veps = eps*eps * (1./m_runinfo.nrecords + 1./m_runinfo.nsaved);

   double t1skip, Vt1skip;
   variance_xea(t1, Vt1, eps, Veps, alpha, Valpha, t1skip, Vt1skip);
   //double t1skip = t1 / (1 + eps * (alpha - 1));
   
   double t1write = t1skip * alpha;
   double Vt1write = t1write*t1write * (Vt1skip/(t1skip*t1skip) + Valpha/(alpha*alpha));

   double eps_max, Veps_max;
   variance_ratio3(t1write, Vt1write, t0, Vt0, -t1skip, Vt1skip, eps_max, Veps_max);
   //double epsmax = calc_epsilon(t0, t1write, t1skip);

   // trigger ratio
   int Nwave = m_runinfo.pre_trigs + m_runinfo.post_trigs + 1;
   double trig_ratio_max	= eps_max / Nwave;
   double Vtrig_ratio_max	= Veps_max / (Nwave*Nwave);
   
   double trig_freq_min		= Nwave / eps_max;
   double Vtrig_freq_min	= Nwave*Nwave*Veps_max / pow(eps_max, 4);

   // base on trig_freq_min + 1-sigma
   int pipeline_max1 = (int) (Nwave * (trig_freq_min) + 0.5);
   int pipeline_max2 = (int) (Nwave * (trig_freq_min + sqrt(Vtrig_freq_min)) + 0.5);

   LOG
      << "pipeline_max: > " << pipeline_max1 << " -- " << pipeline_max2
      << ", rough: " << pipeline_this << " (this run), " << pipeline_sec << " (1 sec run)"
      << endl << "\t"
      << "epsilon < " << per_centage(eps_max) << "(" << per_centage(sqrt(Veps_max)) << ")%"
      << ", trig_ratio < " << per_centage(trig_ratio_max) << "(" << per_centage(sqrt(Vtrig_ratio_max)) << ")%"
      << ", trig_freq > " << (int)(trig_freq_min + 0.5) << "(" << (int)(sqrt(Vtrig_freq_min)+0.5) << ")"
      << endl << "\t"
      // << " t0=" << t0 << " t1=" << t1
      << "Twrite=" << t1write << "(" << sqrt(Vt1write) << ")"
      << " Tskip=" << t1skip << "(" << sqrt(Vt1skip) << ")"
      << " Tcomm=" << tcomm << "(" << sqrt(Vtcomm) << ")"
      << " [usec]"
      << " eps=" << per_centage(eps) << "(" << per_centage(sqrt(Veps)) << ")%"
      << " alpha=Twrite/Tskip=" << alpha << "(" << sqrt(Valpha) << ")"
      << endl;

}


void SupixDAQ::print(const char *msg)
{  TRACE;
   COUT << sprint(msg)
	<< " nonintegrity-ratio=" << (double)x_counts[NONINTEGRITY] / m_runinfo.nreads
	<< "\n\t[config]"
	<< " mem=" << m_fd_mem << "(" << m_dev_mem << ")"
	<< " fifo=" << m_fd_fifo << "(" << m_dev_fifo << ")"
	<< " lock=" << ( m_mode_debug ? "NO" : m_lock_file )
	<< " data dir=" << m_datadir << " tag=" << m_datatag
	<< " pipeline_max=" << m_pipeline_max
	<< " timewait=" << m_timewait << "usec"
	<< " timeout=" << (float)m_timeout/1e6 << "sec"
	<< " maxframe=" << m_maxframe
	<< " filesize_max=" << m_filesize_max << "bytes"
	<< endl;
   m_pipeline->print();
   m_pre_adc->print("adc");
   m_pre_cds->print("cds");
   if (! m_write_root)
      m_runinfo.Print();

}

// run status
//const char*	// this is problematic
std::string
SupixDAQ::sprint(const char* msg)
{
   // if (m_runinfo.daq_mode == M_NOISE)
   //    return "NOISE RUN";
   
   ostringstream oss;
   // key information
   oss << msg << ": "
       << "#R/P/W=" << m_runinfo.nsaved
       << "/" << m_runinfo.nprocs
       << "/" << m_runinfo.nrecords
       << " run_status=" << m_run_status
       << " wr_mode=" << m_wr_mode
       << " 1st=" << m_frame_1st
       << " trig=" << (ushort)m_trig
      ;
   
   // detail counters
   if (m_verbosity >= V_DEBUG || m_run_status == RUN_STOP) {
      for (int i=0; i < NCOUNTS; i++) {
	 oss << " " << x_counts_name[i] << "=" << x_counts[i];
      }
   }

   if (m_verbosity >= V_DEBUG) {
      oss
	 << " " << m_pipeline->sprint()
	 << " " << m_pre_adc->sprint("adc")
	 << " " << m_pre_cds->sprint("cds")
	 ;
   }

   return oss.str();
}
