/*******************************************************************//**
 * $Id: io_raw.cxx 1145 2020-06-29 02:56:16Z mwang $
 *
 * test raw data I/O
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-17 18:43:41
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
//#include "mydefs.h"
#include "../util.h"
#include "../common.h"
using namespace std;

#define BUFFSIZE	4096
#define kB		1024
#define MB		1048576		// (1024*1024)
#define GB		1073741824	// (1024*MB)


const char *m_fn = "io_raw.dat";


//======================================================================
int main(void)
{
   unsigned char buf[BUFFSIZE];
   for (int i=0; i < BUFFSIZE; i++)
      buf[i] = i;

   struct timeval tv_start, tv_stop;

   gettimeofday(&tv_start, NULL);
   // printf("sec %ld usec %ld\n", tv_start.tv_sec, (long)tv_start.tv_usec);
   int fd = open_fd(m_fn);

   int maxbytes = 1*GB;
   int totbytes = 0;
   int nbuf = 0;
   while(totbytes < maxbytes) {
      int nbytes = write_all(fd, buf, BUFFSIZE);
      totbytes += nbytes;
      nbuf++;
   }
   
   close_fd(fd);
   gettimeofday(&tv_stop, NULL);
   // printf("sec %ld usec %ld\n", tv_stop.tv_sec, (long)tv_stop.tv_usec);

   long sec	= tv_stop.tv_sec - tv_start.tv_sec;
   long usec	= tv_stop.tv_usec - tv_start.tv_usec;
   long dusec = sec*1e6 + usec;
   double dsec = dusec * 1e-6;
   // cout << sec << " s " << usec << " usec " << dusec << endl;
   double usec_buf = (double)dusec/nbuf;
   double speed = (double)totbytes / MB / dsec;
   
   cout << "frame " << BUFFSIZE
	<< " total " << totbytes << " bytes"
	<< " " << nbuf << " frames"
	<< setprecision(3)
	<< " speed = " << speed << " MByte/sec"
	<< " " << usec_buf << " usec/frame"
	<< " total " << dsec << " sec"
	<< endl;

   // int nframes = 10;
   // int next = 0;
   // for (int i=0; i < 15; i++) {
   //    cout << i << " " << next << endl;
   //    next = (next + 1) % nframes;
   // }
   
   exit(0);
}
