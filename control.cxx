/*******************************************************************//**
 * $Id: control.cxx 1141 2020-06-22 10:31:56Z mwang $
 *
 * 
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2019-04-15 14:03:16
 * @copyright:  (c)2019 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
//#include "control.h"
#include "SupixFPGA.h"

#include <unistd.h>     // for getopt()
#include <iostream>
using namespace std;

enum UNIT_TESTS { NOTEST, START_RUN, STOP_RUN, LOCK_RUN };

//______________________________________________________________________
void usage(char **argv) {
   cout << "Usage: " << argv[0] << " [options]" << endl
	<< "\t\t-h		# print this" << endl
	<< "\t\t-R		# write ROOT files" << endl
	<< "\t\t-T		# test mode" << endl
	<< "\t\t-W		# write raw data files" << endl
	<< "\t\t-a <addr>	# [0, 7], chip addr. to read" << endl
	<< "\t\t-f <N>		# max file size in MiB" << endl
	<< "\t\t-n <N>		# N frames to read. 0 = infinite" << endl
	<< "\t\t-o <N>		# trigger per N frames" << endl
	<< "\t\t-p <N>		# N frames to record pre-trigger" << endl
	<< "\t\t-q <N>		# N frames to record post-trigger" << endl
	<< "\t\t-s <tag>	# string tag for output files as: tag_yymmdd_HHMMSS..." << endl
	<< "\t\t-t <ncds>	# trigger threshold on CDS" << endl
	<< "\t\t-u <n>		# unit test"
	<< ": " << START_RUN << "=START_RUN"
	<< ", " << STOP_RUN << "=STOP_RUN"
	<< ", " << LOCK_RUN << "=LOCK_RUN"
	<< endl
      ;
   exit(0);
}


//======================================================================
int main(int argc, char **argv)
{
   // defaults
   bool debug		= false;
   bool to_raw		= false;
   bool to_root	= false;

   // run config
   unsigned long nframes = 0;
   int chip_addr = -1;
   int trig_period = -1;
   int trig_cds = 0x10000;	// beyond UShort_t
   int pre_trigs = -1;
   int post_trigs = -1;

   string rawdir = "/userdata/supix/data";
   string rawtag = "raw";
   int unit_test = NOTEST;
   int fsize = 1000;	// MiB

   //................................................ command line options
   // getopt() pre-defined variables:
   //   optind
   //   optarg
   int copt;
   while ( (copt = getopt(argc, argv, "hTRWa:f:n:o:p:q:s:t:u:")) != -1) {
      switch (copt) {
      case 'R':
	 to_root = true;
	 break;
      case 'T':
	 debug = true;
	 rawtag = "test";
	 break;
      case 'W':
	 to_raw = true;
	 break;
      case 'a':
         sscanf(optarg, "%d", &chip_addr);
         break;
      case 'f':
         sscanf(optarg, "%d", &fsize);
         break;
      case 'n':
         sscanf(optarg, "%lu", &nframes);
         break;
      case 'o':
         sscanf(optarg, "%d", &trig_period);
         break;
      case 'p':
         sscanf(optarg, "%d", &pre_trigs);
         break;
      case 'q':
         sscanf(optarg, "%d", &post_trigs);
         break;
      case 's':
         rawtag = optarg;
         break;
      case 't':
         sscanf(optarg, "%d", &trig_cds);
         break;
      case 'u':
         sscanf(optarg, "%d", &unit_test);
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


   SupixFPGA *supix = new SupixFPGA;

   if (debug) {
      supix->set_mode_debug();
   }

   supix->set_write_root(to_root);
   supix->set_write_raw(to_raw);
   supix->set_file_maxbytes(fsize, MiB);
   supix->set_filename(rawdir, rawtag);

   supix->set_read_frames(nframes);
   supix->set_chip_addr(chip_addr);
   supix->set_pre_trigs(pre_trigs);
   supix->set_post_trigs(post_trigs);
   supix->set_trig_period(trig_period);
   supix->set_trig_cds(trig_cds, chip_addr);

   LOG << "config:"
       << " trig_cds " << trig_cds
       << endl;

   if (unit_test) {
      // do unit test
      switch (unit_test) {
      case START_RUN:
	 supix->start_run();
	 break;
      case STOP_RUN:
	 supix->stop_run();
	 break;
      case LOCK_RUN:
	 supix->lock_run();
	 break;
      default:
	 break;
      }
   }
   else {
      // normal run
      supix->start_run();
      supix->stop_run();
   }

   delete supix;
   return 0;
}
