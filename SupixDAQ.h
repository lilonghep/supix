/*******************************************************************//**
 * $Id: SupixDAQ.h 1219 2020-07-15 08:56:47Z mwang $
 *
 * supix DAQ with thread support
 *
 * - size always in byte
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-22 12:39:19
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#ifndef SupixDAQ_h
#define SupixDAQ_h

#include "mydefs.h"
#include "pipeline.h"	// pipeline_t
#include "util.h"
#include "Timer.h"	// RecurStats, Timer
#include "RunInfo.h"

#include "TTree.h"

#include <sys/types.h>	// ushort

// #include <string>
// #include <fstream>
// forward declaration
class TFile;

enum run_status_t { RUN_FIRST, RUN_START, RUN_STOP };

//
// class declaration
//======================================================================
class SupixDAQ {
   // // a pixel data
   // struct {
   //    ushort adc, col, row, fid;
   // } m_pixel ;

public:

   // constructor(s)
   SupixDAQ();

   // destructor
   ~SupixDAQ();

   //
   // configuration
   //   -1 to set default in general
   //-------------------------------------------------------------------
   void set_daq_mode(DAQ_MODE_t x)	{ m_runinfo.daq_mode = x; }
   void set_chip_addr(int x=-1)		{ m_runinfo.chip_addr = x<0 ? 0 : x % NMATRIX ; }
   void set_pre_trigs(int x=-1)		{ m_runinfo.pre_trigs = x<0 ? 3 : x ; }
   void set_post_trigs(int x=-1)	{ m_runinfo.post_trigs = x<0 ? 6 : x ; }
   void set_trig_period(int x=-1)	{ m_runinfo.trig_period = x<=0 ? 31250 : x ; }
   void set_trig_cds_x(double x)	{ m_runinfo.trig_cds_x = x; }
   void set_trig_cds();		// CDS thresholds of each pixel
   void set_pipeline_max(int x)		{ m_pipeline_max = x; }
   void set_maxframe(unsigned long x)	{ m_maxframe = x; }
   void set_write_raw(bool x)		{ m_write_raw = x; }
   void set_write_root(bool x)		{ m_write_root = x; }
   void set_filesize_max(long x)	{ m_filesize_max = x; }
   void set_timewait(int x)		{ m_timewait = x; }
   void set_timeout(int x)		{ m_timeout = x * 1e6 / m_timewait; }	// sec -> usec
   void set_verbosity(int x)		{ m_verbosity = x; }
   // void set_noise_run()			{ m_noise_run = true; }

   void set_mode_debug();	// fake fifo & mem
   void set_filename(const char *dir, const char *tag);

   //
   // control & actions
   //-------------------------------------------------------------------

   // exit for any error.
   void initialize();
   void finalize();

   // run control
   void start_run();
   void stop_run();
   int  lock_run();

   int reader_run();
   int writer_run();
   void noise_run();
   void do_noise();
   void write_noise();
   std::string noise_file();

   RecurStats * m_cds_noise;
   RecurStats * m_adc_noise;
   
   // void alt_run_stop()		// alter run status to RUN_STOP
   // { m_run_status = RUN_STOP; }

   void alt_run_status(run_status_t x)		// alter run status
   { m_run_status = x; }

   // check stop conditions
   bool is_run_stop()
   { return m_run_status == RUN_STOP; }

   int select_chip_addr();

   // need NOT address
   int write_mem(unsigned char data);
   int write_mem(int address, unsigned char data);
   
   // search last pixel in a frame
   // and read fifo to the beginning of next frame
   // return:
   //   -1	= NOT found
   //   >= 0	= found
   int locate_last_pixel();

   // check data integrity
   int check_integrity();

   //
   // child thread
   //-------------------------------------------------------------------
   void new_thread();		// interface for a new thread
   void child_run();		// 
   void decode_frame();
   void do_trig();
   trig_t triged();
   int trig_cds();		// CDS trigger

   //
   // I/O
   //-------------------------------------------------------------------

   // read a frame, default = a whole frame
   int read_fifo(unsigned char *buf, int nbyte=FRAMESIZE);

   // interface for writing out data in frames
   void write_out();			// write out the frame at pipeline_t::_out
   void write_out(int nframes);		// write out pre_trigs of frames
   // int write_out(unsigned char *buf, int nframes=1, bool pre_trigs=false);

   
   // time tag for filename
   std::string time_tag(bool count=true);
   void build_pathbase();
   void new_outfiles();
   const char* m_pathbase;	// as "/path/to/basename"
   
   // raw data files
   // int open_raw();
   int close_raw();
   // void new_raw();

   // write raw date into ROOT files
   // int open_root();
   int close_root();
   int open_tree();

   //
   // utilities
   //-------------------------------------------------------------------
   
   // decode FPGA word (4 bytes)
   // void fpga_decode(pixel_t data);
   void fpga_decode(pixel_t data, ushort &fid, ushort &row, ushort &col, ushort &adc);

   // wait until timeout
   bool wait_timeout(const char* msg);

   // unit tests
   int test_fifo();

   // rough method to estimate pipeline capacity
   //   n0 : expected read-in frames
   //   t0 : time for saving a frame
   //   t1 : time for processing a frame
   // return	: pipeline capacity
   int calc_pipeline(int n0, double t0, double t1)
   {
      if (t0 >= t1)	return -1;
      return (t1 - t0) / t1 * n0;
   }

   // [obsolete] use util::variance_ratio3() instead
   //
   //   t0  : time of saving a frame
   //   t1 : time of recording a frame (Nwrite), t1 > t2
   //   t2  : time of non-recording a frame (Nskip)
   // return:
   //	eps    = Nwrite / (Nwrite+Nskip) = (t0 - t2) / (t1 - t2)
   double calc_epsilon(double t0, double t1, double t2)
   {
      if (t1 <= t2) return -1;
      return (t0 - t2) / (t1 - t2);
   }
   
   void recommend();		// recommend daq configuration
   
   
   std::string sprint(const char* msg="");		// run status
   void print(const char *msg="");		// + configuration
   
   /////////////////////////////////////////////////////////////////////

private:
   int		m_verbosity;	// verbosity for cout
   int		m_pid;		// process id
   RunInfo	m_runinfo;	// attached to GetUserInfo()
   double*	m_threshold;	// fast access to runinfo.trig_cds[][]

   // bool		m_noise_run;	// noise run mode
   // bool		m_consecutive;	// flag for consecutive writing
   // DAQ_MODE_t	m_runinfo.daq_mode;	// DAQ mode
   
   //
   // I/O
   //-------------------------------------------------------------------
   const char *	m_lock_file;
   char *	m_dev_mem;
   char *	m_dev_fifo;

   // file descriptors
   int	m_fd_mem;
   int	m_fd_fifo;
   int	m_fd_raw;		// raw data file
   bool	m_write_raw;
   bool	m_write_root;
   long	m_filesize_max;		// max bytes/file
   long	m_filesize_raw;		// cumulative bytes in raw file
   long	m_filesize_root;	// cumulative bytes in root file
   
   std::string		m_datadir;	// data dir
   std::string		m_datatag;	// data file name tag
   std::string		m_rawfn;	// data file name

   const char *	m_filename_raw;
   const char *	m_filename_root;
   
   // ROOT files
   std::string	 m_rootfn;
   TFile *		m_tfile;
   TTree *		m_tree;

   // data saved on Tree
   // - use ROOT data type for Tree branches
   // - un-aligned branches at the end
   adc_t *	m_pixel_adc;	// point to TOP of m_pre_adc
   cds_t *	m_pixel_cds;	// point to TOP of m_pre_cds
   ULong_t	m_frame;	// global frame id, 0-based
   trig_t	m_trig;		// trigger pattern
   fid_t	m_fid;		// local frame id encoded in pixel data
   UShort_t	m_npixs;	// #pixels fired
   UShort_t	m_pixid[NPIXS];	// fired pixel ids: row=0x03F0, col=0x000F
   
   // NOT on Tree
   Bool_t	m_frame_1st;	// default be first frame

   // for decoded (pre_trigs+1) of frames
   ostack_t *	m_pre_adc;
   ostack_t *	m_pre_cds;
   adc_t *	m_pixel_last;

   // DAQ configuration
   bool		m_mode_debug;	// debug mode
   int		m_timewait;		// usec
   int		m_timeout;		// usec
   unsigned long m_maxframe;		// 0 = infinity
   
   // cyclic pipeline
   //   constructor: pipeline_t(framesize, maxframes)
   pipeline_t *	m_pipeline;
   int		m_pipeline_max;		// total frames
   OUT_MODE_t	m_wr_mode;			// pipeline write-out mode
   
   //int		m_pipeline_sec;		// estimated max for 1 sec

   unsigned char *m_buffer;	// temporary for a frame

   // run control
   int	m_childs;	// number of child threads
   // bool	m_locate_last_pixel;	// search fifo for last pixel position
   // bool	m_done_wr;		// whether all saved frames written out
   
   //
   // signal handler
   // - has to be static
   //___________________________________________________________________
public:
   static void sig_handler(int sig);

private:
   // static members for sig_handler
   static volatile run_status_t m_run_status;
   static int signals[3];		// signals to register


   // for cross-platform compatibility
   //======================================================================
#ifdef __linux__
public:
   inline std::string to_string(int x)
   {
      char str[sizeof(int)*8 + 1];
      sprintf(str, "%d", x);
      return str;
   }

#endif

};

// debug mode
//   using a fifo to fake /dev/xillybus_read_32.
//   create a fifo
//     mkfifo ./data/myfifo
inline void SupixDAQ::set_mode_debug()
{
   m_mode_debug	= true;
   m_dev_mem	= (char*)"./data/mymem";
   m_dev_fifo	= (char*)"./data/myfifo";
}

// dir and filename tag for write-out
inline void SupixDAQ::set_filename(const char *dir, const char *tag)
{
   m_datadir = dir;
   m_datatag = tag;
}

#endif //~ SupixDAQ_h
