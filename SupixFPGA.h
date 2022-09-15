/******************************************************************//**
 * $Id: SupixFPGA.h 1145 2020-06-29 02:56:16Z mwang $
 *
 * @class:      SupixFPGA declaration
 *
 * Description: for ROOT programming.
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2016-05-02 14:35:48
 * @copyright:  (c)2016 HEPG - Shandong University. All Rights Reserved.
 *
 ***********************************************************************/
#ifndef SupixFPGA_h
#define SupixFPGA_h

#include "mydefs.h"

// backward compatible
#define FRAMESIZE	NROWS * NCOLS	// pixels per frame
#define FRAMEBYTES	4 * FRAMESIZE	// bytes of a frame (4096)
#define FRAMEROTIME	32		// 32 us/frame
#define FRAMES_SEC	1000000 / FRAMEROTIME	// frames per second
#define BUFSIZE	FRAMEBYTES		// bytes of buffer

#include "RunInfo.h"
#include "TTree.h"

#include <sys/types.h>	// ushort

#include <string>
#include <fstream>
// forward declaration
class TFile;

// class declaration
//======================================================================
class SupixFPGA {
public:
   enum run_status_t { RUN_START, RUN_STOP };
   enum trig_types {
      TRIG_PERIOD	= 0x01,
      TRIG_CDS		= 0x02,
      TRIG_TYPES	= 2		// N types of trigger
   };

   // constructor(s)
   SupixFPGA();

   // destructor
   ~SupixFPGA();

   // exit for any error.
   void initialize();

   void finalize();

   // open a file/dev
   int open_fd(const char *path, int oflag);

   // close an open fd
   int close_fd(int fd, const char *msg);

   void set_mode_debug();
   

   void set_write_root(bool x)
   { m_write_root = x; }

   void set_write_raw(bool x)
   { m_write_raw = x; }

   void set_file_maxbytes(unsigned x, unsigned bytes=1)
   { m_file_maxbytes = x * bytes; }

   void set_filename(const std::string dir, const std::string tag)
   {
      m_datadir = dir;
      m_datatag = tag;
   }


   // run configuration
   //   -1 to set default in general
   void set_read_frames(unsigned long x=0)	{ m_read_frames = x ; }
   void set_chip_addr(int x=-1)		{ m_runinfo.chip_addr = x<0 ? 0 : x % NMATRIX ; }
   void set_pre_trigs(int x=-1)		{ m_runinfo.pre_trigs = x<0 ? 10 : x ; }
   void set_post_trigs(int x=-1)	{ m_runinfo.post_trigs = x<0 ? 100 : x ; }
   void set_trig_period(int x=-1)	{ m_runinfo.trig_period = x<=0 ? 31250 : x ; }
   void set_trig_cds(int x=0x10000, int chip_addr = 11); //{ m_runinfo.trig_cds = x> 0xFFFF ? 10 : x ;}

   void set_print_freq(int x=-1)	{ m_print_freq = x<0 ? FRAMES_SEC : x ; }

   // perform triggers
   // return: trigger modes
   int do_trigger();

   // CDS trigger
   int trig_cds();

   // check frame data integrity
   int check_integrity(int ir, int ic, UShort_t frame, UShort_t row, UShort_t col);

   // record pre-triggers
   int record_pre_trigs();

   // record an frame
   int record_frame(int ith);

   int select_chip_addr();

   int read_all(int fd, unsigned char *buf, int nbytes);

   bool locate_last_pixel();

   int read_fifo(int nbytes=0);

   // need NOT address
   int write_mem(unsigned char data);

   int write_mem(int address, unsigned char data);

   // write raw date into files
   int write_all(int fd, unsigned char *buf, int nbytes);
   int write_all(unsigned char *buf, int nbytes)
   { return write_all(m_fd_out, buf, nbytes); }

   // write i-th frame in pipeline into .data file
   int write_raw(int ith);

   // raw data files
   int open_raw();
   int close_raw();

