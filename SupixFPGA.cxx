/*******************************************************************//**
 * $Id: SupixFPGA.cxx 1128 2020-06-21 04:02:43Z mwang $
 *
 * @class:      SupixFPGA implementation
 *
 * Description: for ROOT programming.
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2016-05-02 14:35:48
 * @copyright:  (c)2016 HEPG - Shandong University. All Rights Reserved.
 *
 ***********************************************************************/
#include "SupixFPGA.h"

// #include <stdlib.h>

// ROOT headers
#include "TFile.h"

// C headers
//
#include <unistd.h>	// write(), open(), read(), ... getpid()
#include <sys/types.h>	// getpid()
#include <fcntl.h>	// O_WRONLY
#include <signal.h>	// signal
#include <errno.h>	// errno
#include <stdio.h>	// perror
#include <sys/stat.h>	// mode_t
#include <time.h>

// C++ headers
#include <sstream>
#include <iostream>
#include <iomanip>
using namespace std;


// static members = global
//======================================================================

volatile int SupixFPGA::m_run_status = 0;
int SupixFPGA::signals[] = { SIGINT, SIGTERM };

// keep the handler as simple as possible.
// NOT put i/o in it!!
void SupixFPGA::sig_handler(int sig)
{
   (void)(sig);	// avoid unused parameter warning
   m_run_status = RUN_STOP;
}

/***********************************************************************/

//
// constructor(s)
//______________________________________________________________________
SupixFPGA::SupixFPGA()
   : m_lock_file("/tmp/supix_lock")
   , m_xillybus_mem("/dev/xillybus_write_8")
   , m_xillybus_fifo("/dev/xillybus_read_32")
{
   TRACE;

   int nsignals = sizeof(signals) / sizeof(int);
   for (int i=0; i < nsignals; i++) {
      errno = 0;	// reset errno for signal()
      signal(signals[i], sig_handler);
      LOG << "register signal " << signals[i] << ": " << errno << endl;
      if (errno) {
	 string msg = "register signal ";
	 msg += to_string(signals[i]);
	 perror(msg.c_str() );
      }
   }

   // switches
   m_mode_debug		= false;
   m_write_raw		= false;
   m_write_root		= false;

   m_dev_fifo	= m_xillybus_fifo;
   m_dev_mem	= m_xillybus_mem;

   m_fd_fifo = -1;
   m_fd_out = -1;	// 1 = stdout

   // default run config
   set_read_frames();
   set_chip_addr();
   set_pre_trigs();
   set_post_trigs();
   set_trig_period();
   set_trig_cds();
   // m_runinfo.chip_addr		= 0;
   // m_runinfo.pre_trigs  	= 10;		// 320 us
   // m_runinfo.post_trigs 	= 100;		// 3.2 ms
   // m_runinfo.trig_period	= 31250;	// 1 sec
   // m_runinfo.trig_cds		= 10;		// threshold

   m_file_maxbytes = 1*GB;

   set_print_freq();

   m_tfile = 0;
   m_tree = 0;
}


//
// destructor
//______________________________________________________________________
SupixFPGA::~SupixFPGA() {
   TRACE;
}


//______________________________________________________________________
void SupixFPGA::initialize()
{
   TRACE;
   m_fd_mem	= open_fd(m_dev_mem.c_str(),  O_WRONLY);
   m_fd_fifo	= open_fd(m_dev_fifo.c_str(), O_RDONLY);

   for (int i=0; i < NROWS; i++) {
      for (int j=0; j < NCOLS; j++) {
	 m_pixel_adc[i][j] = 0;
	 m_pixel_last[i][j] = 0;
	 m_pixel_cds[i][j] = 0;
      }
   }

   // pipeline
   m_pipeline.size	= m_runinfo.pre_trigs + 1;
   m_pipeline.bytes	= m_pipeline.size * FRAMEBYTES;
   m_pipeline.saved	= 0;
   m_pipeline.post_trigs= 0;
   m_pipeline.buffer	= (unsigned char*)malloc(m_pipeline.bytes);
   m_pipeline.pixel_cds	= (Int_t*)malloc(m_pipeline.size * FRAMESIZE * sizeof(*m_pipeline.pixel_cds) );
   m_pipeline.pixel_adc	= (UShort_t*)malloc(m_pipeline.size * FRAMESIZE * sizeof(*m_pipeline.pixel_adc) );

   m_frame = 0;

   // open files to record.
   //----------------------
   // raw data files
   if (m_write_raw)
      open_raw();

   // ROOT files
   if (m_write_root)
      open_root();
}


