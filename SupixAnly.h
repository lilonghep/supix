/*******************************************************************//**
 * $Id: SupixAnly.h 1229 2020-07-22 06:13:11Z mwang $
 *
 * analysis class base on skeleton produce by tree->MakeClass("...").
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2019-04-18 23:01:17
 * @copyright:  (c)2019 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#ifndef SupixAnly_h
#define SupixAnly_h

#include "mydefs.h"
#include "RunInfo.h"
#include "SupixTree.h"

#include "TCanvas.h"
#include "TGraph.h"
#include <math.h>
#include <fstream>

#define NWAVE_MAX	1000	// limit frames of a waveform

const int g_npixs_max	= 10;
const int g_cds_max	=  30;
const int g_cds_min	= -120;
const int g_cds_sum_min	= -200;
const int g_adc_max	= 0x10000;
const int g_adc_bin	= 0x100;
const int g_adc_cor_max	=  55;		// ADC normalized
const int g_adc_cor_min	= -55;		// ADC normalized
const double g_cds_sum_x	= 5.0;		// try Npixels with cds > x*sigma
const double g_chi2_cds_x	= NPIXS+sqrt(2.0*NPIXS)*8;	// 8x sigma
const double g_chi2_min	= 800;
const double g_chi2_max	= 1800;

//======================================================================

class TH1D;
class TH2D;
class TProfile;
class TMultiGraph;
class TPaveText;

struct event_t {
   // void set(unsigned long i=0);
   TH2D *	display_cds;	// event display with cds
   TPaveText* 	info;
   TPaveText* 	pixs;
} ;

struct frame_t{
   TH2D*   display_frame;
} ;

class SupixAnly : public SupixTree
{
   event_t m_event;
   frame_t m_iframe; 
public:
   // constructor(s)
   SupixAnly(TTree *tree=0);
   ~SupixAnly();
   
   void event_display(long);

   void frame_display(Long64_t);
   
   event_t get_event(long ievt = -1) {
      if (ievt >= 0)
	 event_display(ievt);
      return m_event;
   }
   
   // for special events
   void special_display(long, int);

   event_t get_special(long ievt =-1, int fired = 1){
   	if(ievt >=0)
		special_display(ievt, fired);
	return m_event;
   }

   frame_t get_frame(Long64_t iframe = -1){
   	if(iframe >=0)
		frame_display(iframe);
	return m_iframe;
   }
   // retrieve RunInfo
   virtual RunInfo * get_RunInfo();

   // book histograms
   virtual void Book();

   // write non-histogram objects
   virtual void Write();
   
   // fill histograms
   void Loop(Long64_t nentries);

   // why can't it be overridden? -- NOT virtual inheritance!
   void Loop();

   void set_nwaves_max(int x)	{ m_nwaves_max = x; }
   void build_waveform(Long64_t nentries=0, int row=-1, int col=-1);
   void fill_waveform();
   void write_waveform();

   bool true_trig(trig_t x)		// true triged frame
   { return x > 0 && x < TRIG_PRE; }
   //{ return x > 1 && x < TRIG_PRE; }	// excluding pure periodic trig

   void scan(int row=0, int col=0, const char* cut="");

   double charge_correction(double charge_seed, double charge_raw);
   
private:
   TCanvas *	m_c1;
   RunInfo *	m_runinfo;
   int		m_runinfo_version;

   // histograms
   TList *	m_list_adc;
   TList *	m_list_cds;
   TH1D *	m_adc[NROWS][NCOLS];
   TH1D *	m_cds[NROWS][NCOLS];

   TList *	m_list_misc;
   TH1D *	m_trig;
   TH1D *	m_fid;
   TH1D *	m_npixs;	// of non-periodic trigs
   TH1D *	m_chi2_cds;	// CDS chi2 per frame
   TH1D *	m_chi2_trig;	// chi2 of triged frame
   TH1D *	m_dTevt;	// time different between consecutive events

   TH1D *	m_trig_cds;	// cds of triged pixels
   TH1D *	m_trig_cds_sum;	// cds sum of triged pixels
   TH2D *	m_trig_cds_npixs;	// cds sum vs npixs
   TH2D *	m_hitmap_cds;	// hit-map of pixels with |CDS-mean| > sigma * cds_x
   TH1D *	m_cds_raw;	// raw
   TH1D *	m_cds_cor;	// noise corrected
   TProfile *	m_cds_frame_raw;
   TProfile *	m_cds_frame_cor;

   TH1D *	m_adc_raw;
   TH1D *	m_adc_cor;
   TProfile *	m_adc_frame_raw;
   TProfile *	m_adc_frame_cor;

   //pixel  baseline 
   TH1D* m_hbaseline[16];

   //cluster analysis --LongLI
   
	//seed pixel
   TList* 	m_list_seed;			//seed
   TH1D* 	m_seed_single;
	TH1D*		m_seed_snr;
	TProfile*	m_pix_prf[4];
   TProfile* 	m_pix2_correct;

  //cluster shape in the 5x5 matrix  
   TList*	m_list_shape;
   TH2D*	m_cluster_shape[25];

 //correction 
   TList*	m_list_correction;
   TProfile*	m_prf_matrix[5][5];
   TProfile*	m_prf_matrix_crt[5][5];


   // Sum with various size (CDS_raw, CDS_crt, SNR_raw, SNR_crt)
   TList*		m_list_matrix;
   TProfile*	m_matrix_prf;
	TProfile* 	m_matrix_prf_s;
	TProfile*	m_matrix_pnt_prf;
	TProfile*	m_matrix_pnt_prf_s;
	TProfile*	m_mat_pix_prf;
	TH1D*		m_sum_raw[3];
   TH1D*		m_sum_crt[3];
   TH1D*		m_snr_raw[3];
   TH1D*		m_snr_crt[3];


   // cluster reconstruction algorithm
   TList*		m_list_cluster_rec;
   TH2D*		m_snr_map;
   TProfile*	m_snr_prf;   
   TH1D*		m_pix_sum[7];
   TH1D*		m_clu_size[7];
   TH1D*		m_distance[7];
	TProfile*	m_sum_fixed[7];
	TProfile*	m_snr_fixed[7];
	TH1D*			m_tot_fixed[7];
	TH1D*			m_noise_fixed[7];
	TH1D*			m_hist_snr_fixed[7];
	TH1D*			m_size_fixed[7];

	TProfile*		m_clst_q[2];
   TProfile*		m_clst_snr[3];
   
   TList*		m_list_cluster_opt;
   TH1D*			m_sum_opt[7];
   TH1D* 		m_snr_opt[7];
   TH1D* 		m_size_opt[7];
   TH1D*			m_dist_opt[7];
   TH1D* 		m_snrdis_opt[7];
   
   TList*		m_list_cluster_alg;
   TProfile*	m_sum_alg[20];	 
   TProfile*	m_snr_alg[20];	 
   TH1D*		m_pxs_left[20];
   TH1D* 		m_tot_alg[20];
	TH1D*			m_noise_alg[20];
   TH1D*		m_size_alg[20];
   TH1D* 		m_dist_alg[20];
   TH1D*		m_snr_clu[20];
   TProfile*	m_sum_extr;
   TProfile*	m_snr_extr;
	TProfile*	m_size_thres;
	TH1D* 		m_seed_rat[20];


	TList* 	m_list_cluster_anal;
	TH1D*		m_sum_size[20][8];
	TH1D*		m_snr_size[20][8];
	TProfile*	m_sum_size_prf;


	TList* 	m_list_special_evt;
	TH2D* 	m_special_evt;

	TList* 	m_list_multievt;
	TH1D*		m_evt_num;
	TH1D*		m_sum_multi[3];	
	TH1D*		m_size_multi[3];	
	TH1D*		m_sumratio_multi;
	TH2D* 	m_c1_c2;

//   TH1D*		m_snr_max[10];
//   TH1D*		m_size_max[10];
//   TH1D*		m_dist_max[10];
//   TH1D*		m_sum_max[10];
//   TH1D*		m_sum_size[5];

	TH1D*			m_thres_cut;

//	TList* 		m_list_snr_pix;
//	TH1D*			m_snr_pix[FRAME_MAX];
//	TH1D* 		m_snr_single[FRAME_MAX];
//	TH1D* 		m_ratio_pix[FRAME_MAX];	

	TList*		m_list_cut;
	TH1D*			m_c_snr_s[5];
	TH1D*			m_c_snr_r[5];
	TH1D*			m_s_snr[5];
	TH1D*			m_ratio[5];

	TH1D*			m_snr_matrix;
	TH1D*			m_snr_matrix_raw;
	TH1D*			m_snr_matrix_cor;
	TH1D*			m_snr_matrix_alg;
	TH1D*			m_cds_matrix_raw;
	TH1D*			m_cds_matrix_cor;
	
	TH1D* 		m_snr_all9;
	TH1D*			m_snr_fio;
	TH1D* 		m_snr_fio_raw;
	TH2D* 		m_last_9;
	TH2D*			m_last_9_raw;
	TH1D*			m_ratio_last_9;
	TH1D*			m_cut_3;
	TH1D*			m_cut3_mu;
	
	TH1D*			m_cut3_ratio2;
	TH1D*			m_cut3_mu1[5];
	
	TProfile*	m_rn_prf;
	TProfile*	m_cn_prf;
	TH1D*			m_cnrn_snr;

	TList*		m_list_cluster_multicut;
	TH1D*			m_cluster_multicut[4];	
	TH1D*			m_fixed_sum;


	//  cluster reconstructed final	
	TList* 		m_list_cluster_final;
	TH1D*			m_cluster_final[6];	
	TH1D*			m_cluster_final_raw[6];	
	
	// Correction estimation 
	TList* 		m_list_correction_estimation;
	TH1D*			m_err_ratio;
	TH1D*			m_cor_dis;
	TH1D*			m_nor_dis;
	TH1D*			m_pix_p;
	TH1D* 		m_pix_l;
	TH1D* 		m_pix_r;
	TProfile* 	m_clu_sum_prf;	
	TProfile*	m_pix_nor_prf;
	TProfile*	m_pix_cor_prf;
	TProfile*   m_pix_diff_ratio_prf;
	TProfile*	m_pix_diff_ratio_s_prf;
	
	
	TList* 		m_list_fake_hit;
//	TProfile*	m_rate_per_frame;
	TH1D*			m_snr_allpix;
	TH1D*			m_fake_frames;
	TH1D*			m_snr_def;
	TH1D*			m_fake_frames_def;
	TH1D*			m_frame_num;
	//	TH1D*			m_rate_per_frame_thr[20];


	TList* 		m_list_spatial_resolution;
	TH1D*			h_pos_x;
	TH1D*			h_pos_y;
	TH1D*			h_posx_w;
	TH1D*			h_posy_w;
	
	TList* 		m_list_hitmap_thres;
	TH2D*			m_hitmap_threshold[6];
	TH1D*			m_abnormal_row_pixel;
	TH1D*			m_normal_row_pixel;
	TH1D* 		m_normal_2_row;
	
	TH1D*			m_pix_r0;
	TH1D*			m_pix_r1;
	TH1D* 		m_pix_r2;
	TH1D*			m_pix_r3;
	
	TList* 		m_list_pix_snr;
	TH1D*			m_pix_snr;

	TList*		m_list_cluster_size;
	TH1D*			m_size_anal[4];	

	TH1D*			m_shape_size3;
	TH1D*			m_shape_size4;

	
	TList*		m_list_hit;
	TH1D*			m_hit_pos_x;   // x for row
	TH1D*			m_hit_pos_y;	// y for col

	TH1D*			m_clu_pos_x;
	TH1D*			m_clu_pos_y;



	TH1D*			m_pos_x[4];
	TH1D*			m_pos_y[4];

	TH2D* 		m_hit_pos_map[4];

	TH2D*			m_hit_pos_tot;
	TH2D*			m_hit_prf_size[4];
	TH1D*			m_hit_q_seed[4];
	TH1D*			m_hit_q_adj[4];

	TH2D*			m_hit_prf;
	TH1D*			m_hit_q;

	TH2D*			m_hit_clu;

	bool			m_booked;

   // waveform analysis
   //----------------------------------------
   int		m_nwaves_max;	// max number of waves
   int		m_nwaves;
   int		m_pre_trigs;
   int		m_post_trigs;
   int		m_nwave_max;	// max Nframes per waveform
   int		m_nwave_frames;	// frame index of a waveform
   int		m_exceedings;
   int		m_nwave_trig;	// offset of first triged frame
   int		m_nwave_norm;	// normal waveform length
   int		m_npixs_trig;	// npixs of triged frame
   ULong64_t	m_frame_start;	// start frame of a waveform
   short	m_wave_trig;	// trig of a waveform
   
   // waveform of single pixels
   bool		m_continuous;	// continuous daq mode
   bool		m_pix_fired;
   int		m_pix_row;
   int		m_pix_col;

   TList *	m_list_wave_adc;
   TMultiGraph* m_mg_adc_pixs;
   double *	m_wave_adc_pixs;

   TList *	m_list_wave_cds;
   TMultiGraph* m_mg_cds_pixs;
   double *	m_wave_cds_pixs;

   TMultiGraph* m_mg_adc_frame;
   double *	m_wave_adc_frame;	// waveform of sum adc of a frame

   TMultiGraph* m_mg_cds_trig;
   double *	m_wave_cds_trig;	// cds of single pixel

   TProfile *	m_waveform_cor;		// noise corrected cds summed of all pixels per frame

   TMultiGraph* m_mg_cds_frame;		// waveforms container
   double *	m_wave_cds_frame;	// sum of cds per frame

   TMultiGraph* m_mg_chi2_cds;
   double *	m_wave_chi2_cds;	// cds of single pixel

};

#endif //~ SupixAnly_h
