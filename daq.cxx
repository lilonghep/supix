/*******************************************************************//**
 * $Id: daq.cxx 1219 2020-07-15 08:56:47Z mwang $
 *
 * main DAQ with thread support.
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-22 13:51:39
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "SupixDAQ.h"
#include "error.h"
//#define LOG std::cout <<__PRETTY_FUNCTION__<<" --- "

#include <unistd.h>     // for getopt()
#include <iostream>
using namespace std;

SupixDAQ *g_supix = NULL;

// create a new thread
void * new_thread(void *arg);

enum UNIT_TESTS { NOTEST, START_RUN, STOP_RUN, LOCK_RUN, TEST_FIFO };

//______________________________________________________________________
void usage(char **argv) {
   cout << "Usage: " << argv[0] << " [options]" << endl
	<< "\t\t -h		# print this" << endl
	<< "\t\t -C		# continuous DAQ mode" << endl
	<< "\t\t -L INT		# max frames in pipeline" << endl
	<< "\t\t -N		# noise DAQ mode" << endl
	<< "\t\t -R		# write ROOT files" << endl
	<< "\t\t -T		# test mode" << endl
	<< "\t\t -W		# write raw data files" << endl
	<< "\t\t -a INT		# [0, 7], chip addr. to read" << endl
	<< "\t\t -f INT		# max file size in MiB" << endl
	<< "\t\t -n INT		# N frames to read. 0 = infinite" << endl
	<< "\t\t -o INT		# trigger per N frames" << endl
	<< "\t\t -p INT		# N frames to record pre-trigger" << endl
	<< "\t\t -q INT		# N frames to record post-trigger" << endl
	<< "\t\t -r PATHNAME	# dir for output files" << endl
	<< "\t\t -s STRING	# tag for output files as: STRING_yymmdd_HHMMSS..." << endl
	<< "\t\t -t INT		# trigger threshold on CDS" << endl
	<< "\t\t -u INT		# unit test"
	<< ": " << START_RUN << "=START_RUN"
	<< ", " << STOP_RUN << "=STOP_RUN"
	<< ", " << LOCK_RUN << "=LOCK_RUN"
	<< ", " << TEST_FIFO << "=TEST_FIFO"
	<< endl
	<< "\t\t -v INT		# [0] verbosity" << endl
	<< "\t\t -w INT		# [10] timewait in usec" << endl
	<< "\t\t -z INT		# [1]  timeout in sec" << endl
      ;

   if (g_supix)	delete g_supix;
   exit(0);
}


//======================================================================
int main(int argc, char **argv)
{
   string rawdir = "./data";
   string rawtag = "raw";
   int unit_test = NOTEST;
   bool debug = false;		// mode_debug
   bool noise_run = false;	// noise run
   
   //cout << "supix=" << g_supix << endl;
   SupixDAQ *g_supix = new SupixDAQ;

   //................................................ command line options
   // getopt() pre-defined variables:
   //   optind
   //   optarg
   int copt;
   int xint;
   double xdouble;
   unsigned long xulong;
   while ( (copt = getopt(argc, argv, "hCL:NTRWa:f:n:o:p:q:r:s:t:u:v:w:z:")) != -1) {
      switch (copt) {
      case 'C':
	 g_supix->set_daq_mode(M_CONTINUOUS);
	 break;
      case 'L':
         sscanf(optarg, "%d", &xint);
	 g_supix->set_pipeline_max(xint);
         break;
      case 'N':
	 noise_run = true;
	 g_supix->set_daq_mode(M_NOISE);
	 // g_supix->set_noise_run();
	 break;
      case 'R':
	 g_supix->set_write_root(true);
	 break;
      case 'T':
	 debug = true;
	 break;
      case 'W':
	 g_supix->set_write_raw(true);
	 break;
      case 'a':
         sscanf(optarg, "%d", &xint);
	 g_supix->set_chip_addr(xint);
         break;
      case 'f':
         sscanf(optarg, "%d", &xint);
	 g_supix->set_filesize_max(xint * MiB);
         break;
      case 'n':
         sscanf(optarg, "%lu", &xulong);
	 g_supix->set_maxframe(xulong);
         break;
      case 'o':
         sscanf(optarg, "%d", &xint);
	 g_supix->set_trig_period(xint);
         break;
      case 'p':
         sscanf(optarg, "%d", &xint);
	 g_supix->set_pre_trigs(xint);
         break;
      case 'q':
         sscanf(optarg, "%d", &xint);
	 g_supix->set_post_trigs(xint);
         break;
      case 'r':
         rawdir = optarg;
         break;
      case 's':
         rawtag = optarg;
         break;
      case 't':
         sscanf(optarg, "%lf", &xdouble);
	 g_supix->set_trig_cds_x(xdouble);
         break;
      case 'u':
         sscanf(optarg, "%d", &unit_test);
	 debug = true;
         break;
      case 'v':
         sscanf(optarg, "%d", &xint);
	 g_supix->set_verbosity(xint);
	 break;
      case 'w':
         sscanf(optarg, "%d", &xint);
	 g_supix->set_timewait(xint);
         break;
      case 'z':
         sscanf(optarg, "%d", &xint);
	 g_supix->set_timeout(xint);
         break;
      case 'h':
      default:
         usage(argv);
      }
   }

   /// check non-option arguments
   if (argc == 1 || optind < argc) {
      usage(argv);
   }

   if (debug) {
      g_supix->set_mode_debug();
      rawdir = "./data";
      rawtag = "test";
   }
   g_supix->set_filename(rawdir.c_str(), rawtag.c_str());
   printids("[main] initialize:");
   g_supix->initialize();
   
   if (unit_test) {
      // do unit test
      switch (unit_test) {
      case START_RUN:
	 g_supix->start_run();
	 break;
      case STOP_RUN:
	 g_supix->stop_run();
	 break;
      case LOCK_RUN:
	 g_supix->lock_run();
	 break;
      case TEST_FIFO:
	 g_supix->test_fifo();
	 break;
      default:
	 break;
      }
   }
   else {
      if (noise_run) {
	 printids("[main] noise_run:");
	 g_supix->noise_run();
      }
      else {	 // normal run
	 // create a child thread to write out data
	 printids("[main] pthread_create:");
	 pthread_t ntid;
	 int err = pthread_create(&ntid, NULL, new_thread, g_supix);
	 if (err != 0)	err_sys("pthread_create");
	 usleep(1);

	 // main thread to read in data from fifo
	 printids("[main] start_run:");
	 g_supix->start_run();
	 printids("[main] stop_run:");
	 g_supix->stop_run();
	 //printids("[main] stop_run return:");
      }
   }

   printids("[main] finalize:");
   g_supix->finalize();
   //sleep(1);
   printids("[main] delete:");
   //cout << "supix=" << g_supix << endl;
   delete g_supix;
   //cout << "supix=" << g_supix << endl;
   return 0;
}


void *new_thread(void *arg)
{
   // sleep(1);		// wait start_run()
   SupixDAQ *supix = (SupixDAQ*)arg;
   LOG << supix << endl;
   printids("[child] start:");
   supix->new_thread();
   //sleep(1);
   printids("[child] return:");
   return ((void*)0);
}