//______________________________________________________________________
void SupixFPGA::finalize()
{
   TRACE;

   if (m_fd_mem > 0)
      close_fd(m_fd_mem, m_dev_mem.c_str() );

   if (m_fd_fifo > 0)
      close_fd(m_fd_fifo, m_dev_fifo.c_str() );

   if (m_fd_out > 0)
      close_raw();

   if (m_tfile)
      close_root();

   // free pipeline
   if (m_pipeline.buffer) {
      free(m_pipeline.buffer);
      free(m_pipeline.pixel_cds);
      free(m_pipeline.pixel_adc);
   }

}

// debug mode
//   using a fifo to fake /dev/xillybus_read_32.
//
// create a fifo
//   mkfifo ./data/myfifo
//______________________________________________________________________
void SupixFPGA::set_mode_debug()
{
   TRACE;
   m_mode_debug = true;
   m_dev_mem	= "./data/mymem";
   m_dev_fifo	= "./data/myfifo";
}

//______________________________________________________________________
// open a file/dev
//______________________________________________________________________
int SupixFPGA::open_fd(const char *path, int oflag)
{
   TRACE;

   int fd = open(path, oflag);

   if (fd < 0) {
      if (errno == ENODEV)
	 fprintf(stderr, "(Maybe %s a read-only file?)\n", path);

      string msg = "Failed to open ";
      msg += path;
      perror(msg.c_str() );
      exit(1);
   }

   LOG << path << " ... opened." << endl;

   return fd;
}

// close an open fd
//______________________________________________________________________
int SupixFPGA::close_fd(int fd, const char *msg)
{
   TRACE;

   int status = close(fd);
   if (status == -1) {
      perror(msg);
      exit(EXIT_FAILURE);
   }
   return status;
}


int SupixFPGA::AddrSelect(int fd,unsigned char* buf, int len){
   int sent = 0;
   int rc;
  
   while(sent<len){
      rc = write(fd, buf+sent, len - sent);

      if((rc < 0) && (errno == EINTR))
	 continue;
   
      if(rc < 0){
	 perror("AddrSelect() failed to write");
	 exit(1);
      }

      if(rc == 0){
	 fprintf(stderr,"Reached write EOF(?!)\n");
	 exit(1);


      }

      sent += rc;
   }
   return sent;
}

// select a sub-matrix on the chip
//______________________________________________________________________
int SupixFPGA::select_chip_addr()
{
   TRACE;

   // encode command to send
   unsigned char cmd = 0xF8 | m_runinfo.chip_addr;
   int retval = write_mem(cmd);
   LOG << "write_mem(" << hex << (int)cmd << "): " << dec
       << retval
       << endl;

   return retval;
}

