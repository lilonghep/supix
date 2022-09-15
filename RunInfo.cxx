/*******************************************************************//**
 * $Id: RunInfo.cxx 1215 2020-07-13 11:58:22Z mwang $
 *
 * 
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2019-04-24 14:00:20
 * @copyright:  (c)2019 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "RunInfo.h"
#include <stdlib.h>
#include <iomanip>
using namespace std;

// for TObject
ClassImp(RunInfo);

//
// constructor(s)
//______________________________________________________________________
RunInfo::RunInfo()
   : nrows(NROWS), ncols(NCOLS)
{
   user		= getenv("USER");
   time_start	= 0;
   time_stop	= 0;
   
   chip_addr	= 0 ;		// chip address
   pre_trigs	= 99 ;		// N frames before the triggered
   post_trigs	= 900 ;		// N frames after the triggered
   trig_period	= 10000 ;	// periodic trigger per N frames, old=31250
   trig_cds_x	= 8 ;		// trigger CDS threshold = x * trig_cds
   nreads	= 0 ;		// total frames read
   nsaved	= 0 ;		// total frames saved for processing
   nprocs	= 0 ;		// total frames processed
   nrecords	= 0 ;		// total frames recorded
   ntrigs	= 0 ;		// total frames triggered
   ntrigs_period= 0 ;		// total frames triggered
   ntrigs_cds	= 0 ;		// total frames triggered
   //trig_cds[NROWS][NCOLS];	// CDS noise of each pixel
   for (int i=0; i < nrows; i++) {
      for (int j=0; j < ncols; j++) {
	 trig_cds[i][j] =  0;
	 cds_mean[i][j] = -1;
	 cds_sigma[i][j] = 0;
	 adc_mean[i][j] = -1;
	 adc_sigma[i][j] = 0;
      }
   }
}


// //
// // destructor
// //______________________________________________________________________
// RunInfo::~RunInfo()
// {

// }


//______________________________________________________________________
void RunInfo::Print(Option_t *option) const
{
   (void)(option);
   LOG << "Run Summary" << endl;
   //Print_noise_cds();
   //Print_threshold();
   Dump();
   cout
      << left
      << setw(30) << "Version" << Class_Version()
      << right
      << endl;
   
   if (nreads) {
      cout
	 //<< left
	 << setw(30) << "  nsaved/nreads = " << per_centage((double)nsaved/nreads) << "%"
	 //<< right
	 << endl;
   }
   if (ntrigs) {
      cout
	 << setw(30) << "epsilon = nrecords/nsaved = " << per_centage((double)nrecords/nsaved) << "%" << endl
	 << setw(30) << "trig_ratio = ntrigs/nsaved = " << per_centage((double)ntrigs/nsaved) << "%"
	 << " = " << per_centage((double)ntrigs_period/nsaved) << "% (period)"
	 << " + " << per_centage((double)ntrigs_cds/nsaved) << "% (cds)"
	 << endl
	 << setw(30) << "freq = nsaved/ntrigs = " << (int)((double)nsaved/ntrigs + 0.5) << endl
	 << endl;

   }
}


// CDS thresholds
//______________________________________________________________________
void RunInfo::Print_threshold() const
{
   // restore io state using std::ios::copyfmt()
   std::ios cout_state(nullptr);
   cout_state.copyfmt(std::cout);

   cout << "CDS threshold of each pixel" << endl;
   cout << setw(9) << "col:";
   for (int icol = 0; icol < NCOLS; icol++) {
      cout << setw(6) << icol;
   }
   cout << endl;
   
   for (int irow = 0; irow < NROWS; irow++) {
      cout << setw(6) << "row-" << setfill('0') << setw(2) << irow << ":" << setfill(' ');
      for (int icol = 0; icol < NCOLS; icol++) {
	 cout << setw(6) << setprecision(1) << fixed << trig_cds[irow][icol];
      }
      cout << endl;
   }
   //cout << endl;

   std::cout.copyfmt(cout_state);
}


// CDS noises
//______________________________________________________________________
void RunInfo::Print_noise_cds() const
{
   // restore io state using std::ios::copyfmt()
   std::ios cout_state(nullptr);
   cout_state.copyfmt(std::cout);

   cout << "CDS noise of each pixel (mean/sigma)" << endl;
   cout << setw(9) << "col:";
   for (int icol = 0; icol < NCOLS; icol++) {
      cout << setw(10) << icol;
   }
   cout << endl;
   
   for (int irow = 0; irow < NROWS; irow++) {
      cout << setw(6) << "row-" << setfill('0') << setw(2) << irow << ":" << setfill(' ');
      for (int icol = 0; icol < NCOLS; icol++) {
	 cout << setw(6) << setprecision(1) << fixed
	      << cds_mean[irow][icol] << "/"
	      << cds_sigma[irow][icol]
	    ;
      }
      cout << endl;
   }
   //cout << endl;

   std::cout.copyfmt(cout_state);
}


// ADC noises
//______________________________________________________________________
void RunInfo::Print_noise_adc() const
{
   // restore io state using std::ios::copyfmt()
   std::ios cout_state(nullptr);
   cout_state.copyfmt(std::cout);

   int wmean = 8, wsigma = 4;
   cout << "ADC noise of each pixel (mean/sigma)" << endl;
   cout << setw(9) << "col:";
   for (int icol = 0; icol < NCOLS; icol++) {
      cout << setw(wmean+wsigma+1) << icol;
   }
   cout << endl;
   
   for (int irow = 0; irow < NROWS; irow++) {
      cout << setw(6) << "row-" << setfill('0') << setw(2) << irow << ":" << setfill(' ');
      for (int icol = 0; icol < NCOLS; icol++) {
	 cout << setprecision(1) << fixed
	      << setw(wmean) << adc_mean[irow][icol] << "/"
	      << setw(wsigma) << adc_sigma[irow][icol]
	    ;
      }
      cout << endl;
   }
   //cout << endl;

   std::cout.copyfmt(cout_state);
}


// %F = %Y-%m-%d
// %T = %H:%M:%S
//______________________________________________________________________
void RunInfo::set_time_start()
{
   // raw time
   time( & time_start );

   // convert to readable string
   struct tm *timeinfo;
   char buf[100];

   timeinfo = localtime( & time_start );
   strftime(buf, sizeof(buf), "%F %T", timeinfo);
   time_start_str = buf;

   LOG << time_start_str
       // << " " << time_start_str.size()
       // << " buf: " << sizeof(buf) << " " << buf << " " << string(buf).size()
       << endl;
}

// %F = %Y-%m-%d
// %T = %H:%M:%S
//______________________________________________________________________
void RunInfo::set_time_stop()
{
   // raw time
   time( & time_stop );

   // convert to readable string
   struct tm *timeinfo;
   char buf[100];

   timeinfo = localtime( & time_stop );
   strftime(buf, sizeof(buf), "%F %T", timeinfo);
   time_stop_str = buf;

   LOG << time_stop_str
       // << " " << time_stop_str.size()
       // << " buf: " << sizeof(buf) << " " << buf << " " << string(buf).size()
       << endl;
}
