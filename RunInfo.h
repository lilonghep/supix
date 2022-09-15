/*******************************************************************//**
 * $Id: RunInfo.h 1219 2020-07-15 08:56:47Z mwang $
 *
 * run information to be attached to a tree via GetUserInfo().
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2019-04-24 13:57:03
 * @copyright:  (c)2019 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#ifndef RunInfo_h
#define RunInfo_h

#include "mydefs.h"
#include "TObject.h"
#include <time.h>

// run information
//______________________________________________________________________
class RunInfo : public TObject {
public:
   std::string	user;		// who took data
   time_t	time_start;	// Run start time in second
   time_t	time_stop;	// Run stop time in second
   std::string	time_start_str;	// Run start time string
   std::string	time_stop_str;	// Run stop time string

   DAQ_MODE_t	daq_mode;	// DAQ mode: 0=normal, 1=noise, 2=continuous
   unsigned short	nrows;	// N rows
   unsigned short	ncols;	// N rows
   int	chip_addr;		// chip address
   int	pre_trigs;		// N frames before the triggered
   int	post_trigs;		// N frames after the triggered
   int	trig_period;		// periodic trigger per N frames
   unsigned long nreads;	// total frames read
   unsigned long nsaved;	// total frames saved for processing
   unsigned long nprocs;	// total frames processed => nsaved
   unsigned long nrecords;	// total frames recorded
   unsigned long ntrigs;	// total frames triggered
   unsigned long ntrigs_period;	// total frames triggered by periodic
   unsigned long ntrigs_cds;	// total frames triggered by CDS
   double  trig_cds_x;		// trigger CDS threshold = x * trig_cds
   double  trig_cds[NROWS][NCOLS];	// CDS threshold of each pixel = mean - sigma * x
   double cds_mean[NROWS][NCOLS];	// CDS noise mean of each pixel
   double cds_sigma[NROWS][NCOLS];	// CDS noise sigma of each pixel
   double adc_mean[NROWS][NCOLS];	// ADC noise mean of each pixel
   double adc_sigma[NROWS][NCOLS];	// ADC noise sigma of each pixel

   // must have a default constructor or an I/O constructor
   RunInfo();

   void Print(Option_t *option="") const;	// TObject protocal
   // void print(Option_t *option="") const
   // { Print(option); }

   void Print_noise_cds() const;
   void Print_noise_adc() const;
   void Print_threshold() const;
   
   void set_time_start();
   void set_time_stop();

   // run start time

   // at the end
   // version
   //     1 : non-thread
   //     2 : thread
   //     3 : CDS noise arrays
   //     4 : ADC noise arrays
   //     5 : daq_mode
   //     6 : trig_cds[_x] changd to double
   ClassDef(RunInfo, 6);
};

#endif //~ RunInfo_h