// write Xillybus dual port memory.
// 1 byte
// need NOT address
int SupixFPGA::write_mem(unsigned char data)
{
   TRACE;

   int fd = m_fd_mem;

   int rc = write(fd, &data, 1);

   LOG << "bytes " << rc << " data "
       << showbase
       << hex << (unsigned)data << dec
       << noshowbase
       << endl;

   // if ((rc < 0) && (errno == EINTR))
   //    continue;

   if (rc < 0) {
      perror("write_mem() failed to write");
      exit(1);
   }

   if (rc == 0) {
      fprintf(stderr, "Reached write EOF (?!)\n");
      exit(1);
   }

   return rc;
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
int SupixFPGA::write_mem(int address, unsigned char data)
{
  int fd = m_fd_mem;

  if (lseek(fd, address, SEEK_SET) < 0) {
    perror("Failed to seek");
    exit(1);
  }

  int status = write_all(fd, &data, 1);

  if (status != 1) {
     LOG << "something WRONG!" << endl;
  }

  return status;
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
int SupixFPGA::read_all(int fd, unsigned char *buf, int nbytes)
{
   int received = 0;
   int rc;

   while (received < nbytes) {
      rc = read(fd, buf + received, nbytes - received);

      if ((rc < 0) && (errno == EINTR))
	 continue;

      if (rc < 0) {
	 perror("read_all() failed to read");
	 exit(1);
      }

      if (rc == 0) {
	 LOG << "Reached read EOF (?!): " << received << " bytes read" << endl;
	 // exit(1);
	 // return rc;
	 break;
      }

      received += rc;
   }

/*
#ifdef DEBUG
   LOG << "fd-" << fd
       << " requested " << nbytes
       << ", received " << received
       << " [bytes]" << endl;
#endif
*/

   return received;
}


/*
   Plain write() may not write all bytes requested in the buffer, so
   write_all() loops until all data was indeed written, or exits in
   case of failure, except for EINTR. The way the EINTR condition is
   handled is the standard way of making sure the process can be suspended
   with CTRL-Z and then continue running properly.

   The function has no return value, because it always succeeds (or exits
   instead of returning).

   The function doesn't expect to reach EOF either.
*/
int SupixFPGA::write_all(int fd, unsigned char *buf, int nbytes)
{
   static unsigned long nbytes_file = 0;	// bytes written into a file
   static unsigned long nbytes_read = 0;	// total bytes read

   int sent = 0;
   int rc;

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

   nbytes_file += nbytes;
   nbytes_read += nbytes;

   if (m_write_raw && nbytes_file >= m_file_maxbytes) {
      LOG << "fd-" << fd
	  << " sent " << sent
	  << " nbytes_file " << nbytes_file
	  << " nbytes_read " << nbytes_read
	  << endl;

      close_raw();
      open_raw();

      nbytes_file = 0;
   }

   return sent;
}

/* read_fifo.c -- Demonstrate read from a Xillybus FIFO.

This simple command-line application is given one argument: The device
file to read from. The read data is sent to standard output.

This program has no advantage over the classic UNIX 'cat' command. It was
written merely to demonstrate the coding technique.

We don't use allread() here (see memread.c), because if less than the
desired number of bytes arrives, they should be handled immediately.

See http://www.xillybus.com/doc/ for usage examples an information.

*/

// main loop of DAQ
//
// Read from a Xillybus FIFO.
//   See http://www.xillybus.com/doc/ for usage examples an information.
//
// nbytes = 0		non-stop, write
//        < FRAMEBYTES	not write
//        others	write
//______________________________________________________________________
int SupixFPGA::read_fifo(int nbytes)
{
   TRACE;
   int fd = m_fd_fifo;
   int rc;
   int status;
   int nleft = nbytes;
   // static unsigned long nread = 0;	// count read times

   m_run_status = RUN_START;

   bool daq_mode = ! m_locate_last_pixel;

   // for setw()
   int nx = 1;
   if (m_read_frames > 0)
      nx = log10(m_read_frames) + 1;

   // tot_written = 0;
   while (true) {
      // read buffer
      m_buffer = m_pipeline.buffer + m_pipeline.inow * FRAMEBYTES;

      // read all bytes requested.
      if (nleft >= FRAMEBYTES || nbytes == 0)
	 rc = read_all(fd, m_buffer, FRAMEBYTES);
      else
	 rc = read_all(fd, m_buffer, nleft);

      if (rc <= 0)
	 break;

      // EOF or something wrong
      if (daq_mode && rc < FRAMEBYTES) {
	 break;
      }

      // trigger
      if (daq_mode) {
	 status = do_trigger();

	 // something wrong
	 if (status < 0) {
	    rc *= -1;
	    break;
	 }
	 else if (status)
	    m_runinfo.ntrigs++;

	 // nothing wrong
	 if (m_frame_1st)
	    m_frame_1st = false;
	 m_runinfo.nreads++;
      }

      // next
      //-----
      if (nbytes > 0) {
	 nleft -= rc;
      }

      if (daq_mode && m_runinfo.nreads % m_print_freq == 1) {
	 LOG << "[frames]"
	     << " read " << setw(nx) << m_runinfo.nreads
	     << " expected " << m_read_frames
	     << " [bytes]"
	     << " expected " << nbytes
	     << " last " << rc
	     << " left " << nleft
	    // << ", written " << tot_written
	     << endl;
      }

      // end reading?
      if (m_read_frames > 0 && m_runinfo.nreads >=  m_read_frames) {
	 break;
      }

      // catch signal
      if (m_run_status == RUN_STOP) {
	 LOG << "signal catched: stopping run." << endl;
	 break;
      }

   }//~end of loop

   // last read
   // - m_runinfo.nreads=1 always printed
   if (daq_mode && m_runinfo.nreads > 1) {
      LOG << "[frames]"
	  << " read " << setw(nx) << m_runinfo.nreads
	  << " expected " << m_read_frames
	  << " [bytes]"
	  << " expected " << nbytes
	  << " last " << rc
	  << " left " << nleft
	 // << ", written " << tot_written
	  << endl;
   }

   // if (daq_mode)
   //    print_buffer(m_buffer, nbytes);

   // nread = 0;

   return rc;
}

//______________________________________________________________________
int SupixFPGA::open_raw()
{
   m_rawfn = m_datadir + "/" + m_datatag + "_";
   m_rawfn += time_tag();
   m_rawfn += ".data";

   mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP ;

   m_fd_out = open(m_rawfn.c_str(), O_WRONLY|O_CREAT, mode);
   if (m_fd_out == -1) {
      perror(m_rawfn.c_str());
      exit(EXIT_FAILURE);
   }

   LOG << "fd-" << m_fd_out
       << " " << m_rawfn << " ... opened for writing"
       << endl;

   return m_fd_out;
}

//______________________________________________________________________
int SupixFPGA::close_raw()
{
   int status = close_fd(m_fd_out, m_rawfn.c_str());
   LOG << "fd-" << m_fd_out
       << " " << m_rawfn << " ... closed"
       << endl;

   return status;
}



// write raw date into .root files
//______________________________________________________________________

// prepare ROOT file and TTree.
//______________________________________________________________________
int SupixFPGA::open_root()
{
   m_rootfn = m_datadir + "/" + m_datatag + "_";
   m_rootfn += time_tag(false);
   m_rootfn += ".root";

   // This file is now the current directory.
   m_tfile = new TFile(m_rootfn.c_str(), "NEW");
   LOG << m_rootfn << " ... opened: " << m_tfile << endl;
   open_tree();
   return 0;
}


// close the current ROOT file.
//______________________________________________________________________
int SupixFPGA::close_root()
{
   // renew current file due to TTree::SetMaxTreeSize().
   m_tfile = gDirectory->GetFile();
   // LOG << m_tfile->GetName() << endl;

   int status = m_tree->FlushBaskets();
   // m_tree->Write();
   m_tree->Print();

   // save all objects in the file
   //m_tfile->Flush();
   m_tfile->Write();

   // save run info
   // - has to Clear() before deleting the tree.
   m_tree->GetUserInfo()->First()->Print();
   m_tree->GetUserInfo()->Clear();

   // m_runinfo.Write("runinfo");

   m_tfile->ls();
   m_tfile->Close();

   // NOT delete m_tfile and m_tree!
   LOG << m_tfile->GetName() << " ... closed:"
       // << " tfile= " << m_tfile
       // << " tree= " << m_tree
       << " status= " << status
       << endl;

   return status;
}

// book tree
//______________________________________________________________________
int SupixFPGA::open_tree()
{
   m_tree = new TTree("supix", "test chip");

   // if the file size reaches TTree::GetMaxTreeSize(), the current
   // file is closed and a new file is created as filename_N.root.
   TTree::SetMaxTreeSize(m_file_maxbytes);

   // per second
   m_tree->SetAutoSave(FRAMES_SEC * FRAMEBYTES);

   string leaflist;

   // s = UShort_t, S = Short_t

   // ADC of frame_now
   leaflist = "pixel_adc[" + to_string(NROWS) + "][" + to_string(NCOLS) + "]/s";
   m_tree->Branch("pixel_adc", m_pixel_adc, leaflist.c_str() );

   // CDS = frame_now - frame_prev
   leaflist = "pixel_cds[" + to_string(NROWS) + "][" + to_string(NCOLS) + "]/I";
   m_tree->Branch("pixel_cds", m_pixel_cds, leaflist.c_str() );

   // #frame
   m_tree->Branch("frame", &m_frame, "frame/l" );

   // trigger
   m_tree->Branch("trig", &m_trig, "trig/b" );

   // first frame flag
   m_tree->Branch("frame_1st", &m_frame_1st, "frame_1st/O" );

   LOG << "TTree::SetMaxTreeSize(" << m_file_maxbytes << ")"
       << " SetAutoSave(" << FRAMES_SEC * FRAMEBYTES << ")"
       << endl;

   // attach RunInfo
   m_tree->GetUserInfo()->Add(&m_runinfo);

   return 0;
}

// buf = m_buffer
//______________________________________________________________________
int SupixFPGA::fill_tree(unsigned char *buf, int nbytes)
{
   // must be a frame of data
   if (nbytes != FRAMESIZE) {
      LOG << "bufsize " << nbytes << " < " << FRAMESIZE << endl;
      return 0;
   }

   // 0x7fff = 32767
   UInt_t	*fpga = (UInt_t*)buf;
   UShort_t	adc, col, row, frame;
   static int count = 0;
//   int frame_same = 0;

   for (int ir=0; ir < NROWS; ir++) {
      for (int ic=0; ic < NCOLS; ic++) {
	 fpga_decode( *fpga, adc, col, row, frame );
	 m_pixel_adc[ir][ic] = adc;
	 m_pixel_cds[ir][ic]  = m_pixel_adc[ir][ic] - m_pixel_last[ir][ic];
	 m_pixel_last[ir][ic] = m_pixel_adc[ir][ic];

//	 check_adcinfo(ir,ic,col,row);
  // 	 frame_same += check_frame(int ir,int ic,frame); 

	 // next
	 fpga++;

	 // print information for (1 frame + 1 row).
	 if (count < (1 + NROWS)*NCOLS) {
	    count++;
	    LOG;
	    printf("(%2u, %2u) = adc %5u [frame %2u, row %2u, col %2u]\n"
		   , ir, ic, adc, frame, row, col);
	 }
      }
   }
   
/*
   if(frame_same == FRAMESIZE)
   frame_circle(frame);   
   else
     LOG<<"Failed to frame_circle : ERROR for single frame check in frame  "<< m_frame << endl;
*/
   m_tree->Fill();

   return nbytes;
}

//______________________________________________________________________
bool SupixFPGA::locate_last_pixel()
{
   m_locate_last_pixel = true;
   bool yes = false;
   int nbytes = 4;
   const ushort pixel_addr_last = 0x40F;    // row infromation changed: 1~64, 40F For full matix --LongLI 2020-05-28 
   // const ushort pixel_addr_mask = 0xFF7;
   ushort pixel_addr;
   int count = 0;
   // ushort *data = (ushort*)m_buffer;
   uint data;
   ushort adc, col, row, frame;

   while(true)
   {
      count++;

      read_fifo(nbytes);
      data = *( (uint*)m_buffer);
      fpga_decode(data, adc, col, row, frame);
      pixel_addr = (row << 4) + col;

      LOG << setw(2) << count << ": 0x ";
      // printf("%p:", m_buffer);
      for (int i=0; i<nbytes; i++) {
	 printf(" %02x", *(m_buffer + i) );
      }
      printf(" pixel 0x%03x", pixel_addr);
      cout << ": (frame, row, col, adc) = ("
	   << frame << ", " << row << ", " << col << ", " << adc << ")"
	   << endl;

      if (count > NROWS*NCOLS) break;

      if (pixel_addr == pixel_addr_last) {
	 yes = true;
	 break;
      }
   }

   if (! yes) {
      LOG;
      printf("last pixel %#x NOT found!\n", pixel_addr_last);
   }

   m_locate_last_pixel = false;
   return yes;
}

// get time tag for file name.
//______________________________________________________________________
std::string SupixFPGA::time_tag(bool count)
{
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

// decode FPGA word (4 bytes)
// ushort NOT work!???
//______________________________________________________________________
void SupixFPGA::fpga_decode(UInt_t data, UShort_t &adc, UShort_t &col, UShort_t &row, UShort_t &frame)
{
   const UInt_t
      mask_adc	 = 0x0000FFFF,
      mask_col	 = 0x000F0000,
      mask_row	 = 0x07F00000,    //row information changed: 1~64  --LongLI 2020-05-28
      mask_frame = 0xF0000000
      ;
   adc	= ( data & mask_adc );
   col	= ( data & mask_col ) >> 16;
   row	= ( data & mask_row ) >> 20;
   frame= ( data & mask_frame ) >> 28;
}

// print buffer bytes
//______________________________________________________________________
void SupixFPGA::print_buffer(unsigned char* buf,int nbytes)
{
   printf("%16s", "buffer:");
   for (int i=0; i<nbytes; i++) {
      if (i>0 && i % (NCOLS * 4) == 0) {
	 printf("\t\t");
      }
      printf(" %02x", *(buf + i) );
      if (i % (NCOLS * 4) == 63) {    //for full matrix change 7 tp 63 LongLI --2020-05-28
	 printf("\n");
      }
   }
   // printf("\n");
}

// run control
//______________________________________________________________________
int SupixFPGA::start_run()
{
   TRACE;
   int retval = 0;
   if (m_mode_debug)
      retval = getpid();
   else
      retval = lock_run();
   LOG << "process " << retval << endl;

   initialize();
   select_chip_addr();
   m_runinfo.set_time_start();

   // in case of frame alignment wrong
   bool keep_running = false;
   do {
      locate_last_pixel();
      m_frame_1st = true;
      retval = read_fifo(m_read_frames * FRAMEBYTES);
      keep_running = (retval < 0);
   }
   while (keep_running);

   return retval;
 }


int SupixFPGA::stop_run()
{
   TRACE;
   int retval = 0;

   m_runinfo.set_time_stop();
   if (! m_tree)
      m_runinfo.Print();

   finalize();

   if (! m_mode_debug) {
      // remove the lock file
      string shcmd = "rm ";
      shcmd += m_lock_file;
      retval = system(shcmd.c_str() );
      if (retval) {
	 perror(m_lock_file);
      }
   }

   return retval;
}

// check lock_file
// return: pid
//______________________________________________________________________
int SupixFPGA::lock_run()
{
   TRACE;
   int retval = 0;
   string shcmd;

   // make sure only ONE process is running.
   shcmd = "test -f ";
   shcmd += m_lock_file;
   retval = system(shcmd.c_str() );
   if (retval == 0) {
      LOG << m_lock_file << " exists: "
	  << "Probably another DAQ process is running." << endl;
      exit(1);
   }

   // create a lock file.
   shcmd = "touch ";
   shcmd += m_lock_file;
   retval = system(shcmd.c_str() );
   if (retval != 0) {
      LOG << "Lock file " << m_lock_file << " can NOT be created!!!" << endl;
      exit(1);
   }

   // write pid into the lock file.
   pid_t pid = getpid();
   shcmd = "echo ";
   shcmd += to_string(pid);
   shcmd += " > ";
   shcmd += m_lock_file;
   retval = system(shcmd.c_str() );
   if (retval) {
      perror(m_lock_file);
      exit(1);
   }

   // int fd = open(m_lock_file, O_WRONLY);
   // if (fd < 0) {
   //    perror(m_lock_file);
   //    exit(1);
   // }

   // retval = write(fd, &pid, sizeof(pid_t) );
   // if (retval < 0) {
   //    perror(m_lock_file);
   //    exit(1);
   // }

   return pid;
}

// perform triggers
// return: trigger modes
//______________________________________________________________________
int SupixFPGA::do_trigger()
{
   UChar_t trig = 0;

   // LOG << "frame " << m_frame
   //     << " trig 0x" << setw(2) << setfill('0') << hex << (unsigned)m_trig << dec
   //     << " " << m_pipeline.sprint()
   //     << endl;

   // record frame periodically
   if ( m_frame % m_runinfo.trig_period == 0)
      trig |= TRIG_PERIOD;

   // CDS trigger
   int retval = trig_cds();

   // immediately return for soft error
   if (retval < 0) {
      return retval;
   }
   else if (retval)
      trig |= TRIG_CDS;

   m_trig = trig;

   // take data according to trigger
   if (trig) {				// new-trigger = 1
      if (m_pipeline.post_trigs) {	// post-triggers > 0
	 record_frame(m_pipeline.inow);
      }
      else {				// post-triggers = 0
	 // set post-triggers
	 m_pipeline.post_trigs = m_runinfo.post_trigs;

	 // record pre-trigger and this frames
	 record_pre_trigs();

	 // reset saved-frames count
	 m_pipeline.saved = 0;
      }
   }
   else {				// new-trigger = 0
      if (m_pipeline.post_trigs) {	// post-triggers > 0
	 record_frame(m_pipeline.inow);
	 m_pipeline.post_trigs--;
      }
      else {				// post-triggers = 0
	 m_pipeline.saved++;
      }
   }

   // next in pipeline
   m_pipeline.inow++;
   m_pipeline.inow %= m_pipeline.size;

   // print information
   const int freq = m_runinfo.trig_period > FRAMES_SEC ? m_runinfo.trig_period : FRAMES_SEC ;
   if ( (trig & TRIG_CDS) || (m_runinfo.ntrigs%freq == 0) )
      LOG << "frame " << m_frame
	  << " trig 0x" << setw(2) << setfill('0') << hex
	  << (unsigned)m_trig << setfill(' ') << dec
	  << " " << m_pipeline.sprint()
	  << endl;

   m_frame++;
   return trig;
}

// perform CDS trigger
// - save CDS and ADC into pipeline.
//______________________________________________________________________
int SupixFPGA::trig_cds()
{
   int trig = 0;

   // 0x7fff = 32767
   UInt_t	*fpga = (UInt_t*)m_buffer;
   Int_t 	*cds_now = m_pipeline.pixel_cds + m_pipeline.inow * FRAMESIZE;
   UShort_t 	*adc_now = m_pipeline.pixel_adc + m_pipeline.inow * FRAMESIZE;
   UShort_t	adc, col, row, frame;

   // // lilong
   // int frame_same = 0;

   for (int ir=0; ir < NROWS; ir++) {
      for (int ic=0; ic < NCOLS; ic++) {
	 fpga_decode( *fpga, adc, col, row, frame );

	 // check first
	 int status = check_integrity(ir, ic, frame, row, col);
	 if (status)
	    return -1;

	 // m_pixel_adc[ir][ic] = adc;
	 *cds_now = adc - m_pixel_last[ir][ic];
	 *adc_now = adc;

	 // update the last for CDS calculation
	 m_pixel_last[ir][ic] = adc;


	 // // lilong
   	 // check_adcinfo(ir,ic,col,row);
	 // frame_same += check_frame(ir,ic,frame);

	 // save in pipeline
	 if ( *cds_now >= m_runinfo.trig_cds[ir][ic]) {
	    trig++;

	    // LOG;
	    // printf("%d: (%2u, %2u) = adc %5u [frame %2u, row %2u, col %2u]\n"
	    // 	   , trig, ir, ic, adc, frame, row, col);
	 }

	 // next
	 fpga++;
	 cds_now++;
	 adc_now++;
      }
   }

   // // lilong
   // if(frame_same == FRAMESIZE && m_runinfo.trig_cds < -99999)
   // frame_circle(frame);
   // else if (m_runinfo.trig_cds < -99999)
   // LOG<<"Failed to frame_circle:  for FRAME MISMATCH in frame "<< m_frame<< endl;

   return trig;
}

// record pre-triggers
//______________________________________________________________________
int SupixFPGA::record_pre_trigs()
{
   int nrecords = 0;

   int pre_trigs = m_runinfo.pre_trigs;
   if (m_pipeline.saved < pre_trigs)
      pre_trigs = m_pipeline.saved;

   // save current frame information
   ULong_t	frame = m_frame;
   UChar_t	trig  = m_trig;

   // including current frame
   while(pre_trigs >= 0) {
      nrecords++;
      int ith = m_pipeline.inow - pre_trigs;
      // cyclic
      if (ith < 0)
	 ith += m_pipeline.size;

      // recover previous frame information
      m_frame = frame - pre_trigs;
      if (pre_trigs > 0)
	 m_trig = 0;
      else
	 m_trig = trig;

      // record the frame
      record_frame(ith);

      pre_trigs--;
   }

   if (m_frame % m_print_freq == 1)
      LOG << "frame#" << m_frame
	  << " pipeline-" << m_pipeline.inow
	  << " nrecords = " << nrecords
	  << endl;
   return nrecords;
}


// fill tree with i-th frame in pipeline
//______________________________________________________________________
int SupixFPGA::fill_tree(int ith)
{
   Int_t	*cds_now = m_pipeline.pixel_cds + ith * FRAMESIZE;
   UShort_t	*adc_now = m_pipeline.pixel_adc + ith * FRAMESIZE;
   for (int ir=0; ir < NROWS; ir++) {
      for (int ic=0; ic < NCOLS; ic++) {
	 m_pixel_cds[ir][ic]  = *cds_now;
	 cds_now++;

	 m_pixel_adc[ir][ic]  = *adc_now;
	 adc_now++;
      }
   }

   m_tree->Fill();

#ifdef DEBUG
   static int post_trigs = 0;
   if (m_frame % m_print_freq == 1 || post_trigs) {
      post_trigs = m_pipeline.post_trigs;
      LOG << "frame#" << m_frame << " pipeline-" << ith << endl;
   }
#endif

   return 0;
}


// write i-th frame in pipeline into .data file
//______________________________________________________________________
int SupixFPGA::write_raw(int ith)
{
   unsigned char *buf = m_pipeline.buffer + ith * FRAMEBYTES;
   int nbytes = write_all(buf, FRAMEBYTES);
#ifdef DEBUG
   static int post_trigs = 0;
   if (m_frame % m_print_freq == 1 || post_trigs) {
      post_trigs = m_pipeline.post_trigs;
      LOG << "frame#" << m_frame << " pipeline-" << ith << endl;
   }
#endif
   return nbytes;
}

// record an frame
//______________________________________________________________________
int SupixFPGA::record_frame(int ith)
{
   static int nprints = 0;

   if (m_write_root)
      fill_tree(ith);

   if (m_write_raw)
      write_raw(ith);

   static int post_trigs = 0;
   if ( ! (m_write_root || m_write_raw) &&
	(m_frame % m_print_freq == 0 || post_trigs)
	&& nprints < 100*(m_pipeline.size + m_runinfo.post_trigs)
	) {
      nprints++;
      post_trigs = m_pipeline.post_trigs;

      unsigned char *buf = m_pipeline.buffer + ith * FRAMEBYTES;
      LOG << "#" << nprints << " frame " << m_frame << endl;
      print_buffer(buf, 4*NCOLS*3);
   }

   m_runinfo.nrecords++;

   return 0;
}

std::string SupixFPGA::m_pipeline::sprint()
{
   ostringstream oss;
   oss << "pipeline:"
       << " size(" << size << ")"
       << " bytes(" << bytes << ")"
       << " saved(" << saved << ")"
       << " post_trigs(" << post_trigs << ")"
       << " inow(" << inow << ")"
      ;
   return oss.str();
}




/*
void SupixFPGA::check_adcinfo(int ir, int ic, UShort_t col, UShort_t row)
{
   if(col != ic + 6)
      LOG << endl
	  << left <<"COLUMN MISMATCH: " << "\n" << "frame: " << m_frame<< "  pixel fill to tree" << "\t" << "pixel in raw data" 
	  <<endl<<left<<"\t\t"<<"pixel["<<ir<<"]["<<ic<<"]\t\t\t"<<"pixel["<<row<<"]["<<col-6<<"]\n"<<endl;
   
   if(row != ir )
      LOG << endl
	  <<left<<"ROW MISMATCH: " << "\n" << "frame: " << m_frame << "  pixel fill to tree" << "\t" << "pixel in raw data"
	  <<endl<<left<<"\t\t"<<"pixel["<<ir<<"]["<<ic<<"]\t\t\t"<<"pixel["<<row<<"]["<<col-6<<"]\n"<< endl;

}

int SupixFPGA::check_frame(int ir, int ic, UShort_t frame)
{
   if(ic==0 && ir == 0){   
      m_frame_now = frame;
      m_frame_last = frame;
      return 1;
   }
   else {
      m_frame_now = frame;


         
      if(m_frame_now == m_frame_last) return 1;
      else  {
	 LOG<<endl
	    <<left<< "FRAME MISMATCH:" << "\n" << "frame: "<<m_frame<<"\tFrame number mismatch in a single frame at pixel["
	    <<ir<<"]["<<ic<<"]"<<endl
	    <<left<<"last frame tag" << "\t" << "this frame tag"<<endl
	    <<left<<"\t"<<m_frame_last<<"\t\t"<<m_frame_now<<"\n"<<endl;
	 return 0;
      }
      m_frame_last = m_frame_now;
   
   }


}

void SupixFPGA::frame_circle(UShort_t frame)
{
   if(m_frame == 0){
      m_frame_cir_now = frame;
      m_frame_cir_last = frame;
   }
   else  {
      m_frame_cir_now = frame;
      if(m_frame_cir_now == 0){
	 if(m_frame_cir_last != 15)	
	    LOG <<endl
		<<left<< "FRAME TAG CIRCLE MISMATCH: " << "frame: "<<m_frame<<"\tframe tag is not continuous" << "\t"
		<<"last frame tag  " << m_frame_cir_last << "\t" << "this frame tag  " 
		<< m_frame_cir_now << "\n"  << endl;
      }
      else {
	 if(m_frame_cir_now != m_frame_cir_last + 1)
	    LOG <<endl
		<<left<< "FRAME TAG CIRCLE MISMATCH: " << "frame: "<<m_frame<<"\tframe tag is not continuous" << "\t"
		<< "last frame tag  " << m_frame_cir_last << "\t" << "this frame tag  "
		<< m_frame_cir_now << "\n"<< endl;

      }
      m_frame_cir_last = m_frame_cir_now;
      
   }

}

*/



// check frame data integrity
//______________________________________________________________________
int SupixFPGA::check_integrity(int ir, int ic, UShort_t frame, UShort_t row, UShort_t col)
{
   int retval = 0;
   const int wrong_col	  = 0x1;
   const int wrong_row    = 0x2;
   const int wrong_frame1 = 0x4;	// in a frame
   const int wrong_frame2 = 0x8;	// consecutive frame
   const int max_frame    = 16;

   static int frame_last = -1;
   static int frame_this = -1;

   // initialize
   if (m_frame_1st) {
      frame_last = frame;
      frame_this = frame;
   }

   // first data in a frame
   // if (ir == 0 && ic == 0) {
   if (frame != frame_this) {
      frame_last = frame_this;
      frame_this = frame;

      // should happen at the first pixel
      if ( ! (ir == 0 && ic == 0 ) )
	 retval += wrong_frame1;

      // should be consecutive
      int diff = frame_this - frame_last;
      if (diff < 0)
	 diff += max_frame;
      if (! m_frame_1st && diff != 1)
	 retval += wrong_frame2;
   }

   // check data

   if (col != ic)
      retval += wrong_col;

   if (row != ir + 1)
      retval += wrong_row;

   // if (frame != frame_this)
   //    retval += wrong_frame1;

   if (retval) {
      LOG << "WRONG #" << m_frame
	  << " 0x" << hex << setw(2) << setfill('0')
	  << retval
	  << setfill(' ') << dec
	  << " (frame, row, col):"
	  << " expected (" << (frame_last+1) % max_frame << ", " << ir+1 << ", " << ic << ")"
	  << " read (" << frame << ", " << row << ", " << col << ")"
	  << endl;
   }

   return retval;
}

void SupixFPGA::set_trig_cds(int x, int chip_addr){
	int m = x > 0xFFFF ? 5 : x;
	double noise = 0;
	char noisefile[80];
	sprintf(noisefile,"PixelNoise_A%d.txt", chip_addr-11);
	ofstream fout("trig_cds.txt");
	ifstream fin;
	fin.open(noisefile);
	if(!fin.is_open()){
		std::cout<<"Failed to open the file!"<<std::endl;
		exit(-1);
	}
	for(int i =0; i<NROWS; i++){
		for(int j = 0; j<NCOLS; j++){
			fin>>noise;
			m_runinfo.trig_cds[i][j] = (int)(m * noise);
			fout<<chip_addr<<"\t"<<noisefile<<"\t"<<m<<"x "<<noise<<" = "<<m_runinfo.trig_cds[i][j]<<endl;
		}
	}
	fin.close();
	fout.close();


}