   // write raw date into ROOT files
   int open_root();
   int close_root();
   int open_tree();

   // fill tree by decoding data in buffer
   int fill_tree(unsigned char *buf, int nbytes);

   // fill tree with i-th frame in pipeline
   int fill_tree(int ith);

   // utilities
   //----------

   // time tag for filename
   std::string time_tag(bool count=true);

   // decode FPGA word (4 bytes)
   // ushort NOT work!???
   void fpga_decode(UInt_t data, UShort_t &adc, UShort_t &col, UShort_t &row, UShort_t &frame);

   void print_buffer(unsigned char* buf,int nbytes);

   int AddrSelect(int fd,unsigned char* buf,int nbytes);

   // run control
   int start_run();
   int stop_run();
   int lock_run();

   // signal handler
   static void sig_handler(int sig);

//   void check_adcinfo(int ir, int ic, UShort_t col, UShort_t row);
//   int check_frame(int ir, int ic, UShort_t frame);
//   void frame_circle(UShort_t frame);
private:
   // static members
   static volatile int m_run_status;
   static int signals[2];		// signals to register

   // const members
   const char		*m_lock_file;
   const char		*m_xillybus_mem;
   const char		*m_xillybus_fifo;

   // // move the following into RunInfo
   // int	m_chip_addr;	// matrix on chip
   // int	m_trig_period;	// trigger per N frames
   // int	m_trig_cds;	// CDS trigger threshold
   // int	m_pre_trigs;	// nframes to record pre-trigger, NOT including the triggered
   // int	m_post_trigs;	// nframes to record post-trigger, NOT including the triggered

   int	m_print_freq;		// frequency to print

   // a cyclic pipeline as data buffer
   struct m_pipeline {
      int	size;			// in frames, m_pre_triggers + 1
      int	bytes;			// size in bytes
      int	saved;			// saved frames in pipeline
      int	post_trigs;
      int	inow;			// index now
      unsigned char	*buffer;	// head of pipeline
      Int_t	*pixel_cds;
      UShort_t	*pixel_adc;
      std::string sprint();
   }
      m_pipeline;

   unsigned char *m_buffer;		// for read data, pointing to pipeline
   // switches
   bool	m_write_raw;
   bool	m_write_root;
   bool m_mode_debug;

   std::string		m_dev_fifo;
   std::string		m_dev_mem;

   unsigned long	m_read_frames;	// bytes to read, 0 = infinite

   // file descriptors
   int	m_fd_mem;	// Xillybus dual port memory
   int	m_fd_fifo;	// Xillybus FIFO 32-bits
   int	m_fd_out;
   bool m_locate_last_pixel;


   unsigned long	m_file_maxbytes;	// max bytes/file
   std::string		m_datadir;	// data dir
   std::string		m_datatag;	// data file name tag
   std::string		m_rawfn;	// data file name

   // ROOT files
   std::string	 m_rootfn;
   TFile        *m_tfile;
   TTree        *m_tree;

   // data saved in Tree
   // - use ROOT data type for Tree branches
   Int_t	m_pixel_cds[NROWS][NCOLS];
   UShort_t	m_pixel_adc[NROWS][NCOLS];
   // un-aligned branches at the end
   ULong_t	m_frame;	// frame id, starting 0
   UChar_t	m_trig;		// trigger pattern
   Bool_t	m_frame_1st;	// whether first frame
   RunInfo	m_runinfo;	// attached to GetUserInfo()

   // NOT on tree
   UShort_t	m_pixel_last[NROWS][NCOLS];

   // lilong
   UShort_t	m_frame_now;
   UShort_t	m_frame_last;
   UShort_t	m_frame_cir_now;
   UShort_t	m_frame_cir_last;
};


// for cross-platform compatibility
//======================================================================

#ifdef __linux__

inline std::string to_string(int x)
{
   char str[sizeof(int)*8 + 1];
   sprintf(str, "%d", x);
   return str;
}

#endif

#endif //~ SupixFPGA_h
