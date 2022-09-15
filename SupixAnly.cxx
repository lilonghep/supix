/*******************************************************************//**
 * $Id: SupixAnly.cxx 1235 2020-07-25 07:26:28Z mwang $
 *
 * class definition.
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2019-04-18 23:05:16
 * @copyright:  (c)2019 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "SupixAnly.h"
#include "Timer.h"

#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TStyle.h>
#include <TPaveStats.h>
#include <TMultiGraph.h>
#include <TGraph.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
using namespace std;
#define ALLPIXS			// control pixel-wise histograms
#define MAXGRAPHS	100	// max saved graphs

//
// constructor(s)
//______________________________________________________________________
SupixAnly::SupixAnly(TTree *tree)
   : SupixTree(tree)
{
   TRACE;
   // initialize
   m_nwaves_max = 10;
   
   for (int i=0; i < NROWS; i++) {
      for (int j=0; j < NCOLS; j++) {
	 m_adc[i][j] = 0;
	 m_cds[i][j] = 0;
      }
   }

   // LOG << fChain << endl;
   get_RunInfo();
   if (m_runinfo) {
      m_pre_trigs = m_runinfo->pre_trigs;
      m_post_trigs = m_runinfo->post_trigs;
      m_nwave_norm = m_pre_trigs + 1 + m_post_trigs;
      m_nwave_max = 2 * m_nwave_norm;
      if (m_nwave_max > NWAVE_MAX) {
	 cout << "TOO large waveform!!!" << endl;
	 m_nwave_max = NWAVE_MAX;
      }
   }
   else {
      m_pre_trigs = 0;
      m_nwave_norm = 10;	// default in RunInfo
      m_nwave_max = NWAVE_MAX;
   }

   // int daq_mode = m_runinfo->daq_mode;
   // LOG << "daq_mode=" << m_runinfo->daq_mode << " " << M_CONTINUOUS << endl;
   if (m_runinfo_version >= 5 && m_runinfo->daq_mode == M_CONTINUOUS) {
      m_continuous = true;
      LOG << "daq_mode=" << M_CONTINUOUS << " CONTINUOUS" << endl;
      m_nwave_max = NWAVE_MAX;
   }
   else {
      m_continuous = false;
   }
   
   m_npixs_trig = 0;
   m_frame_start = 0;
   m_wave_trig = 0;
   
   m_c1 = 0;
   m_booked = false;
}


//
// destructor
//______________________________________________________________________
SupixAnly::~SupixAnly()
{
   // for (int i=0; i < NCOLS; i++) {
   //    delete m_mg_adc_pixs[i];
   // }
   delete []m_mg_adc_pixs;
   delete []m_wave_adc_pixs;
   delete []m_wave_adc_frame;
   delete []m_mg_cds_pixs;
   delete []m_wave_cds_pixs;
   delete []m_wave_cds_trig;
   delete []m_wave_cds_frame;
   delete []m_wave_chi2_cds;

}

/***********************************************************************/

//______________________________________________________________________
void SupixAnly::scan(int row, int col, const char* cut)
{
   ostringstream oss;
   oss << "frame:fid:trig:npixs:pixid[0]"
       << ":pixel_adc[" << row << "][" << col << "]"
       << ":pixel_cds[" << row << "][" << col << "]"
      ;
   fChain->Scan(oss.str().c_str(), cut);
}


// retrieve RunInfo
//______________________________________________________________________
RunInfo * SupixAnly::get_RunInfo()
{
   TRACE;
   // must have a Tree first
   Long64_t jentry = fChain->GetEntries() - 1;	// global
   Long64_t ientry = fChain->LoadTree(jentry);	// last tree in chain
   if (ientry < 0) {
      LOG << "ERROR: LoadTree(" << jentry << ") !!!" << endl;
      exit(-1);
   }

   m_runinfo = (RunInfo*)fChain->GetTree()->GetUserInfo()->At(0);
   m_runinfo_version = m_runinfo->Class_Version();
   cout << "RunInfo version=" << m_runinfo_version << endl;
   if (! m_runinfo) {
      LOG << "NO RunInfo object!!!" << endl;
   }

   return m_runinfo;
}


// book histograms
//______________________________________________________________________
void SupixAnly::Book()
{
   TRACE;
   
   ostringstream oss;
   string hname, htitle;
   int xbins, ybins;
   double xmin, xmax, ymin, ymax;

   TH1D* h1d;
   TProfile* prf;
   TH2D* h2d;
   
#ifdef ALLPIXS
   m_list_adc = new TList;
   m_list_cds = new TList;
   for (int i=0; i < NROWS; i++) {
      for (int j=0; j < NCOLS; j++) {
	 // CDS
	 oss.str("");
	 oss << "cds_" << i << "_" << j;
	 hname = oss.str();
	 oss.str("");
	 oss << "CDS[" << i << "][" << j << "];CDS";
	 htitle = oss.str();
	 h1d = (TH1D*)gDirectory->Get(hname.c_str() );
	 if (h1d) delete h1d;
	 xmin = -30, xmax = 300;
	 xbins = xmax - xmin;
	 h1d = new TH1D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax);
	 m_cds[i][j] = h1d;
	 m_list_cds->Add(h1d);

	 // ADC
	 oss.str("");
	 oss << "adc_" << i << "_" << j;
	 hname = oss.str();
	 oss.str("");
	 oss << "ADC[" << i << "][" << j << "];ADC";
	 htitle = oss.str();
	 h1d = (TH1D*)gDirectory->Get(hname.c_str() );
	 if (h1d) delete h1d;
	 xmin = g_adc_cor_min, xmax = g_adc_cor_max;
	 xbins = xmax - xmin;
	 h1d = new TH1D(hname.c_str(), htitle.c_str(), 70000, 0, 70000);
	 m_adc[i][j] = h1d;
	 m_list_adc->Add(h1d);
      }
   }
#endif

   m_list_misc = new TList;

   // trig pattern
   xmin = 0, xmax = 4;
   xbins = xmax - xmin;
   hname = "trig";
   htitle = "trigger pattern;trigger";
   h1d = (TH1D*)gDirectory->Get(hname.c_str() );
   if (h1d) delete h1d;
   h1d = new TH1D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax);
   m_trig = h1d;
   m_list_misc->Add(h1d);
   
   // fid
   xmin = 0, xmax = 16;
   xbins = xmax - xmin;
   hname = "fid";
   htitle = "frames with non-periodic fired pixels;frame id";
   h1d = (TH1D*)gDirectory->Get(hname.c_str() );
   if (h1d) delete h1d;
   h1d = new TH1D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax);
   m_fid = h1d;
   m_list_misc->Add(h1d);

   // npixs
   xmin = 0, xmax = g_npixs_max;
   xbins = xmax - xmin;
   hname = "npixs";
   htitle = "Npixels of fired frame;N_{pixels}";
   h1d = (TH1D*)gDirectory->Get(hname.c_str() );
   if (h1d) delete h1d;
   h1d = new TH1D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax);
   m_npixs = h1d;
   m_list_misc->Add(h1d);

   // chi2_cds
   xmin = g_chi2_min, xmax = g_chi2_max;
   xbins = xmax - xmin;
   hname = "chi2_cds";
   htitle = "chi2 of non-triged events;#chi^{2}_{cds};Entries / 1";
   h1d = (TH1D*)gDirectory->Get(hname.c_str() );
   if (h1d) delete h1d;
   h1d = new TH1D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax);
   m_chi2_cds = h1d;
   m_list_misc->Add(h1d);

   // chi2_trig
   xmin = g_chi2_min, xmax = g_chi2_max;
   xbins = xmax - xmin;
   hname = "chi2_trig";
   htitle = "chi2 of triged events;#chi^{2}_{trig};Entries / 1";
   h1d = (TH1D*)gDirectory->Get(hname.c_str() );
   if (h1d) delete h1d;
   h1d = new TH1D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax);
   m_chi2_trig = h1d;
   m_list_misc->Add(h1d);

   // chi2_trig
   xmin = 0, xmax = 1000;
   xbins = xmax - xmin;
   hname = "dTevt";
   htitle = "time difference between consecutive events;#DeltaT_{evt};Entries / 1";
   h1d = (TH1D*)gDirectory->Get(hname.c_str() );
   if (h1d) delete h1d;
   h1d = new TH1D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax);
   m_dTevt = h1d;
   m_list_misc->Add(h1d);

   // hitmap
   xmin = 0, xmax = NCOLS;
   xbins = xmax - xmin;
   ymin = 0, ymax = NROWS;
   ybins = ymax - ymin;
   hname = "hitmap_cds";
   oss.str("");
   oss << "hitmap of fired pixels;ROW;COL";
   htitle = oss.str();
   h2d = (TH2D*)gDirectory->Get(hname.c_str() );
   if (h2d) delete h2d;
   h2d = new TH2D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax, ybins, ymin, ymax);
   m_hitmap_cds = h2d;
   m_list_misc->Add(h2d);

   // trig_cds
   xmin = g_cds_min*2, xmax = 0;
   xbins = xmax - xmin;
   hname = "trig_cds";
   htitle = "CDS of fired pixels;CDS"; //normalized";
   h1d = (TH1D*)gDirectory->Get(hname.c_str() );
   if (h1d) delete h1d;
   h1d = new TH1D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax);
   m_trig_cds = h1d;
   m_list_misc->Add(h1d);

   // trig_cds_sum
   xmin = g_cds_sum_min, xmax = 0;
   xbins = xmax - xmin;
   hname = "trig_cds_sum";
   htitle = "sum CDS of fired pixels;sum CDS_{fired}";
   h1d = (TH1D*)gDirectory->Get(hname.c_str() );
   if (h1d) delete h1d;
   h1d = new TH1D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax);
   m_trig_cds_sum = h1d;
   m_list_misc->Add(h1d);

   // trig_cds_npixs
   xmin = g_cds_sum_min, xmax = 0;
   xbins = xmax - xmin;
   ymin = 0, ymax = g_npixs_max;
   ybins = ymax - ymin;
   hname = "trig_cds_npixs";
   oss.str("");
   oss << "sum CDS vs Npixels;sum CDS_{fired};N_{pixel}";
   htitle = oss.str();
   h2d = (TH2D*)gDirectory->Get(hname.c_str() );
   if (h2d) delete h2d;
   h2d = new TH2D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax, ybins, ymin, ymax);
   m_trig_cds_npixs = h2d;
   m_list_misc->Add(h2d);

   // cds_raw
   xmin = g_cds_min, xmax = g_cds_max;
   xbins = xmax - xmin;
   hname = "cds_raw";
   htitle = "CDS raw of all pixels;CDS raw";
   h1d = (TH1D*)gDirectory->Get(hname.c_str() );
   if (h1d) delete h1d;
   h1d = new TH1D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax);
   m_cds_raw = h1d;
   m_list_misc->Add(h1d);

   // cds_cor
   xmin = g_cds_min, xmax = g_cds_max;
   xbins = xmax - xmin;
   hname = "cds_cor";
   htitle = "corrected CDS of all pixels;CDS normalized";
   h1d = (TH1D*)gDirectory->Get(hname.c_str() );
   if (h1d) delete h1d;
   h1d = new TH1D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax);
   m_cds_cor = h1d;
   m_list_misc->Add(h1d);

   // cds_frame_raw
  // xmin = 0, xmax = NPIXS;
   xmin = 512, xmax = NPIXS;   //for half matrix LongLI 2021-10-23
   xbins = xmax - xmin;
   ymin = g_cds_min, ymax = g_cds_max;
   hname = "cds_frame_raw";
   htitle = "CDS raw of frame pixels;64#timesCOL + ROW;CDS raw";
   prf = (TProfile*)gDirectory->Get(hname.c_str() );
   if (prf) delete prf;
   prf = new TProfile(hname.c_str(), htitle.c_str(), xbins, xmin, xmax, ymin, ymax);
   m_cds_frame_raw = prf;
   m_list_misc->Add(prf);

   // cds_frame_cor
  // xmin = 0, xmax = NPIXS;
   xmin = 512, xmax = NPIXS;   //for half matrix LongLI 2021-10-23
   xbins = xmax - xmin;
   ymin = g_cds_min, ymax = g_cds_max;
   hname = "cds_frame_cor";
   htitle = "corrected CDS of frame pixels;64#timesCOL + ROW;CDS correlated";
   prf = (TProfile*)gDirectory->Get(hname.c_str() );
   if (prf) delete prf;
   prf = new TProfile(hname.c_str(), htitle.c_str(), xbins, xmin, xmax, ymin, ymax);
   m_cds_frame_cor = prf;
   m_list_misc->Add(prf);
   
   // adc_raw
   xmin = 0, xmax = 0x10000;
   xbins = 0x200;	// 512
   hname = "adc_raw";
   htitle = "ADC raw of all pixels;ADC";
   h1d = (TH1D*)gDirectory->Get(hname.c_str() );
   if (h1d) delete h1d;
   h1d = new TH1D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax);
   m_adc_raw = h1d;
   m_list_misc->Add(h1d);

   // adc_cor
   xmin = g_adc_cor_min, xmax = g_adc_cor_max;
   xbins = xmax - xmin;
   hname = "adc_cor";
   htitle = "corrected ADC of all pixels;ADC normalized";
   h1d = (TH1D*)gDirectory->Get(hname.c_str() );
   if (h1d) delete h1d;
   h1d = new TH1D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax);
   m_adc_cor = h1d;
   m_list_misc->Add(h1d);

   // adc_frame_raw
  // xmin = 0, xmax = NPIXS;
   xmin = 512, xmax = NPIXS;   //for half matrix LongLI 2021-10-23
   xbins = xmax - xmin;
   ymin = 0, ymax = g_adc_max;
   hname = "adc_frame_raw";
   htitle = "ADC raw of frame pixels;64#timesCOL + ROW;ADC raw";
   prf = (TProfile*)gDirectory->Get(hname.c_str() );
   if (prf) delete prf;
   prf = new TProfile(hname.c_str(), htitle.c_str(), xbins, xmin, xmax, ymin, ymax);
   m_adc_frame_raw = prf;
   m_list_misc->Add(prf);

   // adc_frame_cor
  // xmin = 0, xmax = NPIXS;
   xmin = 512, xmax = NPIXS;   //for half matrix LongLI 2021-10-23
   xbins = xmax - xmin;
   ymin = g_adc_cor_min, ymax = g_adc_cor_max;
   hname = "adc_frame_cor";
   htitle = "corrected ADC of frame pixels;64#timesCOL + ROW;ADC correlated";
   prf = (TProfile*)gDirectory->Get(hname.c_str() );
   if (prf) delete prf;
   prf = new TProfile(hname.c_str(), htitle.c_str(), xbins, xmin, xmax, ymin, ymax);
   m_adc_frame_cor = prf;
   m_list_misc->Add(prf);

   // waveform_cor
   xmin = -m_pre_trigs, xmax = m_nwave_max - m_pre_trigs;
   xbins = xmax - xmin;
   ymin = g_cds_min, ymax = g_cds_max;
   hname = "waveform_cor";
   htitle = "waveform of corrected CDS;frame;CDS normalized";
   prf = (TProfile*)gDirectory->Get(hname.c_str() );
   if (prf) delete prf;
   prf = new TProfile(hname.c_str(), htitle.c_str(), xbins, xmin, xmax, ymin, ymax);
   m_waveform_cor = prf;
   m_list_misc->Add(prf);

   m_mg_adc_frame = new TMultiGraph("wave_adc_frame", ";frame;sum ADC of all pixels");

   // all pixels
   m_list_wave_adc = new TList;
   m_mg_adc_pixs = new TMultiGraph[NPIXS];
   m_list_wave_cds = new TList;
   m_mg_cds_pixs = new TMultiGraph[NPIXS];
   TMultiGraph* mg;
   for (int i=0; i < NROWS; i++) {
      for (int j=0; j < NCOLS; j++) {
	 mg = &(*(m_mg_adc_pixs +j+i*NCOLS) );
	 m_list_wave_adc->Add(mg);
	 oss.str("");
	 oss << "wave_adc_pix_r" << i << "_c" << j;
	 mg->SetName(oss.str().c_str() );
	 mg->SetTitle("ADC waveform of a pixel;frame;ADC");
	 
	 mg = &(*(m_mg_cds_pixs +j+i*NCOLS) );
	 m_list_wave_cds->Add(mg);
	 oss.str("");
	 oss << "wave_cds_pix_r" << i << "_c" << j;
	 mg->SetName(oss.str().c_str() );
	 mg->SetTitle("CDS waveform of a pixel;frame;CDS");
      }
   }
   
   m_mg_cds_trig = new TMultiGraph("wave_cds_trig", ";frame;sum CDS of fired pixels");
   m_mg_cds_frame = new TMultiGraph("wave_cds_frame", ";frame;sum CDS of all pixels");
   m_mg_chi2_cds = new TMultiGraph("wave_chi2_cds", ";frame;#chi^{2}_{CDS}");

   // arrays for waveforms
   m_wave_adc_pixs	= new double[NWAVE_MAX*NPIXS];
   m_wave_adc_frame	= new double[NWAVE_MAX];
   m_wave_cds_pixs	= new double[NWAVE_MAX*NPIXS];
   m_wave_cds_trig	= new double[NWAVE_MAX];
   m_wave_cds_frame	= new double[NWAVE_MAX];
   m_wave_chi2_cds	= new double[NWAVE_MAX];
   
   // cluster analysis --LongLI
   //cluster with 5x5
   
   //output around the seed 
   m_list_seed = new TList;  
   TH1D* hseed;
   oss.str("");
   oss<<"seed_pixel";
   hname = oss.str();
   oss.str("");
   oss<<"Seed pixel output";
   htitle = oss.str();
   hseed = (TH1D*) gDirectory -> Get(hname.c_str());
   if(hseed) delete hseed;
   hseed = new TH1D(hname.c_str(), htitle.c_str(), 320, -20 , 300);
   m_seed_single = hseed;
   m_list_seed ->Add(hseed);
  
	oss.str("");
	oss<<"seed_snr";
	hname = oss.str();
	oss.str("");
	oss<<"SNR of seed pixel;SNR;Entries";
	htitle = oss.str();
	hseed = new TH1D(hname.c_str(), htitle.c_str(), 250, -50, 200);
	m_seed_snr = hseed;
	m_list_seed->Add(hseed);



   TProfile* prf_ab;
   for(int i=0; i<4; i++){
	oss.str("");
	oss<<"pix_prf_"<<i+1;
	hname = oss.str();
	oss.str("");
	oss<<"Output of Pixel_"<<i+1<<" vs. CDS_{seed};CDS_{seed};CDS";
	htitle = oss.str();
	prf_ab = (TProfile*) gDirectory -> Get(hname.c_str());
	if(prf_ab) delete prf_ab;

	prf_ab = new TProfile(hname.c_str(), htitle.c_str(), 150, 0, 300, -5, 25);
	m_pix_prf[i] = prf_ab;
	m_list_seed ->Add(prf_ab);
	}
	
	prf_ab = new TProfile("pix_2_correct", "Output of pix_2 vs. CDS_{seed}(corrected);CDS_{seed};CDS",
			150, 0, 300, -5, 25);
	m_pix2_correct = prf_ab;
	m_list_seed ->Add(prf_ab);


	//shape of the cluster
	m_list_shape = new TList;
	TH2D* h2d_shape;
	for(int i = 0; i< 25; i++){
	oss.str("");
	oss<<"shape_of_"<<i+1<<"_pixel";
	hname = oss.str();
	oss.str("");
	oss<<"Shape of the "<<i+1<<"significant pixel(s);COL;ROW";
	htitle = oss.str();
	h2d_shape = (TH2D*) gDirectory -> Get(hname.c_str());
	if(h2d_shape) delete h2d_shape;
	h2d_shape = new TH2D(hname.c_str(), htitle.c_str(), 5, 0, 5, 5, 0, 5);
	m_cluster_shape[i] = h2d_shape;
	m_list_shape ->Add(h2d_shape);
	}

	//pixel output in 5x5 matrix
	m_list_correction = new TList;
	TH1D* hmatrix;
	for(int i =0; i<5; i++){
		for(int j = 0; j<5; j++){
		oss.str("");
		oss<<"pix_prf["<<i+1<<"]["<<j+1<<"]";
		hname = oss.str();
		oss.str("");
		oss<<"Output of Pixel["<<i+1<<"]["<<j+1<<"] vs. CDS_{seed};CDS_{seed};CDS";
		htitle = oss.str();
		prf_ab = (TProfile*) gDirectory -> Get(hname.c_str());
		if(prf_ab) delete prf_ab;

		prf_ab = new TProfile(hname.c_str(), htitle.c_str(), 300, 0, 300, -5, 25);
		m_prf_matrix[i][j] = prf_ab;
		m_list_correction->Add(prf_ab);
	



		oss.str("");
		oss<<"prf["<<i+1<<"]["<<j+1<<"]_crt";
		hname = oss.str();
		oss.str("");
		oss<<"Output of Pixel["<<i+1<<"]["<<j+1<<"] (corrected)vs. CDS_{seed};CDS_{seed};CDS";
		htitle = oss.str();
		prf_ab = (TProfile*) gDirectory -> Get(hname.c_str());
		if(prf_ab) delete prf_ab;

		prf_ab = new TProfile(hname.c_str(), htitle.c_str(), 300, 0, 300, -5, 25);
		m_prf_matrix_crt[i][j] = prf_ab;
		m_list_correction->Add(prf_ab);
		}
	}
	


	
	m_list_matrix = new TList;
	
	prf = new TProfile("matrix_prf", "Cluster ADC vs. number of pixels;N_{pixel} in matrix;ADC", 25, 0.5, 25.5, 0, 300);
	m_matrix_prf = prf;
	m_list_matrix->Add(prf);

	prf = new TProfile("matrix_prf_s", "Cluster ADC vs. number of pixels;N_{pixel} in matrix;ADC", 25, 0.5, 25.5, 0, 300, "s");
	m_matrix_prf_s = prf;
	m_list_matrix->Add(prf);
	
	prf = new TProfile("matrix_pnt_prf", "cluster N / cluster 25; N_{pixel} in matrix; [%]", 25, 0.5, 25.5, 0, 2);
	m_matrix_pnt_prf = prf;
	m_list_matrix->Add(prf);
	
	prf = new TProfile("matrix_pnt_prf_s", "cluster N / cluster 25; N_{pixel} in matrix; [%]", 25, 0.5, 25.5, 0, 2, "s");
	m_matrix_pnt_prf_s = prf;
	m_list_matrix->Add(prf);

	prf = new TProfile("mat_pix_prf", "Output of pixels in matrix after descending order;Pixel Number; ADC", 25, 0.5, 25.5, -50, 250, "s");
	m_mat_pix_prf = prf;
	m_list_matrix->Add(prf);


	TH1D* h_cds;
	for(int i=0; i<3; i++){
		oss.str("");
		oss<<"cds_"<<2*i+1<<"_raw";
		hname = oss.str();
		oss.str("");
		oss<<2*i+1<<"x"<<2*i+1<<" matrix with CDS_{raw}";
		htitle = oss.str();
		h_cds = new TH1D(hname.c_str(), htitle.c_str(), 320, -20, 300);
		m_sum_raw[i] = h_cds;
		m_list_matrix ->Add(h_cds);

		
		oss.str("");
		oss<<"snr_"<<2*i+1<<"_raw";
		hname = oss.str();
		oss.str("");
		oss<<2*i+1<<"x"<<2*i+1<<" matrix with SNR_{raw}";
		htitle = oss.str();
		h_cds = new TH1D(hname.c_str(), htitle.c_str(), 320, -20, 300);
		m_snr_raw[i] = h_cds;
		m_list_matrix->Add(h_cds);
	


		oss.str("");
		oss<<"cds_"<<2*i+1<<"_crt";
		hname = oss.str();
		oss.str("");
		oss<<2*i+1<<"x"<<2*i+1<<" maxtrix with CDS_{crt}";
		htitle = oss.str();
		hmatrix = (TH1D*) gDirectory -> Get(hname.c_str());
		if(hmatrix) delete hmatrix;

		hmatrix = new TH1D(hname.c_str(), htitle.c_str(), 300, 0, 300);
		m_sum_crt[i] = hmatrix;
		m_list_matrix->Add(hmatrix);
	

		oss.str("");
		oss<<"snr_"<<2*i+1<<"_crt";
		hname = oss.str();
		oss.str("");
		oss<<2*i+1<<"x"<<2*i+1<<" matrix with SNR_{crt}";
		htitle = oss.str();
		hmatrix = (TH1D*) gDirectory -> Get(hname.c_str());
		if(hmatrix) delete hmatrix;

		hmatrix = new TH1D(hname.c_str(), htitle.c_str(), 300, 0, 300);
		m_snr_crt[i] = hmatrix;
		m_list_matrix->Add(hmatrix);
	
	}

	
   
	
	for(int i=0; i<16; i++){
		oss.str("");
		oss<<"gbase"<<i;
		hname=oss.str();
		oss.str("");
		oss<<"COL_"<<i;
		htitle=oss.str();
		h_cds = new TH1D(hname.c_str(),htitle.c_str(), 64, 0, 64);
		m_hbaseline[i] = h_cds;
	}

	
	
	// cluster rec algorithm   --LongLI 2021-03-21
	// fixed size (3x3)
	m_list_cluster_rec = new TList;
	TH2D* snr_map;
	oss.str("");
	oss<<"snr_map";
	hname=oss.str();
	oss.str("");
	oss<<"Map of SNR;Distance from central pixel [pixels]; SNR";
	htitle=oss.str();
	snr_map = new TH2D(hname.c_str(), htitle.c_str(), 130, -1, 64, 250,-50, 200);
	m_snr_map = snr_map;
	m_list_cluster_rec->Add(snr_map);
	
	TProfile* snr_prf;
	oss.str("");
	oss<<"snr_prf";
	hname=oss.str();
	oss.str("");
	oss<<"Profile of SNR; Distance from central pixel [pixels]; SNR";
	htitle=oss.str();
	snr_prf = new TProfile(hname.c_str(), htitle.c_str(),256, 0, 64, -20, 200);
	m_snr_prf = snr_prf;
	m_list_cluster_rec->Add(snr_prf);

	TH1D* clu, *pixsum, *dist, *snr_opt;
	for(int i=0; i<7; i++){
	oss.str("");
	oss<<"pix_sum_"<<i+2;
	hname=oss.str();
	oss.str("");
	oss<<"CDS sum in 5#times5 array with S/R > "<<i+2<<";CDS sum [ADCu];Entries/1";
	htitle=oss.str();
	pixsum = new TH1D(hname.c_str(), htitle.c_str(),300, 0, 300);
	m_pix_sum[i] = pixsum;
	m_list_cluster_rec->Add(pixsum);

	oss.str("");
	oss<<"clu_size_"<<i+2;
	hname=oss.str();
	oss.str("");
	oss<<"Cluster size for S/R > "<<i+2<<";Cluster size;Entries/1";
	htitle=oss.str();
	clu = new TH1D(hname.c_str(), htitle.c_str(),10, 0, 10);
	m_clu_size[i] = clu;
	m_list_cluster_rec->Add(clu);

	oss.str("");
	oss<<"clu_dist_"<<i+2;
	hname=oss.str();
	oss.str("");
	oss<<"Distance from the central pixel for S/R > "<<i+2<<";Distance [pixels];Entries/1";
	htitle=oss.str();
	dist = new TH1D(hname.c_str(), htitle.c_str(), 20, 0, 4.5);
	m_distance[i] = dist;
	m_list_cluster_rec->Add(dist);

	oss.str("");
	oss<<"sum_fixed_"<<i+2;
	hname = oss.str();
	oss.str("");
	oss<<"Sum in fixed array with threshold "<<i+2<<"#sigma";
	htitle = oss.str();
	snr_prf = new TProfile(hname.c_str(), htitle.c_str(), 9, 0.5, 9.5, 0, 5000);
	m_sum_fixed[i] = snr_prf;
	m_list_cluster_rec->Add(snr_prf);

	oss.str("");
	oss<<"snr_fixed_"<<i+2;
	hname = oss.str();
	oss.str("");
	oss<<"Cluster SNR in fixed array with threshold "<<i+2<<"#sigma";
	htitle = oss.str();
	snr_prf = new TProfile(hname.c_str(), htitle.c_str(), 9, 0.5, 9.5, 0, 250);
	m_snr_fixed[i] = snr_prf;
	m_list_cluster_rec->Add(snr_prf);
	

	oss.str("");
	oss<<"tot_fixed_"<<i+2;
	hname = oss.str();
	oss.str("");
	oss<<"Pixel sum in the cluster with threshold "<<i+2<<" #sigma;Sum;Entries/1";
	htitle = oss.str();
	clu = new TH1D(hname.c_str(), htitle.c_str(), 250, 0, 250);
	m_tot_fixed[i] = clu;
	m_list_cluster_rec->Add(clu);

	oss.str("");
	oss<<"noise_fixed_"<<i+2;
	hname = oss.str();
	oss.str("");
	oss<<"Pixel noise in the cluster with threshold "<<i+2<<" #sigma;Sum;Entries/1";
	htitle = oss.str();
	clu = new TH1D(hname.c_str(), htitle.c_str(), 50, 0, 10);
	m_noise_fixed[i] = clu;
	m_list_cluster_rec->Add(clu);
	
	oss.str("");
	oss<<"hist_snr_fixed_"<<i+2;
	hname = oss.str();
	oss.str("");
	oss<<"Cluster SNR in the cluster with threshold "<<i+2<<" #sigma;Sum;Entries/1";
	htitle = oss.str();
	clu = new TH1D(hname.c_str(), htitle.c_str(), 150, 0, 150);
	m_hist_snr_fixed[i] = clu;
	m_list_cluster_rec->Add(clu);

	oss.str("");
	oss<<"size_fixed_"<<i+2;
	hname = oss.str();
	oss.str("");
	oss<<"Cluster size with threshold "<<i+2<<" #sigma; Size; Entries/1";
	htitle = oss.str();
	clu = new TH1D(hname.c_str(), htitle.c_str(), 20, 0.5, 20.5);
	m_size_fixed[i] = clu;
	m_list_cluster_rec->Add(clu);


	}








	TProfile* clst_q, *clst_snr;
	oss.str("");
	oss<<"cluster_q_adc";
	hname=oss.str();
	oss.str("");
	oss<<"Charge as a function of pixels in 3#times3 arrays (ADC ordering)";
	htitle=oss.str();
	clst_q = new TProfile(hname.c_str(), htitle.c_str(), 9, 0, 9, 0, 250);
	m_clst_q[0] = clst_q;
	m_list_cluster_rec->Add(clst_q);
	oss.str("");
	oss<<"cluster_q_snr";
	hname=oss.str();
	oss.str("");
	oss<<"Charge as a function of pixels in 3#times3 arrays (SNR ordering)";
	htitle=oss.str();
	clst_q = new TProfile(hname.c_str(), htitle.c_str(), 9, 0, 9, 0, 250);
	m_clst_q[1] = clst_q;
	m_list_cluster_rec->Add(clst_q);

	oss.str("");
	oss<<"cluster_snr_adc";
	hname=oss.str();
	oss.str("");
	oss<<"cluster SNR for individual pixel noise (ADC ordering)";
	htitle=oss.str();
	clst_snr = new TProfile(hname.c_str(), htitle.c_str(), 9, 0, 9, 0, 100);
	m_clst_snr[0] = clst_snr;
	m_list_cluster_rec->Add(clst_snr);
	oss.str("");
	oss<<"cluster_snr_snr";
	hname=oss.str();
	oss.str("");
	oss<<"cluster SNR for individual pixel noise (SNR ordering)";
	htitle=oss.str();
	clst_snr = new TProfile(hname.c_str(), htitle.c_str(), 9, 0, 9, 0, 100);
	m_clst_snr[1] = clst_snr;
	m_list_cluster_rec->Add(clst_snr);
	oss.str("");
	oss<<"cluster_snr_average";
	hname=oss.str();
	oss.str("");
	oss<<"cluster SNR for average pixel noise";
	htitle=oss.str();
	clst_snr = new TProfile(hname.c_str(), htitle.c_str(), 9, 0, 9, 0, 100);
	m_clst_snr[2] = clst_snr;
	m_list_cluster_rec->Add(clst_snr);


	// for maximum SNR cluster
	m_list_cluster_opt = new TList;
	TH1D* snrdis_opt;
	for(int i=0; i<7; i++){
		oss.str("");
		oss<<"SNR_opt_"<<i+2;
		hname=oss.str();
		oss.str("");
		oss<<"Cluster SNR with threshold "<<i+2<<"#sigma; SNR; Entries/1";
		htitle=oss.str();
		snr_opt = new TH1D(hname.c_str(), htitle.c_str(), 100, 0, 60);
		m_snr_opt[i] = snr_opt;
		m_list_cluster_opt->Add(snr_opt);
		
		
		oss.str("");
		oss<<"ADC_sum_opt_"<<i+2;
		hname=oss.str();
		oss.str("");
		oss<<"ADC sum for pixels in optimized cluster for threshold "<<i+2<<"#sigma;ADC;Entries/1";
		htitle=oss.str();
		pixsum = new TH1D(hname.c_str(), htitle.c_str(), 300, 0, 300);
		m_sum_opt[i] = pixsum;
		m_list_cluster_opt->Add(pixsum);


		oss.str("");
		oss<<"clst_size_opt_"<<i+2;
		hname=oss.str();
		oss.str("");
		oss<<"Size of optimized clusters with threshold "<<i+2<<"#sigma; Size; Entries/1";
		htitle=oss.str();
		clu = new TH1D(hname.c_str(), htitle.c_str(), 9, 0, 9);
		m_size_opt[i] = clu;
		m_list_cluster_opt->Add(clu);

		oss.str("");
		oss<<"pix_dist_"<<i+2;
		hname=oss.str();
		oss.str("");
		oss<<"Distance from the central pixel of pixels in optimized cluster with threshold "<<i+2<<"#sigma; Distance; Entries/1";
		htitle=oss.str();
		dist = new TH1D(hname.c_str(), htitle.c_str(), 20, 0, 4.5);
		m_dist_opt[i] = dist;
		m_list_cluster_opt->Add(dist);
		
		oss.str("");
		oss<<"snrdis_opt_"<<i+2;
		hname=oss.str();
		oss.str("");
		oss<<"SNR of optimized cluster with threshold"<<i+2<<"#sigma; SNR; Entries/1";
		htitle=oss.str();
		snrdis_opt = new TH1D(hname.c_str(), htitle.c_str(), 100, 0, 100);
		m_snrdis_opt[i] = snrdis_opt;
		m_list_cluster_opt->Add(snrdis_opt);
	}		



	//extrapolation clustering method


	m_list_cluster_alg = new TList;
	for(int i=0; i<20; i++){
		oss.str("");
		oss<<"sum_alg_"<<i*0.2+1;
		hname = oss.str();
		oss.str("");
		oss<<"ADC sum in cluster with threshold "<<i*0.2+1<<"#sigma";
		htitle = oss.str();
		clst_q = new TProfile(hname.c_str(), htitle.c_str(), 9, 0.5, 9.5, 0, 5000);
		m_sum_alg[i] = clst_q;
		m_list_cluster_alg->Add(clst_q);

		oss.str("");
		oss<<"snr_alg_"<<i*0.2+1;
		hname = oss.str();
		oss.str("");
		oss<<"Cluster SNR with threshold "<<i*0.2+1<<"#sigma";
		htitle = oss.str();
		clst_snr = new TProfile(hname.c_str(), htitle.c_str(), 20, 0.5, 20.5, 0, 250);
		m_snr_alg[i] = clst_snr;
		m_list_cluster_alg->Add(clst_snr);

		oss.str("");
		oss<<"pxs_left_"<<i*0.2+1;
		hname = oss.str();
		oss.str("");
		oss<<"Pixels over threshold left after clustering("<<i*0.2+1<<" #sigma);Pixels;Entries/1";
		htitle = oss.str();
		clu = new TH1D(hname.c_str(), htitle.c_str(), 50, -0.5, 50.5);
		m_pxs_left[i] = clu;
		m_list_cluster_alg->Add(clu);


		oss.str("");
		oss<<"clusre_snr_"<<i*0.2+1;
		hname = oss.str();
		oss.str("");
		oss<<"Cluster SNR threshold "<<i*0.2+1<<"#sigma; SNR; Entries/1";
		htitle = oss.str();
		clu = new TH1D(hname.c_str(), htitle.c_str(), 150, 0, 150);
		m_snr_clu[i] = clu;
		m_list_cluster_alg->Add(clu);

		oss.str("");
		oss<<"tot_alg_"<<i*0.2+1;
		hname = oss.str();
		oss.str("");
		oss<<"ADC summed in clusters with threshold "<<i*0.2+1<<"#sigma; ADC ; Entries/1";
		htitle = oss.str();
		pixsum = new TH1D(hname.c_str(), htitle.c_str(), 250, 0, 250);
		m_tot_alg[i] = pixsum;
		m_list_cluster_alg->Add(pixsum);	
		
		oss.str("");
		oss<<"noise_alg_"<<i*0.2+1;
		hname = oss.str();
		oss.str("");
		oss<<"Pixel noise in the cluster with threshold "<<i*0.2+1<<" #sigma;Sum;Entries/1";
		htitle = oss.str();
		clu = new TH1D(hname.c_str(), htitle.c_str(), 50, 0, 10);
		m_noise_alg[i] = clu;
		m_list_cluster_alg->Add(clu);
	
		
		oss.str("");
		oss<<"size_alg_"<<i*0.2+1;
		hname = oss.str();
		oss.str("");
		oss<<"Cluster size with threshold "<<i*0.2+1<<"#sigma; ADC ; Entries/1";
		htitle = oss.str();
		clu = new TH1D(hname.c_str(), htitle.c_str(), 20, 0.5, 20.5);
		m_size_alg[i] = clu;
		m_list_cluster_alg->Add(clu);	
		
		oss.str("");
		oss<<"dist_alg_"<<i*0.2+1;
		hname = oss.str();
		oss.str("");
		oss<<"Distance from the central pixel with threshold "<<i*0.2+1<<"#sigma; ADC ; Entries/1";
		htitle = oss.str();
		dist = new TH1D(hname.c_str(), htitle.c_str(), 20, 0, 4.5);
		m_dist_alg[i] = dist;
		m_list_cluster_alg->Add(dist);	

		oss.str("");
		oss<<"seed_ratio"<<i*0.2+1;
		hname = oss.str();
		oss.str("");
		oss<<"seed / cluster with threshold: "<<i*0.2+1<<" #sigma; ratio; Entries/0.01";
		htitle = oss.str();
		dist = new TH1D(hname.c_str(), htitle.c_str(), 100, 0, 1);
		m_seed_rat[i] = dist;
		m_list_cluster_alg->Add(dist);



/*		
		oss.str("");
		oss<<"snr_max_"<<6-i;
		hname = oss.str();
		oss.str("");
		oss<<"Max SNR in cluster with threshold "<<6-i<<"#sigma; SNR; Entries/1";
		htitle = oss.str();
		snrdis_opt = new TH1D(hname.c_str(), htitle.c_str(), 100, 0, 100);
		m_snr_max[i] = snrdis_opt;
		m_list_cluster_alg->Add(snrdis_opt);

		oss.str("");
		oss<<"size_max_"<<6-i;
		hname = oss.str();
		oss.str("");
		oss<<"Cluster size with max SNR with threshold "<<6-i<<"#sigma; Size;Entries/1";
		htitle = oss.str();
		clu = new TH1D(hname.c_str(), htitle.c_str(), 20, 0, 20);
		m_size_max[i] = clu;
		m_list_cluster_alg->Add(clu);

		oss.str("");
		oss<<"dist_max_"<<6-i;
		hname = oss.str();
		oss.str("");
		oss<<"Distance from the central pixel with threshold "<<6-i<<"#sigma; Distance [pixels]; Entries/1";
		htitle = oss.str();
		dist = new TH1D(hname.c_str(), htitle.c_str(), 20, 0, 4.5);
		m_dist_max[i] = dist;
		m_list_cluster_alg->Add(dist);

		oss.str("");
		oss<<"sum_max_"<<6-i;
		hname = oss.str();
		oss.str("");
		oss<<"ADC sum in cluster with max SNR with threshold "<<6-i<<"#sigma;ADC; Entries/1 ";
		htitle = oss.str();
		pixsum = new TH1D(hname.c_str(), htitle.c_str(), 300, 0, 300);
		m_sum_max[i] = pixsum;
		m_list_cluster_alg->Add(pixsum);
*/
			
	}

	oss.str("");
	oss<<"sum_extrapolation";
	hname = oss.str();
	oss.str("");
	oss<<"ADC sum vs. threshold; Threshold; ADC";
	htitle = oss.str();
	clst_q = new TProfile(hname.c_str(), htitle.c_str(), 20, 0.5, 20.5, 0, 500);
	m_sum_extr = clst_q;
	m_list_cluster_alg->Add(clst_q);

	oss.str("");
	oss<<"snr_extrapolation";
	hname = oss.str();
	oss.str("");
	oss<<"Cluster SNR vs. threshold; Threshold; SNR";
	htitle = oss.str();
	clst_q = new TProfile(hname.c_str(), htitle.c_str(), 20, 0.5, 20.5, 0, 200);
	m_snr_extr = clst_q;
	m_list_cluster_alg->Add(clst_q);

	oss.str("");
	oss<<"threshold_cut";
	hname = oss.str();
	oss.str("");
	oss<<"Threshold with cluster between 3 and 5 pixels;Threshold; Entries/1";
	htitle = oss.str();
	clu = new TH1D(hname.c_str(), htitle.c_str(), 20, 0.5, 20.5);
	m_thres_cut = clu;
	m_list_cluster_alg->Add(clu);

	oss.str("");
	oss<<"size_threshold";
	hname = oss.str();
	oss.str("");
	oss<<"Size of cluster with various threshold; Threshold; Size";
	htitle = oss.str();
	clst_q = new TProfile(hname.c_str(), htitle.c_str(), 20, 0.5, 20.5, 0, 80);
	m_size_thres = clst_q;
	m_list_cluster_alg->Add(clst_q);

	oss.str("");
	oss<<"sum_size_prf";
	hname = oss.str();
	oss.str("");
	oss<<"CDS sum as a function of cluster size; Size; CDS";
	htitle = oss.str();
	clst_q = new TProfile(hname.c_str(), htitle.c_str(), 20, 0.5, 20.5, 0, 200);
	m_sum_size_prf = clst_q;
	m_list_cluster_alg->Add(clst_q);
/*	
	for(int i=0; i<5; i++){
		oss.str("");
		oss<<"sum_size_"<<i+1;
		hname = oss.str();
		oss.str("");
		oss<<"ADC sum of cluster with"<<i+1<<"pixel(s) (Threshold 5#sigma); ADC; Entries/1";
		htitle = oss.str();
		pixsum = new TH1D(hname.c_str(), htitle.c_str(), 300, 0, 300);
		m_sum_size[i] = pixsum;
		m_list_cluster_alg->Add(pixsum);

	}
*/		

	// various pixel size output for each threshold
	m_list_cluster_anal = new TList;
	for(int i=0; i<20; i++){
		for(int j=0; j<8; j++){
			oss.str("");
			oss<<"sum_t"<<i+1<<"_size_"<<j+1;
			hname = oss.str();
			oss.str("");
			oss<<"ADC sum with threshold "<<i+1<<" #sigma and size "<<j+1;
			htitle = oss.str();
			clu = new TH1D(hname.c_str(), htitle.c_str(), 250, 0, 250);
			m_sum_size[i][j] = clu;
			m_list_cluster_anal->Add(clu);
		}
	}

	// for special event (large output)
	m_list_special_evt = new TList;
	oss.str("");
	oss<<"special_evt";
	hname = oss.str();
	oss.str("");
	oss<<"Map of the special events; Cluster size; ADC sum";
	htitle = oss.str();
	snr_map = new TH2D(hname.c_str(), htitle.c_str(), 100, 0, 100, 100, 300, 1000);
	m_special_evt = snr_map;
	m_list_special_evt->Add(snr_map);


	// for multi-event
	m_list_multievt = new TList;
	oss.str("");
	oss<<"evt_num";
	hname = oss.str();
	oss.str("");
	oss<<"Cluster number in a single frame;Cluster number; Entries/1";
	htitle = oss.str();
	clu = new TH1D(hname.c_str(), htitle.c_str(), 10, 0.5, 10.5);
	m_evt_num = clu;
	m_list_multievt->Add(clu);


	// cluster properties of multi-events
	for(int i=0; i<3; i++){
		oss.str("");
		oss<<"sum_multi_"<<i+1;
		hname = oss.str();
		oss.str("");
		oss<<"Pixel sum of the cluster_"<<i+1<<" under the threshold of 5 #sigma; Sum; Entries/1";
		htitle = oss.str();
		clu = new TH1D(hname.c_str(), htitle.c_str(), 250, 0, 250);
		m_sum_multi[i] = clu;
		m_list_multievt->Add(clu);

		oss.str("");
		oss<<"size_multi"<<i+1;
		hname = oss.str();
		oss.str("");
		oss<<"Size of the cluster_"<<i+1<<"under the threshold of 5 #sigma; Size; Entries/1";
		htitle = oss.str();
		clu = new TH1D(hname.c_str(), htitle.c_str(), 20, 0.5, 20.5);
		m_size_multi[i] = clu;
		m_list_multievt->Add(clu);
	}

	m_list_correction_estimation = new TList;
	prf = new TProfile("cluster_sum_err", "Cluster sum with CDS_{SEED} < 100 (PIX_F_COR in);CDS_{SEED};SUM", 100, 0, 100, 0, 500);
	m_clu_sum_prf = prf;
	m_list_correction_estimation->Add(prf);

	prf = new TProfile("pix_nor", "Output of the normal pixel vs. CDS_{SEED}; CDS_{SEED}; ADC", 100, 0 , 100, -50, 500);
	m_pix_nor_prf = prf;
	m_list_correction_estimation->Add(prf);

	prf = new TProfile("pix_nor", "Output of the normal pixel vs. CDS_{SEED}; CDS_{SEED}; ADC", 100, 0 , 100, -50, 500);
	m_pix_cor_prf = prf;
	m_list_correction_estimation->Add(prf);

	prf = new TProfile("pix_cor", "#frac{Mean_{PIX_F_cor} - Mean_{normal pixels}}{Cluster ADC sum} vs. CDS_{SEED} (PIX_F in the cluster); CDS_{SEED}; %", 100, 0 , 100, -50, 500);
	m_pix_diff_ratio_prf = prf;
	m_list_correction_estimation->Add(prf);

	prf = new TProfile("pix_cor_s", "#frac{Mean_{PIX_F_cor} - Mean_{normal pixels}}{Cluster ADC sum} vs. CDS_{SEED} (PIX_F in the cluster) ; CDS_{SEED}; %", 100, 0 , 100, -50, 500, "S");
	m_pix_diff_ratio_s_prf = prf;
	m_list_correction_estimation->Add(prf);


	m_list_spatial_resolution = new TList;
	clu = new TH1D("pos_x", "Weighted position X;X-pitch;Entries",200, -1, 1);
	h_pos_x = clu;
	m_list_spatial_resolution->Add(clu);

	clu = new TH1D("pos_y", "Weighted position Y;Y-pitch;Entries",200, -1, 1);
	h_pos_y = clu;
	m_list_spatial_resolution->Add(clu);

	clu = new TH1D("center_x", "Weighted center X of cluster;X [x-pitch]; Entries", 400, -2, 2);
	h_posx_w = clu;
	m_list_spatial_resolution->Add(clu);

	clu = new TH1D("center_y", "Weighted center Y of cluster;Y [x-pitch]; Entries", 400, -2, 2);
	h_posy_w = clu;
	m_list_spatial_resolution->Add(clu);

	m_list_hitmap_thres = new TList;
	for(int i=0; i<6; i++){
		xmin =8; xmax = NCOLS;
		xbins = xmax - xmin;
		ymin = 0; ymax = NROWS;
		ybins = ymax -ymin;
		oss.str("");
		oss<<"hitmap_"<<i<<endl;
		hname = oss.str();
		oss.str("");
		oss<<"hitmap of pixel arrays for threshold "<<i<<"#sigma;COL;ROW";
		htitle = oss.str();
		h2d = new TH2D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax, ybins, ymin, ymax);
	  	m_hitmap_threshold[i] = h2d;
		m_list_hitmap_thres->Add(h2d);	
	}

	clu  = new TH1D("abnormal row", "Pixel cds in ROW 0-1;CDS [ADC];", 250, -50, 200);
	m_abnormal_row_pixel = clu;
	m_list_hitmap_thres->Add(clu);
	
	clu  = new TH1D("normal row", "Pixel cds in ROW 2-63;CDS [ADC];", 250, -50, 200);
	m_normal_row_pixel = clu;
	m_list_hitmap_thres->Add(clu);

	clu  = new TH1D("normal 2rows", "Pixel cds in ROW 2-3;CDS [ADC];", 250, -50, 200);
	m_normal_2_row = clu;
	m_list_hitmap_thres->Add(clu);
	
	
	clu  = new TH1D("pix_r0", "Pixel cds in ROW 0 ;CDS [ADC];", 250, -50, 200);
	m_pix_r0 = clu;
	m_list_hitmap_thres->Add(clu);
	
	clu  = new TH1D("pix_r1", "Pixel cds in ROW 1 ;CDS [ADC];", 250, -50, 200);
	m_pix_r1 = clu;
	m_list_hitmap_thres->Add(clu);

	clu  = new TH1D("pix_r2", "Pixel cds in ROW 2 ;CDS [ADC];", 250, -50, 200);
	m_pix_r2 = clu;
	m_list_hitmap_thres->Add(clu);
   
	clu  = new TH1D("pix_r3", "Pixel cds in ROW 3 ;CDS [ADC];", 250, -50, 200);
	m_pix_r3 = clu;
	m_list_hitmap_thres->Add(clu);

	
	
	m_list_fake_hit = new TList;
	
	clu = new TH1D("snr_allpix", "SNR for all pixels in the matrix; SNR; Entries", 400, -100, 100);
	m_snr_allpix = clu;
	m_list_fake_hit->Add(clu);

	clu = new TH1D("fake_trigger", "Fake-trigger vs. threshold; Threshold [#sigma]; Entries", 20, -0.5, 19.5);
	m_fake_frames = clu;
	m_list_fake_hit->Add(clu);	

	clu = new TH1D("snr_default", "Default SNR for all pixels vs. threshold; SNR; Entries", 400, -100, 100);
	m_snr_def = clu;
	m_list_fake_hit->Add(clu);	

	clu = new TH1D("fake_trigger_def", "Fake-trigger default vs. threshold; Threshold [#sigma]; Entries", 20, -0.5, 19.5);
	m_fake_frames_def = clu;
	m_list_fake_hit->Add(clu);	
	
	clu = new TH1D("frame_tot", "Total frame; frame; Entries", 2, 0, 2);
	m_frame_num = clu;
	m_list_fake_hit->Add(clu);	





	m_list_pix_snr = new TList;
	clu = new TH1D("snr_pix", "SNR of pixels in half-matrix;SNR;Entries", 250, -50, 200);
	m_pix_snr = clu;
	m_list_pix_snr->Add(clu);



	m_list_cluster_size = new TList;
	for(int i=0; i<4; i++){
		oss.str("");
		oss<<"size_"<<i+2;
		htitle = oss.str();
		oss.str("");
		oss<<"Distance distribution in the cluster with size of "<<i+2<<"; Distance [pixels];";
		hname = oss.str();
		clu = new TH1D(htitle.c_str(), hname.c_str(), 100, 0, 10);
		m_size_anal[i] = clu;
		m_list_cluster_size->Add(clu);
	}

	clu = new TH1D("shape_size3", "shape of cluster with size of 3;Shape ID;", 3, 0.5, 3.5);
	m_shape_size3 = clu;
	m_list_cluster_size->Add(clu);
	clu = new TH1D("shape_size4", "shape of cluster with size of 4;Shape ID;", 5, 0.5, 5.5);
	m_shape_size4 = clu;
	m_list_cluster_size->Add(clu);


	m_list_hit = new TList;

	clu = new TH1D("hit_pos_x", "Hit position X;#times Pitch-X;",20, -1.05, 0.95);
	m_hit_pos_x = clu;
	m_list_hit->Add(clu);

	clu = new TH1D("hit_pos_y", "Hit position Y;#times Pitch-Y;", 20, -1.05, 0.95);
	m_hit_pos_y = clu;
	m_list_hit->Add(clu);
	

	for(int i =0; i<4; i++){
		oss.str("");
		oss<<"pos_x_size_"<<i+2;
		htitle = oss.str();
		oss.str("");
		oss<<"Hit X-position with pixel size "<<i+2<<";#times Pitch-X;";
		hname = oss.str();
		clu = new TH1D(htitle.c_str(), hname.c_str(), 20, -1.05, 0.95);
		m_pos_x[i] = clu;
		m_list_hit->Add(clu);
		
		oss.str("");
		oss<<"pos_y_size_"<<i+2;
		htitle = oss.str();
		oss.str("");
		oss<<"Hit Y-position with pixel size "<<i+2<<";#times Pitch-Y;";
		hname = oss.str();
		clu = new TH1D(htitle.c_str(), hname.c_str(), 20, -1.05, 0.95);
		m_pos_y[i] = clu;
		m_list_hit->Add(clu);

		oss.str("");
		oss<<"hit_pos_map_"<<i+2;
		hname = oss.str();
		oss.str("");
		oss<<"Map of hit position with size "<<i+2<<";X [pixels];Y [pixels]";
		htitle = oss.str();
		h2d = new TH2D(hname.c_str(), htitle.c_str(), 20, -1.05, 0.95, 20, -1.05, 0.95);
		m_hit_pos_map[i] = h2d;
		m_list_hit->Add(h2d);

		oss.str("");
		oss<<"hit_prf_"<<i+2;
		hname = oss.str();
		oss.str("");
		oss<<"Cluster projection with size "<<i+2<<";X [pixels];Y [pixels]";
		h2d = new TH2D(hname.c_str(), htitle.c_str(), 5, 0.5, 5.5, 5, 0.5, 5.5);
		m_hit_prf_size[i] = h2d;
		m_list_hit->Add(h2d);

		oss.str("");
		oss<<"q_seed_"<<i+2;
		hname = oss.str();
		oss.str("");
		oss<<"Seed charge with cluster size "<<i+2<<";ADC;";
		htitle = oss.str();
		clu = new TH1D(hname.c_str(), htitle.c_str(), 150, 0, 150);
		m_hit_q_seed[i] = clu;
		m_list_hit->Add(clu);


		oss.str("");
		oss<<"q_adjacent_"<<i+2;
		hname = oss.str();
		oss.str("");
		oss<<"Charge of adjacent pixels with cluster size "<<i+2<<";ADC;";
		htitle = oss.str();
		clu = new TH1D(hname.c_str(), htitle.c_str(), 150, 0, 150);
		m_hit_q_adj[i] = clu;
		m_list_hit->Add(clu);
	}

	h2d = new TH2D("hit_pos_tot", "Map of hit position;X [pixels];Y [pixels]", 20, -1.05, 0.95, 20, -1.05, 0.95);	
	m_hit_pos_tot = h2d;
	m_list_hit->Add(h2d);

	h2d = new TH2D("hit_prf", "Cluster projection;X [pixels];Y [pixels]", 3, 0.5, 3.5, 3, 0.5, 3.5);	
	m_hit_prf = h2d;
	m_list_hit->Add(h2d);
	
	clu = new TH1D("q_dis", "charge distribution in the cluster;ADC", 150, 0, 150);
	m_hit_q = clu;
	m_list_hit->Add(clu);

	clu = new TH1D("clu_pos_x", "X-position in cluster;#times X-pitch;", 20, -1.05, 0.95);
	m_clu_pos_x = clu;
	m_list_hit->Add(clu);

	clu = new TH1D("clu_pos_y", "Y-position in cluster;#times Y-pitch;", 20, -1.05, 0.95);
	m_clu_pos_y = clu;
	m_list_hit->Add(clu);

	h2d = new TH2D("hit_clu", "Hit position in cluster;X [pixels];Y [pixels]", 20, -1.05, 0.95, 20, -1.05, 0.95);	
	m_hit_clu = h2d;
	m_list_hit->Add(h2d);
	
	m_booked = true;
}

// write out non-histogram objects
//______________________________________________________________________
void SupixAnly::write_waveform()
{
   m_mg_adc_frame->Write();
   m_mg_cds_trig->Write();
   m_mg_cds_frame->Write();
   m_mg_chi2_cds->Write();

   LOG << gFile->GetName() << endl;
   // save pixels waveforms in a subdir
   const char* subdir = "wave_pixs";
   gFile->mkdir(subdir);
   gFile->cd(subdir);
   //bytes = m_list_wave_adc->Write("wave_adc", 1);
   m_list_wave_adc->Write();
   // m_list_wave_cds->Write("wave_cds", 1);
   m_list_wave_cds->Write();
   // others in home dir
   gFile->cd();
}

// write out non-histogram objects
//______________________________________________________________________
void SupixAnly::Write()
{
   m_list_adc->Write("pix_adc", 1);
   m_list_cds->Write("pix_cds", 1);
   m_list_seed->Write("seed", 1);
   m_list_shape->Write("shape", 1);
   m_list_correction->Write("correction", 1);
   m_list_matrix->Write("matrixes",1);
   m_list_cluster_rec->Write("cluster_rec",1);
   m_list_cluster_opt->Write("cluster_opt",1);
   m_list_cluster_alg->Write("cluster_alg",1);
   m_list_cluster_anal->Write("cluster_anal",1);
   m_list_special_evt->Write("special_events",1);
	m_list_multievt->Write("multi_events", 1);
	m_list_correction_estimation->Write("Cluster_ERR", 1);
	m_list_spatial_resolution->Write("Spatial_resolution", 1);
	m_list_hitmap_thres->Write("hitmap_thres", 1);
	m_list_fake_hit->Write("fake_hit", 1);
	m_list_pix_snr->Write("pix_snr", 1);
	m_list_cluster_size->Write("cluster_size_cut_3", 1);
	m_list_hit->Write("hit_position", 1);
	m_list_misc->Write();
}

// fill histograms
//______________________________________________________________________
void SupixAnly::Loop()
{
   TRACE;
   if (fChain == 0) return;
   //Long64_t nentries = fChain->GetEntriesFast();
   Long64_t nentries = fChain->GetEntries();
   LOG << " nentries = " << nentries << endl;
   Loop(nentries);
}

//______________________________________________________________________
void SupixAnly::Loop(Long64_t nentries)
{
   TRACE;
   //   In a ROOT session, you can do:
   //      Root > .L SupixTree.C
   //      Root > SupixTree t
   //      Root > t.GetEntry(12); // Fill t data members with entry number 12
   //      Root > t.Show();       // Show values of entry 12
   //      Root > t.Show(16);     // Read and show values of entry 16
   //      Root > t.Loop();       // Loop on all entries
   //

   //     This is the loop skeleton where:
   //    jentry is the global entry number in the chain
   //    ientry is the entry number in the current Tree
   //  Note that the argument to GetEntry must be:
   //    jentry for TChain::GetEntry
   //    ientry for TTree::GetEntry and TBranch::GetEntry
   //
   //       To read only selected branches, Insert statements like:
   // METHOD1:
   //    fChain->SetBranchStatus("*",0);  // disable all branches
   //    fChain->SetBranchStatus("branchname",1);  // activate branchname
   // METHOD2: replace line
   //    fChain->GetEntry(jentry);       //read all branches
   //by  b_branchname->GetEntry(ientry); //read only this branch
   if (fChain == 0) return;

   Timer timer(__func__);
   timer.start();
 	
	Double_t pix_rms[NROWS][NCOLS]; 
   Double_t cds_snr[NROWS][NCOLS];
   Double_t cds_snr_raw[NROWS][NCOLS];
   Double_t cds_output[NROWS][NCOLS];
   Double_t cds_output_raw[NROWS][NCOLS];
   Double_t adc_output_raw[NROWS][NCOLS];
   Double_t snr25_raw[25];
   Double_t snr25_crt[25];
   Double_t cds25_raw[25];
   Double_t cds25_crt[25];
   Double_t pixel_fired[25];
   Double_t snr9_crt[9];
   Double_t cds9_raw[9];
   Double_t snr9_raw[9];
   Double_t cds9_crt[9];


   //For  SNR  noise average
   Double_t sigma_sum =0, sigma_mean=0;
   if (! m_booked)
      Book();

   if (nentries == 0)
      nentries = fChain->GetEntriesFast();

   //
   // before starting the global loop
   //
   ULong64_t frame_last = 0;	// of last triged event

   unsigned long nconsecutives = 0;	// consecutive trigs
   unsigned long nhits = 0;		// real trig
   Long64_t nbytes = 0, nb = 0;
   for (Long64_t jentry=0; jentry<nentries;jentry++) {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);
      nbytes += nb;
      // if (Cut(ientry) < 0) continue;
      //--------------------------------------->> start here

      // for each new tree
      if (ientry == 0) {
	 m_runinfo = (RunInfo*)fChain->GetTree()->GetUserInfo()->At(0);
      }
      
      adc_t*	padc = (adc_t*)pixel_adc;
      cds_t*	pcds = (cds_t*)pixel_cds;
      double	cds_x = m_runinfo->trig_cds_x;
      double*	pthr = (double*)(m_runinfo->trig_cds);
      double*	pcds_mean = (double*)(m_runinfo->cds_mean);
      double*	pcds_sigma = (double*)(m_runinfo->cds_sigma);
      double*	padc_mean = (double*)(m_runinfo->adc_mean);
      double*	padc_sigma = (double*)(m_runinfo->adc_sigma);
      double	cds_cor, adc_cor;
      double	chi2_cds = 0;
      double	chi2_trig = 0;
#ifdef DEBUG
      LOG << "DEBUG"
	  << " ientry=" << ientry
	  << " runinfo=" << m_runinfo
	  << " *pcds_mean=" << *pcds_mean
	  << " *pcds_sigma=" << *pcds_sigma
	  << endl;
#endif
      for (int i=0; i < NROWS; i++) {//+++++++++++++++++++++++++++++++++start a frame
	 for (int j=0; j < NCOLS; j++) {

		 cds_output_raw[i][j] = *pcds;
		 adc_output_raw[i][j] = *padc;
//		cout<<"cds_raw["<<i+1<<"]["<<j<<"]:\t"<<*pcds<<endl; 
		 cds_cor = *pcds;// - *pcds_mean;
		//Non-normalized frame output --Long LI
		cds_output[i][j] = *pcds;

		//Non-normalized CDS
		m_cds[i][j] ->Fill(*pcds);		
		// CDS profile
	   if( j > 7){   // for Half matrix --LongLI 21-10-23
	  	 m_cds_frame_raw->Fill(NROWS*j + i, *pcds);
	    m_cds_frame_cor->Fill(NROWS*j + i, cds_cor);
		}
		cds_cor /= *pcds_sigma;		// normalized
	    adc_cor = *padc - *padc_mean;
	   
		// fill the hitmap half matrix -- LongLI 2021-10-28
		
		pix_rms[i][j] = *pcds_sigma;		


		// ADC profile
		if(j> 7){
	    m_adc_frame_raw->Fill(NROWS*j + i, *padc);
	    m_adc_frame_cor->Fill(NROWS*j + i, adc_cor);
		}
		 adc_cor /= *padc_sigma;		// normalized
	    chi2_cds += cds_cor * cds_cor;
#ifdef ALLPIXS
	    // m_cds[i][j]->Fill( *pcds );
	    
		//normalized output --Long LI
//		m_cds[i][j]->Fill(cds_cor);
		cds_snr[i][j] = cds_cor;
		cds_snr_raw[i][j] = cds_cor;
	    // m_adc[i][j]->Fill(*padc);
	    m_adc[i][j]->Fill(*padc);
#endif
	    // CDS
	    m_cds_raw->Fill(*pcds);
	    m_cds_cor->Fill(cds_cor);

	    // CDS profile
	//    m_cds_frame_raw->Fill(NROWS*j + i, *pcds);
	//    m_cds_frame_cor->Fill(NROWS*j + i, cds_cor);

	    // ADC
	    m_adc_raw->Fill(*padc);
	    m_adc_cor->Fill(adc_cor);

	    // ADC profile
	 //   m_adc_frame_raw->Fill(NROWS*j + i, *padc);
	 //   m_adc_frame_cor->Fill(NROWS*j + i, adc_cor);
	    
		// noise sum
		sigma_sum += m_runinfo->cds_sigma[i][j];
		// hitmap threshold
	    
		// next pixel
	    padc++;
	    pcds++;
	    pthr++;
	    pcds_mean++;
	    pcds_sigma++;
	    padc_mean++;
	    padc_sigma++;
	 }
      }//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~end a frame

		//noise average
		sigma_mean = sigma_sum/(NROWS*NCOLS);
		sigma_sum =0;

		// show the base line of the pixel output 
		if(jentry ==1){
	//		 ofstream fnoise("pixel_noise/a8_pixel_noise.txt");
 			 for(int i =0; i<NCOLS; i++){
				for(int j=0; j<NROWS; j++){
					m_hbaseline[i]->SetBinContent(j,adc_output_raw[j][i]);
	//				fnoise<<pix_rms[i][j]<<" ";
				}
	//			fnoise<<endl;
			}

		}

      // frame-wise
      //
      // of triged frame
      if (true_trig(trig) ) {
	 m_trig->Fill(trig+0.5);

	 // excluding periodic trigs
	 if (trig > 1) {
	    nhits++;
		double _cds = 0;
		Int_t seed_row = 0, seed_col = 0;
	    double cds_sum = 0;
	    for (int i=0; i < npixs; i++) {
	       int row = pixid[i] >> NBITS_COL;
	       int col = pixid[i] &  MASK_COL;
	       m_hitmap_cds->Fill(col+0.5, row+0.5);

	       double cds_cor = pixel_cds[row][col] - m_runinfo->cds_mean[row][col];

		   //save the fired pixel output for  not more than 25 pixels		   
		   if(npixs < 26){
			   pixel_fired[i] = -cds_output[row][col];
			}

	       cds_cor /= m_runinfo->cds_sigma[row][col];	// normalize by sigma
	       m_trig_cds->Fill(pixel_cds[row][col]);
	       cds_sum += cds_cor;
		

		// add  analysis  --LongLI
			if(trig < 128){
				if(cds_snr[row][col] < cds_snr[seed_row][seed_col]){
					seed_row = row;
					seed_col = col;
				}

			}

		}

		if(trig <128){
		int count =0;
		double cds = 0;
		double cds_crt = 0;
		double cds_normalized = 0;
	    double fired_cds = 0;	
		double fired_cds_correct = 0;


		// 5x5 


		if(seed_row>1 && seed_row <63 && seed_col >8 && seed_col <14){
			
			//for special events (sum_cds >300)
			double sum_cds =0;
   		for (int i=0; i < npixs; i++) {
      		int row = pixid[i] >> NBITS_COL;
      		int col = pixid[i] &  MASK_COL;
				sum_cds += -cds_output[row][col];
   		}
			if(sum_cds > 300) m_special_evt->Fill(npixs, sum_cds);
			sum_cds =0;



			for(int i = seed_row-2; i<=seed_row+2; i++){
				for(int j = seed_col-2; j<=seed_col+2; j++){
					
					//correction
					if(i == seed_row+1 && j == seed_col){
				
						cds_snr[i][j] = (charge_correction(cds_output[seed_row][seed_col], cds_output[i][j]) -
							m_runinfo->cds_mean[i][j])/(m_runinfo->cds_sigma[i][j]);
						cds_output[i][j] = charge_correction(cds_output[seed_row][seed_col], cds_output[i][j]);
					}


					snr25_raw[count] = cds_snr_raw[i][j];
					snr25_crt[count] = cds_snr[i][j];
					cds25_raw[count] = cds_output_raw[i][j];
					cds25_crt[count] = cds_output[i][j];

					count++;

				}
			}

			count = 0;

			sort(cds25_crt, cds25_crt + 25);
			
			double adc_sum = 0;
			double Q_sum[25] = {0};
			for(int i=0; i<25; i++){
				adc_sum += -cds25_crt[i];
				Q_sum[i] = adc_sum;
				m_matrix_prf->Fill(i+1, adc_sum);
				m_matrix_prf_s->Fill(i+1, adc_sum);
				m_mat_pix_prf->Fill(i+1, -cds25_crt[i]);	
			}
			adc_sum = 0;

			for(int i=0; i<25; i++){
				m_matrix_pnt_prf->Fill(i+1, Q_sum[i]/124.2);//(1.0*Q_sum[24]));
				m_matrix_pnt_prf_s->Fill(i+1, Q_sum[i]/124.2);//(1.0*Q_sum[24]));

			}

			//fill the pixel snr in the  half matrix   --LongLI 2021-12-03

			for(int i=0; i<NROWS; i++){
				for(int j=0; j<NCOLS; j++){
					if(j <8) continue;
					m_pix_snr->Fill(-cds_snr[i][j]);
				}
			}







			double pix_rms[NROWS][NCOLS] = {0};

			if(pix_rms[0][0] == 0){
				ifstream fin("pixel_noise/a0_pixel_noise.txt");
				if(!fin.is_open()){
					cout<<"Fail to open the noise file!"<<endl;
					exit(-1);
				}
				
				for(int i=0; i<NROWS; i++){
					for(int j=0; j<NCOLS; j++){
						fin>>pix_rms[i][j];
					}
				}
				fin.close();
			}	
		
		// for fake-hit   LongLI 2021-11-03
		bool over_thr[20] = {0};
		bool over_thr_def[20] = {0};
		for(int i=0; i<NROWS; i++){
			for(int j=0; j<NCOLS; j++){
				if(j < 8) continue;
				m_snr_allpix->Fill(-cds_output[i][j]/pix_rms[i][j]);	
				m_snr_def->Fill(-cds_snr[i][j]);
				
				for(int k=0; k<20; k++){
					if(-cds_output[i][j]/pix_rms[i][j] >= k ){
						over_thr[k] = true;	
					}
					if(-cds_snr[i][j] >=k ){
						over_thr_def[k] = true;
					}	
				}
			}
				
		}

		for(int k=0; k<20; k++){
			if(over_thr[k] == true)
				m_fake_frames->Fill(k);
			
			if(over_thr_def[k] == true)
				m_fake_frames_def->Fill(k);

			over_thr[k] == false;
			over_thr_def[k] == false;
		}


		m_frame_num->Fill(1);


			//fill the seed snr -- LongLI 2021-10-24

			m_seed_snr->Fill(-cds_snr[seed_row][seed_col]);
		

			//anomaly in ROW0 and ROW1   LongLI --2021-10-29
		/*
			for(int i =0; i<NROWS; i++){
				for(int j =0; j<NCOLS; j++){
		 			for(int k =0; k<6; k++){
		 				if(j > 7 && -cds_snr[i][j] >= k){
				 		m_hitmap_threshold[k] ->Fill(j+ 0.5, i+0.5);

						if(k == 4 && i < 2) m_abnormal_row_pixel->Fill(-cds_output[i][j]);
						if(k == 4 && i > 2) m_normal_row_pixel->Fill(-cds_output[i][j]);
		 				if(k ==4 && (i== 2||i ==3)) m_normal_2_row->Fill(-cds_output[i][j]);
			}
		 }
					
				}
			}

*/




			// Mean of ADC_{normal pixel}   --LongLI 2021-10-24
			double pix_mean_nor[100] = {
		  		 0,0,0,0,0,0,0,0,0,0,                                                                                                         
  				 0,0,0,5.46664,4.32249,4.30971,4.41694,4.6719,5.10492,5.0715,
  				 5.58685,5.5901,5.62377,6.04017,6.02815,6.33619,6.55763,6.33782,6.80423,6.75667,
  				 6.70185,6.8082,6.95955,6.73127,6.79323,6.86964,6.63521,6.51712,6.48791,6.35799,
  				 6.23003,6.1698,5.94241,5.76254,5.64788,5.67928,5.66383,5.99773,5.98289,5.5885,
  				 6.02573,5.94561,6.467,6.36298,6.57126,6.80028,6.85928,6.85512,7.05563,7.12313,
  				 7.40827,7.42279,7.63819,7.70161,7.72629,7.75554,7.92468,7.92804,7.99664,7.96075,
  				 8.14537,8.17192,8.14017,8.21287,8.20517,8.3405,8.4356,8.53593,8.51158,8.59353,
  				 8.73125,8.73658,8.60807,8.97186,8.96729,8.98774,9.03495,9.16369,9.1426,9.24362,
 				 9.37923,9.42668,9.53806,9.4106,9.74709,9.80555,9.91234,9.85703,9.81151,9.89866
			};
			double pix_mean_cor[100] = {
				 0,0,0,0,0,0,0,0,0,0,                                                                                                         
  				 0,0,0,1.32152,2.79376,2.28484,2.95986,3.1873,3.21223,4.04305,
  				 3.40222,3.99839,4.47288,4.51717,4.84964,5.06274,4.59626,4.9941,4.67707,5.07054,
  				 4.98284,5.11267,4.53954,5.07006,5.23708,5.07674,5.21115,4.98198,5.30596,4.69664,
  				 3.9978,3.9801,4.39313,4.27648,4.27723,3.66075,4.02171,3.97604,3.8803,4.28023,
  				 4.29398,4.36328,4.80771,4.82283,4.86522,4.90933,5.29831,5.63844,5.84625,5.74713,
  				 5.7366,6.32607,6.37832,6.4796,6.47173,6.58061,6.66941,6.80146,6.29387,6.78597,
  				 6.93655,6.63726,7.08871,7.00549,7.24587,7.25348,7.38817,7.53857,7.32099,7.53989,
  				 7.63595,7.82506,8.0588,7.51866,7.39809,7.81781,7.86724,7.70332,8.32736,8.10022,
 				 8.38531,8.71637,8.52362,8.76948,9.10689,8.6232,8.6507,9.00265,9.15226,9.29647
			};

			// profile of the normal pixel --LongLI 2021-10-20
			double pix_diff = 0;
			if(-cds_output[seed_row][seed_col] < 100){
				double seedpix = -cds_output[seed_row][seed_col];
				double pixcor = -cds_output[seed_row+1][seed_col];
				double pixp = -cds_output[seed_row-1][seed_col];
				double pixl = -cds_output[seed_row][seed_col-1];
				double pixr = -cds_output[seed_row][seed_col+1];
				m_pix_nor_prf->Fill(seedpix, pixp);
				m_pix_nor_prf->Fill(seedpix, pixl);
				m_pix_nor_prf->Fill(seedpix, pixr);
				m_pix_cor_prf->Fill(seedpix, pixcor);
				

				for(int i=0; i<100; i++){
					if(seedpix > i && seedpix <= i+1){
						pix_diff = pix_mean_cor[i] - pix_mean_nor[i];
					}
				}

			}






			// 3x3 matrix
			for(int i = seed_row-1; i<=seed_row+1; i++){
				for(int j = seed_col-1; j<=seed_col+1; j++){
					cds9_crt[count] = cds_output[i][j];
					cds9_raw[count] = cds_output_raw[i][j];
					snr9_crt[count] = cds_snr[i][j];
					snr9_raw[count] = cds_snr_raw[i][j];
					count++;
				}
			}
			count =0;
			
			double mtx_sum_raw = 0, mtx_sum_crt = 0, snr_sum_raw =0, snr_sum_crt =0;
			//fill the matrix: 1, 3x3, 5x5
			// Seed
			m_sum_raw[0] ->Fill(-cds_output_raw[seed_row][seed_col]);
			m_snr_raw[0] ->Fill(-cds_snr_raw[seed_row][seed_col]);
			m_sum_crt[0] ->Fill(-cds_output[seed_row][seed_col]);
			m_snr_crt[0] ->Fill(-cds_snr[seed_row][seed_col]);
			
			//3x3
			for(int i=0; i<9; i++){
				mtx_sum_raw += cds9_raw[i];
				mtx_sum_crt += cds9_crt[i];
				snr_sum_raw += snr9_raw[i];
				snr_sum_crt += snr9_crt[i];
			}
			m_sum_raw[1] ->Fill(-mtx_sum_raw);
			m_sum_crt[1] ->Fill(-mtx_sum_crt);
			m_snr_raw[1] ->Fill(-snr_sum_raw);
			m_snr_crt[1] ->Fill(-snr_sum_crt);
			mtx_sum_raw =0; mtx_sum_crt =0;
			snr_sum_raw =0; snr_sum_crt =0;
			//5x5
			for(int i =0; i< 25; i++){
				mtx_sum_raw += cds25_raw[i];
				mtx_sum_crt += cds25_crt[i];
				snr_sum_raw += snr25_raw[i];
				snr_sum_crt += snr25_crt[i];

				
			}
			m_sum_raw[2]->Fill(-mtx_sum_raw);
			m_sum_crt[2]->Fill(-mtx_sum_crt);
			m_snr_raw[2]->Fill(-snr_sum_raw);
			m_snr_crt[2]->Fill(-snr_sum_crt);
			mtx_sum_raw =0; mtx_sum_crt =0;
			snr_sum_raw =0; snr_sum_crt =0;
	
		// fill the profile of pixels vs. CDS_seed

		for(int i = 0; i < 5; i++){
			for(int j =0; j<5; j++){
				m_prf_matrix[i][j] ->Fill(-cds_output_raw[seed_row][seed_col], -cds25_raw[i*5+j]);
				m_prf_matrix_crt[i][j] ->Fill(-cds_output[seed_row][seed_col], -cds25_crt[i*5+j]);
			}
		}



		
		//fill the fired pixel	
	//	if(npixs == 1)
			m_seed_single ->Fill(-cds_output[seed_row][seed_col]);

			//pixel around the seed: 0-up, 1-down, 2-left, 3-right
			m_pix_prf[0] ->Fill(-cds_output_raw[seed_row][seed_col], -cds_output_raw[seed_row -1][seed_col]);
			m_pix_prf[1] ->Fill(-cds_output_raw[seed_row][seed_col], -cds_output_raw[seed_row +1][seed_col]);
			m_pix_prf[2] ->Fill(-cds_output_raw[seed_row][seed_col], -cds_output_raw[seed_row][seed_col-1]);
			m_pix_prf[3] ->Fill(-cds_output_raw[seed_row][seed_col], -cds_output_raw[seed_row][seed_col+1]);
			// for corrected PIX_F
			m_pix2_correct->Fill(-cds_output_raw[seed_row][seed_col], -cds_output[seed_row+1][seed_col]);



			//find the most significant charge contribution pixels
			sort(snr25_raw, snr25_raw+25);
			int matrix_row[25] = {0}, matrix_col[25] = {0};
			for(int k =0; k <25; k++){
				for(int i =0; i<5; i++){
					for(int j=0; j<5; j++){
						if(cds_snr_raw[seed_row + i -2][seed_col + j -2] == snr25_raw[k]){
							matrix_row[k] = i;
						    matrix_col[k] = j;	
						}
						
					}

				}
				

			}

			for(int i = 0; i<25; i++){
	//			if(snr25_raw[i] > -(cds_x) ) continue;
				for(int j =0; j<=i; j++){
					
					m_cluster_shape[i] ->Fill(matrix_col[j]+0.5, matrix_row[j]+0.5);
				}
			}


//_____________________________________________________________________________________________
			// reconstruction algorithm     --Long LI 2021-03-21
			double pxl_nor, distance;
			for(int i=0; i<NROWS; i++){
				for(int j=0; j<NCOLS; j++){
					pxl_nor = -cds_snr[i][j];
					distance = sqrt((i-seed_row)*(i-seed_row) + (j-seed_col)*(j-seed_col));
					m_snr_map->Fill(distance, pxl_nor);
					m_snr_prf->Fill(distance, pxl_nor);
				}

			}
			
			
			
			// pixel sum and  size in 3x3 array
			double pix_sum=0, clu_size=0, pxl_dist=0;
			for(int k=0; k<7; k++){	
				for(int i=seed_row-1; i<=seed_row+1; i++){
					for(int j=seed_col-1; j<=seed_col+1; j++){
						if(-cds_snr[i][j] >(k+2)){
							pix_sum += -cds_output[i][j];
							clu_size+=1;
							pxl_dist = sqrt((i-seed_row)*(i-seed_row) + (j-seed_col)*(j-seed_col));
							m_distance[k]->Fill(pxl_dist);
						}
							
					}
				}

				m_pix_sum[k] ->Fill(pix_sum);
				m_clu_size[k] ->Fill(clu_size);
				pix_sum=0; clu_size=0;
			}

			
			// opt in 3x3 pixel array
			sort(cds9_crt, cds9_crt+9);
			sort(snr9_crt, snr9_crt+9);
			// position the pixels 
			int mat9_row[9], mat9_col[9];
			for(int k=0; k<9; k++){
				for(int i= seed_row-1; i<=seed_row+1; i++){
					for(int j=seed_col-1; j<=seed_col+1; j++){
						
						if(cds_output[i][j] == cds9_crt[k]){
							mat9_row[k] = i;
							mat9_col[k] = j;
//							cout<<"DEBUG: _9: K="<<k<<" ROW="<<i<<" COL="<<j<<endl;

						}
						
						}
					}
				}


			int row_snr[9], col_snr[9];
			for(int k=0; k<9; k++){
				for(int i=seed_row-1; i<=seed_row+1; i++){
					for(int j=seed_col-1; j<=seed_col+1; j++){
						if(cds_snr[i][j] == snr9_crt[k]){
							row_snr[k] = i;
							col_snr[k] = j;
						}
					}
				}
			}





				if(sizeof(mat9_row)/sizeof(mat9_row[0]) !=9)
				cout<<"DEBUG: _9 SIZE="<<sizeof(mat9_row)/sizeof(mat9_row[0])<<endl;	

			//Fill the charge and SNR in 3x3 array
			double sum_9 =0, snr_9=0, noise_cum2=0, noise_single=0;
		   	int row_9=0, col_9=0;	
			for(int i =0; i<9; i++){
				sum_9 += -cds9_crt[i];
				m_clst_q[0]->Fill(i+1, sum_9);

				
				row_9 = mat9_row[i]; col_9 = mat9_col[i];
				noise_single = m_runinfo->cds_sigma[row_9][col_9];
//				cout<<"DEBUG: _9 "<<i<<" ROW: "<<row_9<<" COL "
//					<<col_9<<" SIGMA: "<<m_runinfo->cds_sigma[row_9][col_9]<<endl;

				noise_cum2 += noise_single*noise_single;
				snr_9 = sum_9/(sqrt(noise_cum2));
				m_clst_snr[0]->Fill(i+1, snr_9);

				m_clst_snr[2]->Fill(i+1, sum_9/(sigma_mean*sqrt(i+1)));
				
				}
			sum_9 =0; snr_9=0; noise_cum2=0; noise_single=0; row_9=0; col_9=0;
		



			int row_opt[7][9], col_opt[7][9];
			for(int i=0; i<7; i++){
				for(int j=0; j<9; j++){
					row_opt[i][j]=-1;
					col_opt[i][j]=-1;
				}
			}


			double sum_fix=0, snr_fix=0, noise_fix=0;
			int size_fix =0;
			for(int k=0; k<7; k++){
				for(int i=0; i<9; i++){
					row_9 = row_snr[i]; col_9 = col_snr[i];
					sum_9 += -cds_output[row_9][col_9];
				
				
					m_clst_q[1]->Fill(i+1, sum_9);


					noise_single = m_runinfo->cds_sigma[row_9][col_9];
					noise_cum2 += noise_single*noise_single;
					snr_9 = sum_9/(sqrt(noise_cum2));
					m_clst_snr[1] ->Fill(i+1, snr_9);

					
					if(-cds_snr[row_9][col_9] > k+2){
						sum_fix += -cds_output[row_9][col_9];

						noise_single = m_runinfo->cds_sigma[row_9][col_9];
						noise_fix += noise_single*noise_single;
						snr_fix = sum_fix/sqrt(noise_fix);
						size_fix++;
						
						m_sum_fixed[k]->Fill(size_fix, sum_fix);
						m_snr_fixed[k]->Fill(size_fix, snr_fix);

					}

					
				}
				
				m_tot_fixed[k]->Fill(sum_fix);
				m_noise_fixed[k]->Fill(sqrt(noise_fix));
				m_hist_snr_fixed[k]->Fill(snr_fix);
				m_size_fixed[k]->Fill(size_fix);
				sum_fix =0; snr_fix =0; size_fix =0; noise_fix =0;


				sum_9 =0; snr_9=0; noise_cum2=0; noise_single=0; row_9=0; col_9=0;
			}	


			double snr_cum[7][9], snr_origin[7][9];
			for(int i=0; i<7; i++){
				for(int j=0; j<9; j++){
					snr_cum[i][j] = -1;
					snr_origin[i][j] = -1;
				}
			}
			// SNR opt
			for(int k=0; k<7; k++){
				for(int i =0; i<9; i++){
				
					row_9 = row_opt[k][i];
					col_9 = col_opt[k][i];
					if(row_9 != -1 && col_9 != -1){
						sum_9 += -cds_output[row_9][col_9];

						noise_single = m_runinfo->cds_sigma[row_9][col_9];
						noise_cum2 += noise_single*noise_single;
						snr_9 = sum_9/(sqrt(noise_cum2));
						snr_cum[k][i] = -snr_9;	
						snr_origin[k][i] = -snr_9;	
					}
				}
				m_snr_opt[k]->Fill(snr_9);
				sum_9 =0; snr_9=0; noise_cum2=0; noise_single=0; row_9=0; col_9=0;
			}
			
		// sum size and distance of optimized 
			int clst_size_opt[7];
			for(int i=0; i<7; i++){
				clst_size_opt[i] = -1;
			}
			for(int k=0; k<7; k++){
				sort(snr_cum[k], snr_cum[k]+9);
				m_snrdis_opt[k]->Fill(-snr_cum[k][0]);
				for(int i=0; i<9; i++){
					if(snr_origin[k][i] == snr_cum[k][0]){
						clst_size_opt[k] = i;
					//	cout<<"DEBUG: _SIZE="<<i<<endl;
						m_size_opt[k]->Fill(i+1);
					}
				}

			}
			
			for(int k=0; k<7; k++){
				if(clst_size_opt[k] == -1) continue;
				for(int i=0; i<=clst_size_opt[k]; i++){
					row_9 = row_opt[k][i];
					col_9 = col_opt[k][i];

					// pixel sum
					sum_9 += -cds_output[row_9][col_9];
					
					// diatance
					pxl_dist = sqrt((row_9 - seed_row)*(row_9 - seed_row) + 
							(col_9 - seed_col)*(col_9 - seed_col));
					m_dist_opt[k]->Fill(pxl_dist);		
				
				}
				
				m_sum_opt[k]->Fill(sum_9);
				sum_9 =0; row_9 =0; col_9 = 0; pxl_dist = 0;
			}


	


//______________________Exxtrapolation___Method____________________________________________________


		// copy the raw pixel
	double cds_cpy[NROWS][NCOLS], snr_cpy[NROWS][NCOLS];
	int firemax[20];
	int firecnt = 0;	
	int row_alg[20][49], col_alg[20][49];
	// extrapolation
	int num_alg =0;
	int row_cntr = 0, col_cntr =0;	
	int cluster_num =0;
	bool cluster_last = true;
	// select all potential cluster   --2021-04-14

	for(int i=0; i<NROWS; i++){
		 for(int j=0; j<NCOLS; j++){
			cds_cpy[i][j] = cds_output[i][j];
			snr_cpy[i][j] = cds_snr[i][j];
		 
		 }
	
	}


	while(cluster_last){
	
	// reset the index for each cluster in a frame
	for(int k=0; k<20; k++){
		for(int i=0; i<49; i++){
			row_alg[k][i] = -1;
			col_alg[k][i] = -1;
			}
		}
		 		 
		// find the central pixel in each cluster for multi-events 
		int row_center =0, col_center=0;
		if(cluster_num ==0){
			row_center = seed_row;
			col_center = seed_col;
		}
		else{
			for(int i =0; i<NROWS; i++){
				for(int j=0; j<NCOLS; j++){
					if(snr_cpy[i][j] < snr_cpy[row_center][col_center]){
						row_center = i;
						col_center = j;
					}
				}
			}
		
		}

		if(-snr_cpy[row_center][col_center] < 5){
			cluster_last = false;
			continue;
		}

		// multi-events analysis with threshold 5 		2021-04-14
		const int MULTI_CUT = 5;
		int row_multi[49], col_multi[49];
		num_alg = 0;
		for(int i=0; i<49; i++){
			row_multi[i] = -1;
			col_multi[i] = -1;
		}
		
		
		
		for(int i =row_center-1; i<=row_center+1; i++){
			for(int j=col_center-1; j<=col_center+1; j++){
				//1st	
				if(-snr_cpy[i][j] > MULTI_CUT){
					row_multi[num_alg] = i; 
					col_multi[num_alg] = j;
			   	num_alg++;

				  	row_cntr = i; col_cntr =j;
				  	snr_cpy[i][j] = 0;

				  for(int a =row_cntr-1; a<=row_cntr+1; a++){
					  for(int b=col_cntr-1; b<=col_cntr+1; b++){
						  if(a<0 || a>63 || b<8 || b>15) continue;
						  //2nd
						  if(-snr_cpy[a][b] > MULTI_CUT){
							  row_multi[num_alg] = a;
							  col_multi[num_alg] = b;
							  num_alg++;

							  row_cntr = a; col_cntr = b;
							  snr_cpy[a][b] = 0;


							  for(int c =row_cntr-1; c<=row_cntr+1; c++){
								  for(int d=col_cntr-1; d<=col_cntr+1; d++){
									  if(c<0 || c>63 || d<8 || d>15) continue;
									  //3rd
									  if(-snr_cpy[c][d]> MULTI_CUT){
										  row_multi[num_alg] = c;
										  col_multi[num_alg] = d;
										  num_alg++;
										  snr_cpy[c][d] =0;
									  }
									  
									  
								  }
							  
							  }


						  }	
					  
					  }
				  }
					  
			  }
				
			}
		}

		num_alg =0;

		// no energy cut for multi-events
		cluster_num++;


		for(int i=0; i<49; i++){
			row_9 = row_multi[i]; col_9 = col_multi[i];
			if(row_9 ==-1 || col_9 ==-1) continue;
			num_alg++;
			sum_9 += -cds_output[row_9][col_9];
		}

		
		// energy cut of the fe-55 events

		double cut_up = 154.6+5*11.1, cut_low = 154.6-5*11.1;
		if(sum_9>cut_low && sum_9<cut_up) cluster_num++;
		
		else{
			cluster_last = false;
			continue;
		
		}
		
		if(cluster_num > 3) continue; 
		m_sum_multi[cluster_num-1]->Fill(sum_9);	
		m_size_multi[cluster_num-1]->Fill(num_alg);	
		sum_9 =0; num_alg =0;
			
		
		for(int i=0; i<49; i++){
			row_multi[i] =-1;
			col_multi[i] =-1;
		}

	}


	m_evt_num ->Fill(cluster_num);
	cluster_num =0;


		
		// start to extrapolation   LongLI 2021.12.23		
		
		bool row01 = false;	
		double cds_cpy2[NROWS][NCOLS], snr_cpy2[NROWS][NCOLS];
		for(int k =0; k<20; k++){
			
			// copy the pixels
			for(int i=0; i<NROWS; i++){
				for(int j=0; j<NCOLS; j++){
					
					 cds_cpy2[i][j] = cds_output[i][j];
					 snr_cpy2[i][j] = cds_snr[i][j];
					 
					 if(-cds_snr[i][j] > 0.2*k+1){
						if(j < 8) continue;	
						firecnt++;							
						
					}
				}
			}
			firemax[k] = firecnt;
			firecnt =0;


		for(int i=seed_row-1; i<=seed_row+1; i++){
			for(int j=seed_col-1; j<=seed_col+1; j++){
				//1st	
				if(-snr_cpy2[i][j] > 0.2*k+1){
					row_alg[k][num_alg] = i; 
					col_alg[k][num_alg] = j;
			   	num_alg++;

				  	row_cntr = i; col_cntr =j;
				  	snr_cpy2[i][j] = 0;
					
					if(i<2 && k == 10){
						row01 = true;
						continue;
					}

				  for(int a =row_cntr-1; a<=row_cntr+1; a++){
					  for(int b=col_cntr-1; b<=col_cntr+1; b++){
						  if(a<2 || a>63 || b<8 || b>15 && k== 10){
								row01 = true;
							  	continue; // row0 row1 exclude
						  }
						  //2nd
						  if(-snr_cpy2[a][b] > 0.2*k+1){
							  row_alg[k][num_alg] = a;
							  col_alg[k][num_alg] = b;
							  num_alg++;

							  row_cntr = a; col_cntr = b;
							  snr_cpy2[a][b] = 0;


							  for(int c =row_cntr-1; c<=row_cntr+1; c++){
								  for(int d=col_cntr-1; d<=col_cntr+1; d++){
									  if(c<2 || c>63 || d<8 || d>15 && k==10){
											row01 = true;  
											continue;
										}
									  //3rd
									  if(-snr_cpy2[c][d]> 0.2*k+1){
										  row_alg[k][num_alg] = c;
										  col_alg[k][num_alg] = d;
										  num_alg++;
										  snr_cpy2[c][d] =0;
									  }
									  
									  
								  }
							  
							  }


						  }	
					  
					  }
				  }

					  
			  }
				  
			  
		  }
	  }
			

		//	cout<<"THRESHOLD: "<<k<<"\t SIZE: "<<num_alg<<endl;
			num_alg = 0;
				
	}


	if(row01) continue;	
		
		num_alg =0;
		int firealg[20];
		double charge_alg[20][49] = {0}, snr_alg[20][49] = {0}, snr_alg_cpy[20][49]={0};	
		
		for(int k=0; k<20; k++){
			for(int i=0; i<49; i++){
				if(row_alg[k][i] == -1 || col_alg[k][i] == -1) continue;
				row_9 = row_alg[k][i]; col_9 = col_alg[k][i];
				snr_alg[k][num_alg] = cds_snr[row_9][col_9]; 
				snr_alg_cpy[k][num_alg] = cds_snr[row_9][col_9]; 
				num_alg++;
			}
			firealg[k] = num_alg;
			m_pxs_left[k]->Fill(firemax[k] - firealg[k]);
			num_alg =0;
		}
	

		//reset the position infomation
		int row_alg_cpy[20][49], col_alg_cpy[20][49];
		for(int k=0; k<20; k++){
			for(int i =0; i<49; i++){
				row_alg_cpy[k][i] = row_alg[k][i];
				col_alg_cpy[k][i] = col_alg[k][i];
				row_alg[k][i] = -1;
				col_alg[k][i] = -1;
			}
		}

		// pixel ordering by SNR
		for(int k=0; k<20; k++){
			sort(snr_alg[k], snr_alg[k]+49);
			for(int i=0; i<49; i++){
				if(snr_alg[k][i] ==0) continue;
				for(int m=0; m<49; m++){


					if(snr_alg_cpy[k][m] == snr_alg[k][i]){
						row_alg[k][i] = row_alg_cpy[k][m]; 
						col_alg[k][i] = col_alg_cpy[k][m]; 
					}
				
				}
			}	
		}
				


		// ADC sum and  SNR
		double snr_cum3[20][49] = {0}, snr_origin3[20][49] = {0};
		// fixed size elements
		sum_9 =0; snr_9=0; noise_cum2=0; noise_single=0; row_9=0; col_9=0, num_alg=0;		
		for(int k=0; k<20; k++){
			for(int i=0; i<49; i++){
				row_9 = row_alg[k][i]; col_9 = col_alg[k][i];
				if(row_9 ==-1 || col_9 == -1) continue;
				sum_9 += -cds_output[row_9][col_9];	

				num_alg++;
				m_sum_alg[k]->Fill(num_alg, sum_9);

				noise_single = m_runinfo->cds_sigma[row_9][col_9];
				noise_cum2 += noise_single*noise_single;
				snr_9 = sum_9/sqrt(noise_cum2);
				m_snr_alg[k]->Fill(num_alg, snr_9);

				snr_cum3[k][i] = -snr_9;
				snr_origin3[k][i] = -snr_9;

				double dist_alg3 = sqrt((row_9 - seed_row)*(row_9-seed_row) + 
						(col_9-seed_col)*(col_9-seed_col));
				m_dist_alg[k]->Fill(dist_alg3);

				

			}
			
			if(sum_9==0 || snr_9 ==0) continue;

			m_tot_alg[k]->Fill(sum_9);
			m_noise_alg[k]->Fill(sqrt(noise_cum2));
			m_snr_clu[k]->Fill(snr_9);
			m_size_alg[k]->Fill(num_alg);
			
			m_sum_extr->Fill(k+1, sum_9);
			m_snr_extr->Fill(k+1, snr_9);
	
			m_size_thres->Fill(k+1, num_alg);	
			m_sum_size_prf->Fill(num_alg, sum_9);


			double m_ratio = (double) -cds_output[seed_row][seed_col]/ (1.0 * sum_9);
			if(sum_9 > 150 && sum_9 < 170)
			m_seed_rat[k]->Fill(m_ratio);

			// fill the distance for cut = 3;
			if(k ==10){
				double dist_sum = 0;
				double dist_anal = 0;
				for(int i=0; i<49; i++){
					row_9 = row_alg[k][i]; col_9 = col_alg[k][i];
					if(row_9 == -1 || col_9 == -1) continue;
					 dist_anal = sqrt((row_9 - seed_row)*(row_9 - seed_row) + 
							  (col_9 - seed_col)*(col_9 - seed_col));

				

					if(num_alg == 2) m_size_anal[0]->Fill(dist_anal);
					else if(num_alg == 3) m_size_anal[1]->Fill(dist_anal);
					else if(num_alg == 4) m_size_anal[2]->Fill(dist_anal);
					else if(num_alg == 5) m_size_anal[3]->Fill(dist_anal);

					dist_sum += dist_anal;

				}
				
				//cluster shape analysis SIZE 3
				if(num_alg == 3){
					 //Shape 1: 	# #
					 //            *
					if(dist_sum > 2) m_shape_size3->Fill(1);

					int row_s[3], col_s[3];

					if(dist_sum == 2){
						row_s[0] = row_alg[k][0]; col_s[0] = col_alg[k][0];
						row_s[1] = row_alg[k][1]; col_s[1] = col_alg[k][1];
						row_s[2] = row_alg[k][2]; col_s[2] = col_alg[k][2];

						// Shape 2:		# * #
						//
						if(((row_s[0] == row_s[1])&&(row_s[0] == row_s[2])) || 
							((col_s[0] == col_s[1])&&(col_s[0] == col_s[2])))
							m_shape_size3->Fill(2);
						
						// Shape 3:		* #
						// 				#
						else m_shape_size3->Fill(3);
					}
				
				
				}
				
				//cluster shape analysis SIZE 4
				if(num_alg == 4){
					int row4[4], col4[4];
					//Shape 1:		#
					//				 # * #
					if(dist_sum == 3) m_shape_size4->Fill(1);
					
					//Shape 2:		*
					//				 # # #
					else if(dist_sum > 3.8) m_shape_size4->Fill(2);
	
					else if(dist_sum > 3.4 && dist_sum < 3.8){
						
						 //Shape3:	#
						 //			# * #
						row4[0] = row_alg[k][0]; col4[0] = col_alg[k][0];
						row4[1] = row_alg[k][1]; col4[1] = col_alg[k][1];
						row4[2] = row_alg[k][2]; col4[2] = col_alg[k][2];
						row4[3] = row_alg[k][3]; col4[3] = col_alg[k][3];

						bool s3 = false;
						// row same
						for(int i=1; i<4; i++){
							if(row4[i] == row4[0]){
								for(int j=1; j<4; j++){
									if(j ==i) continue;
									if(row4[j] == row4[0]) s3 = true;
								}
							
							}		
						}

						//col same
						for(int i=1; i<4; i++){
							if(col4[i] == col4[0]){
								for(int j=1; j<4; j++){
									if(j ==i) continue;
									if(col4[j] == col4[0]) s3 = true;
								}
							
							}		
						}

						if(s3) m_shape_size4->Fill(3);


						//Shape 4: 		#
						//					# *
						//					  #
						bool s4 = false;
						
						if(!s3){
							for(int i = 1; i<4; i++){
								for(int j =1; j<4; j++){
									double dist_max = sqrt((row4[j] - row4[i])*(row4[j] - row4[i]) + 
											  (col4[j] - col4[i])*(col4[j] - col4[i]));
									if(dist_max > 3) s4 = true;
								}
							}

							if(s4) m_shape_size4->Fill(4);
						}

						if((!s3) && (!s4)) m_shape_size4->Fill(5);
						

					}
						
				}
				
				dist_sum =0;



			// Hit position determination    --LongLI 2021-12-09
			int row_hit[num_alg], col_hit[num_alg];
			// reset the tag
			for(int i=0; i<num_alg; i++){
				 row_hit[i] = -1;
				 col_hit[i] = -1;
			}
			// for X-coordinate position
			//
			// find the nonredundant row-tag, col-tag  
			int row_cnt = 0, col_cnt = 0;
			for(int i=0; i<num_alg;i++){
				if(i ==0){
					row_hit[row_cnt] = row_alg[k][i];
					col_hit[col_cnt] = col_alg[k][i];
					row_cnt++;
					col_cnt++;
				}
				
				else{
					if(row_alg[k][i] != row_hit[row_cnt-1]){
						row_hit[row_cnt] = row_alg[k][i];
						row_cnt++;
					}
					
					if(col_alg[k][i] != col_hit[col_cnt-1]){
						col_hit[i] = col_alg[k][i];
						col_cnt++;
					}
				
				}
				
				//fill the ADC of pixel in cluster
				int row_q = row_alg[k][i];
				int col_q = col_alg[k][i];
				double hit_q = -cds_output[row_q][col_q];
				m_hit_q->Fill(hit_q);
			
				for(int ii =0; ii<4; ii++ ){
					if(num_alg == ii+2){
						if(i == 0) m_hit_q_seed[ii]->Fill(hit_q);
						else m_hit_q_adj[ii]->Fill(hit_q);
					}
				}




			}

			// fill the cluster projection
			m_hit_prf->Fill(row_cnt, col_cnt);

			for(int i=0; i<4; i++){
				if(num_alg == i+2) m_hit_prf_size[i]->Fill(row_cnt, col_cnt);
			}


			// X-position 
			// single X-hit
			
			sort(row_hit, row_hit + row_cnt);
			sort(col_hit, col_hit + col_cnt);

			double x_tot = -10, y_tot =-10;

			if(row_cnt == 1){
				m_hit_pos_x->Fill(0);
				x_tot = 0;
			
			}


			double q1 = 0, q2 = 0;
		   int hit_row =0, hit_col= 0;	
			double x_pos = -10, y_pos = -10;

		
			if(row_cnt > 1){

				 
			//	row_cnt % 2 == 0
				if(row_cnt%2 ==0){
					 // for charge in the pixel with small X-tag 
					for(int i = 0; i<row_cnt/2; i++){
						for(int j = 0; j<num_alg; j++){
							if(row_alg[k][j] == row_hit[i]){
								hit_row = row_alg[k][j];
								hit_col = col_alg[k][j];
								q1 += -cds_output[hit_row][hit_col];
							
							}
								 
						}
					
					}
					
					for(int i=row_cnt/2; i<row_cnt; i++){
						for(int j=0; j<num_alg; j++){
							if(row_alg[k][j] == row_hit[i]){
								hit_row = row_alg[k][j];
								hit_col = col_alg[k][j];
								q2 += -cds_output[hit_row][hit_col];
							}
						}
					}

					// Calculate the X-position from the geometrical center
					// charge ratio
					double q_ratio = (double) (q2-q1)/(1.0*(q1+q2));
					
					//charge impact on distance from hit to geometrical center
					double hit_x = (double) q_ratio*(row_cnt-1)/2.0;

					//The coordinate of the center referring to the seed
					double x_center = (double) row_hit[row_cnt/2] - 0.5 - seed_row; 

					m_hit_pos_x->Fill(x_center + hit_x);
					
					x_pos = abs(x_center + hit_x);

					x_tot = x_center + hit_x;

					for(int ii=0; ii<4; ii++){
						if(num_alg == ii+2) m_pos_x[ii]->Fill(x_center + hit_x);
					}



				}
			
				q1 =0; q2 =0;

			// row_cnt % 2 == 1
			if(row_cnt%2 == 1){
					 // for charge in the pixel with small X-tag 
					for(int i = 0; i<(row_cnt+1)/2; i++){
						for(int j = 0; j<num_alg; j++){
							if(row_alg[k][j] == row_hit[i]){
								hit_row = row_alg[k][j];
								hit_col = col_alg[k][j];
								if(i == (row_cnt-1)/2) q1 += -cds_output[hit_row][hit_col]/2.0;
								else q1 += -cds_output[hit_row][hit_col];
							
							}
								 
						}
					
					}
					
					for(int i=(row_cnt-1)/2; i<row_cnt; i++){
						for(int j=0; j<num_alg; j++){
							if(row_alg[k][j] == row_hit[i]){
								hit_row = row_alg[k][j];
								hit_col = col_alg[k][j];
								if(i == (row_cnt-1)/2) q2 += -cds_output[hit_row][hit_col]/2.0;
								else q2 += -cds_output[hit_row][hit_col];
							}
						}
					}

					// Calculate the X-position from the geometrical center
					// charge ratio
					double q_ratio = (double) (q2-q1)/(1.0*(q1+q2));
					
					//charge impact on distance from hit to geometrical center
					double hit_x = (double) q_ratio*(row_cnt-1)/2.0;

					//The coordinate of the center referring to the seed
					double x_center = (double) row_hit[(row_cnt-1)/2] - seed_row; 

					m_hit_pos_x->Fill(x_center + hit_x);

					x_pos = abs(x_center + hit_x);
					x_tot = x_center + hit_x;
				
					for(int ii=0; ii<4; ii++){
						if(num_alg == ii+2) m_pos_x[ii]->Fill(x_center+hit_x);
					}
			
			}	
				
			
			
			}

			q1 = 0; q2 = 0;

			if(col_cnt == 1){
				m_hit_pos_y->Fill(0);
				y_tot = 0;
			}


			if(col_cnt > 1){

				 
			//	row_cnt % 2 == 0
				if(col_cnt%2 ==0){
					 // for charge in the pixel with small X-tag 
					for(int i = 0; i<col_cnt/2; i++){
						for(int j = 0; j<num_alg; j++){
							if(col_alg[k][j] == col_hit[i]){
								hit_row = row_alg[k][j];
								hit_col = col_alg[k][j];
								q1 += -cds_output[hit_row][hit_col];
							
							}
								 
						}
					
					}
					
					for(int i=col_cnt/2; i<col_cnt; i++){
						for(int j=0; j<num_alg; j++){
							if(col_alg[k][j] == col_hit[i]){
								hit_row = row_alg[k][j];
								hit_col = col_alg[k][j];
								q2 += -cds_output[hit_row][hit_col];
							}
						}
					}

					// Calculate the X-position from the geometrical center
					// charge ratio
					double q_ratio = (double) (q2-q1)/(1.0*(q1+q2));
					
					//charge impact on distance from hit to geometrical center
					double hit_y = (double) q_ratio*(col_cnt-1)/2.0;

					//The coordinate of the center referring to the seed
					double y_center = (double) col_hit[col_cnt/2] - 0.5 - seed_col; 

					m_hit_pos_y->Fill(y_center + hit_y);
					
					y_pos = abs(y_center + hit_y);
					y_tot = y_center + hit_y;

					for(int ii=0; ii<4; ii++){
						if(num_alg == ii+2) m_pos_y[ii]->Fill(y_center+hit_y);
					}
				}
			
				q1 =0; q2 =0;

			// row_cnt % 2 == 1
			if(col_cnt%2 == 1){
					 // for charge in the pixel with small X-tag 
					for(int i = 0; i<(col_cnt+1)/2; i++){
						for(int j = 0; j<num_alg; j++){
							if(col_alg[k][j] == col_hit[i]){
								hit_row = row_alg[k][j];
								hit_col = col_alg[k][j];
								if(i == (col_cnt-1)/2) q1 += -cds_output[hit_row][hit_col]/2.0;
								else q1 += -cds_output[hit_row][hit_col];
							
							}
								 
						}
					
					}
					
					for(int i=(col_cnt-1)/2; i<col_cnt; i++){
						for(int j=0; j<num_alg; j++){
							if(col_alg[k][j] == col_hit[i]){
								hit_row = row_alg[k][j];
								hit_col = col_alg[k][j];
								if(i == (col_cnt-1)/2) q2 += -cds_output[hit_row][hit_col]/2.0;
								else q2 += -cds_output[hit_row][hit_col];
							}
						}
					}

					// Calculate the X-position from the geometrical center
					// charge ratio
					double q_ratio = (double) (q2-q1)/(1.0*(q1+q2));
					
					//charge impact on distance from hit to geometrical center
					double hit_y = (double) q_ratio*(col_cnt-1)/2.0;

					//The coordinate of the center referring to the seed
					double y_center = (double) col_hit[(col_cnt-1)/2]  - seed_col; 

					m_hit_pos_x->Fill(y_center + hit_y);
					y_pos = abs(y_center + hit_y);
					y_tot = y_center + hit_y;

					for(int ii=0; ii<4; ii++){
						if(num_alg == ii+2) m_pos_y[ii]->Fill(y_center+hit_y);
					}
				
			
			}	
				
			
			
			}

			q1 = 0; q2 = 0;


			for(int ii=0; ii<4; ii++){
			//	 if(x_pos == -10 || y_pos == -10) continue;
				 if(x_tot == -10 || y_tot == -10 || row_cnt ==1 || col_cnt ==1) continue;
			//	if(num_alg == ii+2) m_hit_pos_map[ii]->Fill(x_pos+0.5, y_pos+0.5);
				if(num_alg == ii+2) m_hit_pos_map[ii]->Fill(x_tot, y_tot);
			}
			
			if(x_tot != -10 && y_tot != -10) m_hit_pos_tot->Fill(x_tot, y_tot);


			if(x_tot != -10 && y_tot != -10 && num_alg > 1){
				m_clu_pos_x->Fill(x_tot);
				m_clu_pos_y->Fill(y_tot);
				m_hit_clu->Fill(x_tot, y_tot);
			}
			
			
			}



			// szie cut 3~5  A8(3)
			if(num_alg == 4)
				 m_thres_cut->Fill(k+1);

			for(int j=0; j<8; j++){
			   if(num_alg-1 == j)
					 m_sum_size[k][j]->Fill(sum_9);
			}
		
			double pix_seed_cor = -cds_output[seed_row][seed_col];
			double pix_f_cor = -cds_output[seed_row+1][seed_col];
			if(k == 10 && pix_seed_cor > 15 && pix_seed_cor < 100 && pix_f_cor > 3){
				m_clu_sum_prf->Fill(pix_seed_cor, sum_9);
				double pix_diff_ratio = 100*pix_diff/(1.0*sum_9);
				m_pix_diff_ratio_prf->Fill(pix_seed_cor, pix_diff_ratio);
				m_pix_diff_ratio_s_prf->Fill(pix_seed_cor, pix_diff_ratio);

			}
		
		
		// for cluster spatial resolution using charge graviation     LongLI --2021-10-25
		double centerx = 0, centery =0;
		if(k == 10){
			for(int i=0; i<49; i++){
				int row = row_alg[k][i], col = col_alg[k][i];
				if(row == -1 || col == -1) continue;
				double x = col - seed_col;    // X-direction: COL
				double y = row - seed_row;
				double weight = -cds_output[row][col]/(1.0*sum_9);
				x *= weight;
				y *= weight;
				h_pos_x->Fill(x);
				h_pos_y->Fill(y);

				centerx +=x;
				centery += y;

			}
				h_posx_w ->Fill(centerx);
				h_posy_w ->Fill(centery);
				
				centerx = 0; centery =0;	
		}







		
		
		
			sum_9 =0; snr_9=0; noise_cum2=0; noise_single=0; row_9=0; col_9=0, num_alg=0;		

		}






		
//___________________________________________________________________end extrapolation method




//_____________________________________________________________________single frame snr
	
		double c_snr[10], row_cen[10], col_cen[10];
		double c_pix[100], c_pix_c[100], c_col[100], c_row[100];

		//init 
		for(int i =0 ; i<100; i++){
			c_row[i] = 100;
			c_col[i] = 100;
			c_pix[i] = 100;
			c_pix_c[i] = 100;
		}




		double cds_c[NROWS][NCOLS];
		for(int i =0; i<NROWS; i++){
			for(int j =0; j<NCOLS;j++){
				cds_c[i][j] = cds_output[i][j];
			}
		}

		int cnt_c =0;
		bool c_exist = false;
		double c_sum =0, c_snr_c =0, c_noise_s = 0, c_noise_c =0;
		for(int k=0; k<10; k++){
			if(k==0) {
				row_cen[k] = 0;
				col_cen[k] = 0;
		
				c_sum = -cds_output[seed_row][seed_col];

				c_noise_s = m_runinfo->cds_sigma[seed_row][seed_col];
				c_noise_c = sqrt(c_noise_s * c_noise_s);

				c_snr_c = c_sum/c_noise_c;

				c_snr[k] = c_snr_c;


			}
			//    for k >0 
			 for(int i=seed_row + row_cen[k] - 1; i<=seed_row + row_cen[k] + 1; i++){
				for(int j=seed_col + col_cen[k] - 1; j<=seed_col + col_cen[k] + 1;j++){
					for(int a =0; a<100; a++){
						if(i == c_row[a] || j == c_col[a])
							 c_exist = true;
					}
					if(c_exist) continue;
					 c_pix[cnt_c] = cds_c[i][j];
					 c_pix_c[cnt_c] = cds_c[i][j];
					 c_row[cnt_c] = i;
					 c_col[cnt_c] = j;
					 cds_c[i][j] = 100;
 					 cnt_c++;
				}
			}
			
			 // sort 
			 sort(c_pix, c_pix+cnt_c-1);
			
			//find the lagerst output (-) index
			for(int i =0; i<cnt_c-1; i++){
				 if(c_row[i] == 100 || c_col[i] == 100)
					  continue;
				if(c_pix_c[i] == c_pix[0]){
					row_cen[k+1] = c_row[i];
					col_cen[k+1] = c_col[i];
					c_row[i] = 100;
					c_col[i] = 100;
					break;
					
				}					 
			}


		}

//____________________________________________________________________________________________end single frame



//_________________________________________________________________________________________________

		}


		}




		
	    
	    m_fid->Fill(fid + 0.5);
	    m_npixs->Fill(npixs);
	    m_trig_cds_sum->Fill(cds_sum);
	    m_trig_cds_npixs->Fill(cds_sum, npixs);
	    m_dTevt->Fill((double)(frame - frame_last) );
	    if (1 == frame - frame_last) {
	       nconsecutives++;
	       if (nconsecutives % 100 == 1) {
		  cout << "DEBUG"
		       << " nconsecutives=" << nconsecutives
		       << " frame=" << frame
		       << " trig=" << (short)trig
		       << " chi2_cds=" << chi2_cds
		       << " npixs=" << npixs << ":"
		     ;
		  for (int i=0; i < npixs; i++) {
		     int row = pixid[i] >> NBITS_COL;
		     int col = pixid[i] &  MASK_COL;
		     double cds_cor = pixel_cds[row][col] - m_runinfo->cds_mean[row][col];
		     cds_cor /= m_runinfo->cds_sigma[row][col];	// normalize by sigma
		     cout << " [" << row << " " << col << " " << cds_cor << "]";
		  }
		  cout << endl;
	       }
	    }
	    frame_last = frame;
	 }
      }

      if (trig > 1 && trig < TRIG_PRE)	m_chi2_trig->Fill(chi2_cds);
      else				m_chi2_cds->Fill(chi2_cds);
	 
      // next frame
      if (ientry%10000 == 0)
	 cout << __PRETTY_FUNCTION__
	      << " entry = " << ientry << "/" << jentry
	      << " bytes = " << nb << "/" << nbytes
	      << endl;
   }

   cout << __PRETTY_FUNCTION__ << " END:"
	<< " Total entries = " << nentries
	<< " nhits=" << nhits
	<< " nconsecutives=" << nconsecutives
	<< endl;

   timer.stop();
   timer.print(1);
   // fChain->GetUserInfo()->Dump();
}

// after a full waveform found
//______________________________________________________________________
void SupixAnly::fill_waveform()
{
   static int ngfs = 0;
   int npx = m_nwave_frames;
   double xp[npx]
      , ycds_sum[npx], yadc_sum[npx]
      , ycds_trig[npx]
      , ychi2_cds[npx]
      ;
   
   if (m_exceedings)
      cout << "waveform #" //<< nwaves << ":"
	   << " exceeding " << m_exceedings << " frames"
	   << " max=" << m_nwave_max
	   << endl;
   
   for (int i = 0; i < m_nwave_frames; i++) {
      m_waveform_cor->Fill(i - m_nwave_trig, m_wave_cds_frame[i]);

      xp[i] = i - m_nwave_trig;
      ycds_trig[i] = m_wave_cds_trig[i];
      ycds_sum[i] = m_wave_cds_frame[i];
      yadc_sum[i] = m_wave_adc_frame[i];
      ychi2_cds[i] = m_wave_chi2_cds[i];
   }

   // selected pixel's
   if (ngfs < MAXGRAPHS && (m_pix_fired || m_continuous) ) {		// max graphs
      ngfs++;
      ostringstream oss;
      TGraph* gf;

      // a frame of pixels ADC
      for (int j=0; j < NPIXS; j++) {
	 double yp[npx];
	 for (int i=0; i < npx; i++)
	    yp[i] = m_wave_adc_pixs[j + NPIXS*i];
	 gf = new TGraph(npx, xp, yp);
	 m_mg_adc_pixs[j].Add(gf);
	 oss.str("");
	 oss << "ADC pixel[" << j/NCOLS << ", " << j%NCOLS << "]"
	     << ";frame;pixel ADC normalized"
	    ;
	 gf->SetTitle(oss.str().c_str() );
	 gf->GetXaxis()->SetLimits(-10-m_pre_trigs, m_nwave_frames-m_nwave_trig+10);
      }

      // a frame of pixels CDS
      for (int j=0; j < NPIXS; j++) {
	 double yp[npx];
	 for (int i=0; i < npx; i++)
	    yp[i] = m_wave_cds_pixs[j + NPIXS*i];
	 gf = new TGraph(npx, xp, yp);
	 m_mg_cds_pixs[j].Add(gf);
	 oss.str("");
	 oss << "CDS pixel[" << j/NCOLS << ", " << j%NCOLS << "]"
	     << ";frame;pixel CDS normalized"
	    ;
	 gf->SetTitle(oss.str().c_str() );
	 gf->GetXaxis()->SetLimits(-10-m_pre_trigs, m_nwave_frames-m_nwave_trig+10);
      }
      
      // CDS frame
      gf = new TGraph(npx, xp, ycds_sum);
      m_mg_cds_frame->Add(gf);
      oss.str("");
      oss << "sum CDS frame:"
	  << " F=" << m_frame_start << "-" << m_frame_start + m_nwave_frames - 1
	  << " trig=" << m_wave_trig
	  << " npixs=" << m_npixs_trig
	  << ";frame;sum CDS of all pixels"
	 ;
      gf->SetTitle(oss.str().c_str() );
      gf->GetXaxis()->SetLimits(-10-m_pre_trigs, m_nwave_frames-m_nwave_trig+10);
      
      // ADC frame
      gf = new TGraph(npx, xp, yadc_sum);
      m_mg_adc_frame->Add(gf);
      oss.str("");
      oss << "sum ADC frame:"
	  << " F=" << m_frame_start << "-" << m_frame_start + m_nwave_frames - 1
	  << " trig=" << m_wave_trig
	  << " npixs=" << m_npixs_trig
	  << ";frame;sum ADC normalized"
	 ;
      gf->SetTitle(oss.str().c_str() );
      gf->GetXaxis()->SetLimits(-10-m_pre_trigs, m_nwave_frames-m_nwave_trig+10);
      
      // CDS trig
      gf = new TGraph(npx, xp, ycds_trig);
      m_mg_cds_trig->Add(gf);
      oss.str("");
      oss << "sum CDS tirg:"
	  << " F=" << m_frame_start << "-" << m_frame_start + m_nwave_frames - 1
	  << " trig=" << m_wave_trig
	  << " npixs=" << m_npixs_trig
	  << ";frame;sum CDS of pixels > "
	  << m_runinfo->trig_cds_x << "#sigma:"
	 ;
      gf->SetTitle(oss.str().c_str() );
      gf->GetXaxis()->SetLimits(-10-m_pre_trigs, m_nwave_frames-m_nwave_trig+10);
      
      // chi2 CDS
      gf = new TGraph(npx, xp, ychi2_cds);
      m_mg_chi2_cds->Add(gf);
      oss.str("");
      oss << "#chi^{2}_{CDS}:"
	  << " F=" << m_frame_start << "-" << m_frame_start + m_nwave_frames - 1
	  << " trig=" << m_wave_trig
	  << " npixs=" << m_npixs_trig
	  << ";frame;#chi^{2}_{CDS}"
	 ;
      gf->SetTitle(oss.str().c_str() );
      gf->GetXaxis()->SetLimits(-10-m_pre_trigs, m_nwave_frames-m_nwave_trig+10);

   }
   
   if (
#ifdef DEBUG
       true ||
#endif
       m_nwave_trig > m_pre_trigs) {	// should not happen!!!
      LOG << "#frame=" << m_frame_start
	  << " waveform Nframes=" << m_nwave_frames
	  << " trig=" << m_nwave_trig
	  << ":";
#ifdef DEBUG
      for (int i=0; i < m_nwave_frames; i++)
	 cout << setw(6) << m_wave_cds_frame[i];
#endif
      cout << endl;
   }
   
}

// build waveforms
// - the same structure as Loop()
// - pixel [row, col]
//   < 0 : any from pixid[] or 0
//______________________________________________________________________
void SupixAnly::build_waveform(Long64_t nentries, int row, int col)
{
   if (fChain == 0) return;
   Timer timer(__func__);
   timer.start();
   
   if (! m_booked)
      Book();

   if (nentries == 0)
      nentries = fChain->GetEntriesFast();

   //
   // before starting the global loop
   //
   ULong64_t frame_last = -1;	// for determing start of a new waveform
   // long nhits = 0;
   trig_t trig_last = 0;
   // bool newwave = true;
   long ntrigs_after = 0;	// sum of trigs after the first one in a waveform
   int	nwave_max = 0;		// max frames of a waveform

   m_nwaves = 0;
   m_nwave_frames = 0;		// for continuous mode
   m_nwave_trig = 0;
   
   Long64_t nbytes = 0, nb = 0;
   for (Long64_t jentry=0; jentry<nentries;jentry++) {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);
      nbytes += nb;

      //--------------------------------------->> start here

      // start of a new waveform
      if (frame - frame_last != 1			// discontinued #frame
	  || (trig_last == 0 && trig == TRIG_PRE)	// trig: post -> pre
	  || (m_continuous && m_nwave_frames % NWAVE_MAX == 0)	// continuous daq mode
	  )
      {//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++start a new waveform
	 if (m_nwaves >= m_nwaves_max)
	    break;
	 
	 if (m_nwaves % 10 == 0)
	    cout << __PRETTY_FUNCTION__ << " new waveform"
		 << " #" << m_nwaves
		 << " @ frame=" << frame
		 << endl;
	 
	 // newwave = true;
	 if (m_nwaves) {		// fill last waveform if existing
	    fill_waveform();
	 }
	 m_nwaves++;
	 
	 // after fill_waveform()
	 // - reset for a new waveform
	 m_nwave_frames = 0;		// count of frames, frame-index
	 m_exceedings = 0;		// exceeded frames
	 m_npixs_trig = npixs;		// npixs of triged frame
	 m_frame_start = frame;		// start #frame

	 // select a pixel
	 m_pix_row = row;
	 m_pix_col = col;
	 m_pix_fired = false;
	 for (int i=0; i < npixs; i++) {
	    int _row = pixid[i] >> NBITS_COL;
	    int _col = pixid[i] &  MASK_COL;
	    if (row < 0 && col < 0) {	// first fired pixel
	       m_pix_row = _row;
	       m_pix_col = _col;
	       m_pix_fired = true;
	       break;
	    }
	    else if (row < 0) {	// col pre-selected
	       if (_col == col) {
		  m_pix_row = _row;
		  m_pix_fired = true;
		  break;
	       }
	    }
	    else if (col < 0) {	// col pre-selected
	       if (_row == row) {
		  m_pix_col = _col;
		  m_pix_fired = true;
		  break;
	       }
	    }
	    else {
	       if (_row == row && _col == col) {
		  m_pix_row = _row;
		  m_pix_col = _col;
		  m_pix_fired = true;
		  break;
	       }
	    }
	 }
	 // in case no matched pixel found
	 if (m_pix_row < 0) m_pix_row = 0;
	 if (m_pix_col < 0) m_pix_col = 0;
	 
      }//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~end a new waveform
      
      m_wave_adc_frame[m_nwave_frames] = 0;
      m_wave_cds_trig[m_nwave_frames] = 0;
      m_wave_cds_frame[m_nwave_frames] = 0;
      m_wave_chi2_cds[m_nwave_frames] = 0;

      // fired pixels
      for (int i=0; i < npixs; i++) {
	 int row = pixid[i] >> NBITS_COL;
	 int col = pixid[i] &  MASK_COL;
	 double cds_cor = pixel_cds[row][col] - m_runinfo->cds_mean[row][col];
	 cds_cor /= m_runinfo->cds_sigma[row][col];	// normalize by sigma
	 m_wave_cds_trig[m_nwave_frames] += cds_cor;
      }

      
      // of triged frame
      if (true_trig(trig) ) {
	 // first triged frame
	 if (trig_last == TRIG_PRE) {		// pre -> trig
	    m_nwave_trig = m_nwave_frames;	// triged frame-index
	    m_wave_trig = trig;			// triged pattern
	 }
	 else
	    ntrigs_after++;
      }
      
      // pixel-wise histograms
      adc_t*	padc = (adc_t*)pixel_adc;
      cds_t*	pcds = (cds_t*)pixel_cds;
      double*	pthr = (double*)(m_runinfo->trig_cds);
      double*	pcds_mean = (double*)(m_runinfo->cds_mean);
      double*	pcds_sigma = (double*)(m_runinfo->cds_sigma);
      double*	padc_mean = (double*)(m_runinfo->adc_mean);
      double*	padc_sigma = (double*)(m_runinfo->adc_sigma);
      double	cds_cor, adc_cor;
      for (int i=0; i < NROWS; i++) {
	 for (int j=0; j < NCOLS; j++) {
	    cds_cor = *pcds - *pcds_mean;
	    cds_cor /= *pcds_sigma;		// normalized
	    adc_cor = *padc - *padc_mean;
	    adc_cor /= *padc_sigma;		// normalized

	    m_wave_cds_frame[m_nwave_frames] += cds_cor;
	    m_wave_adc_frame[m_nwave_frames] += adc_cor;
	    m_wave_adc_pixs[j+i*NCOLS + NPIXS*m_nwave_frames] = adc_cor;
	    m_wave_cds_pixs[j+i*NCOLS + NPIXS*m_nwave_frames] = cds_cor;
	    m_wave_chi2_cds[m_nwave_frames] += cds_cor*cds_cor;
	    
	    // next pixel
	    padc++;
	    pcds++;
	    pthr++;
	    pcds_mean++;
	    pcds_sigma++;
	    padc_mean++;
	    padc_sigma++;
	 }
      }//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~end loop of a frame

      // next frame
      frame_last = frame;
      trig_last = trig;
      // boundary check
      if (m_nwave_frames < m_nwave_max) {
	 m_nwave_frames++;
      }
      else {
	 m_exceedings++;
      }
      if (nwave_max < m_nwave_frames)
	 nwave_max = m_nwave_frames;
      
      //cout<<"Total entries = "<<nentries<<endl;
   }

   // last waveform
   if (m_nwaves) {
      fill_waveform();
      // write_waveform();
   }
   
   m_waveform_cor->SetAxisRange(m_waveform_cor->GetXaxis()->GetXmin(), nwave_max-m_pre_trigs);

   cout << __PRETTY_FUNCTION__ << " END:"
	<< " Total entries = " << nentries
	<< " nwave_max=" << m_nwave_max
	<< " m_nwaves=" << m_nwaves
	<< " ntrigs_after=" << ntrigs_after
	<< ( m_runinfo->ntrigs - m_nwaves - ntrigs_after == 0 ? "" : " NOT" )
	<< " consistent with RunInfo.ntrigs=" << m_runinfo->ntrigs
	<< endl;

   timer.stop();
   timer.print(1);
}

// as size_t
//______________________________________________________________________
void SupixAnly::event_display(long ievt)
{
   //   In a ROOT session, you can do:
   //      Root > .L SupixTree.C
   //      Root > SupixTree t
   //      Root > t.GetEntry(12); // Fill t data members with entry number 12
   //      Root > t.Show();       // Show values of entry 12
   //      Root > t.Show(16);     // Read and show values of entry 16
   //      Root > t.Loop();       // Loop on all entries
   //

   //     This is the loop skeleton where:
   //    jentry is the global entry number in the chain
   //    ientry is the entry number in the current Tree
   //  Note that the argument to GetEntry must be:
   //    jentry for TChain::GetEntry
   //    ientry for TTree::GetEntry and TBranch::GetEntry
   //
   //       To read only selected branches, Insert statements like:
   // METHOD1:
   //    fChain->SetBranchStatus("*",0);  // disable all branches
   //    fChain->SetBranchStatus("branchname",1);  // activate branchname
   // METHOD2: replace line
   //    fChain->GetEntry(jentry);       //read all branches
   //by  b_branchname->GetEntry(ientry); //read only this branch
   if (fChain == 0) return;

   Long64_t nentries = fChain->GetEntriesFast();

   // fChain->SetBranchStatus("*",0);		// disable all branches
   // fChain->SetBranchStatus("npixs",1);		// activate branchname
   // fChain->SetBranchStatus("pixel_cds",1);	// activate branchname

   static vector<Long64_t> jevent;	// jentry of events
   static vector<int> jcut;		// ncuts of events
   static vector<double> jcutmin;	// min sigma passing cut
   
   // pave size
   int nlines = 5;
   double text_h = 0.035;
   double x1 = 0.01;
   double xw = 0.27;
   double y1, y2 = 0.995;
   
   // first execute
   // - scan for potential events
   if (jevent.size() == 0) {
      Long64_t nbytes = 0, nb = 0;
      for (Long64_t jentry=0; jentry<nentries;jentry++) {
	 Long64_t ientry = LoadTree(jentry);
	 if (ientry < 0) break;
	 nb = fChain->GetEntry(jentry);   nbytes += nb;
	 // if (Cut(ientry) < 0) continue;
   
   	 // if not physics trigged, continue;
	 if(trig != 2) continue; 

	 // pixel-wise histograms
	 double cds_raw[NROWS][NCOLS];
	 cds_t*		pcds = (cds_t*)pixel_cds;
	 double*	pcds_mean = (double*)(m_runinfo->cds_mean);
	 double*	pcds_sigma = (double*)(m_runinfo->cds_sigma);
	 double	cds_cor;
	 int ncuts = 0;
	 double cutmin = 999;
	 double chi2_cds = 0;
	 for (int i=0; i < NROWS; i++) {//++++++++++++++++++++++++++++++start loop a frame
	    for (int j=0; j < NCOLS; j++) {
	       cds_cor = *pcds - *pcds_mean;
	       cds_cor /= *pcds_sigma;		// normalized
	       chi2_cds += cds_cor*cds_cor;
	       
	       if (cds_cor < -g_cds_sum_x) {	// Esum???
		  ncuts++;
		  if (cutmin > -cds_cor)
		     cutmin = -cds_cor;
	       }
			//record the raw pixel
			cds_raw[i][j] = *pcds;

	       // next pixel
	       pcds++;
	       pcds_mean++;
	       pcds_sigma++;
	    }
	 }//------------------------------------------------------------end loop a frame

	
//	 if (! (npixs > 0 || ncuts > 10 || chi2_cds > g_chi2_cds_x) ) continue;
	//select the output sum over 1000
	double adc_sum =0;
	int seed_row =0, seed_col =0;
   for (int i=0; i < npixs; i++) {
      int row = pixid[i] >> NBITS_COL;
      int col = pixid[i] &  MASK_COL;
		if(cds_raw[row][col] < cds_raw[seed_row][seed_col]){
			seed_row = row;
			seed_col = col;
		}
      adc_sum += -cds_raw[row][col];
   }
	if(npixs < 12) continue;
	if(seed_row<1 || seed_row>63 || seed_col<8 || seed_col >14) continue;	
	if(adc_sum < 300) continue;
	adc_sum =0;

	 
	 // special events
	 if (chi2_cds > g_chi2_cds_x ) {
	    cout << "\tspecial #" << jevent.size()
		 << " frame=" << frame
		 << " chi2_cds=" << chi2_cds
		 << " npixs=" << npixs
		 << " ncuts=" << ncuts
		 << endl;
	 }

	 jevent.push_back(jentry);
	 jcut.push_back(ncuts);
	 jcutmin.push_back(cutmin);
      }

      // prepare plots
      //
      // event display
      int xbins, ybins;
      double xmin, xmax, ymin, ymax;
      xmin = 0, xmax = NROWS;
      xbins = xmax - xmin;
      ymin = 0, ymax = NCOLS;
      ybins = ymax - ymin;
      string hname = "event_cds";
      ostringstream oss;
      oss << ";ROW;COL;CDS";
      string htitle = oss.str();
      TH2D* h2d = (TH2D*)gDirectory->Get(hname.c_str() );
      if (h2d) delete h2d;
      h2d = new TH2D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax, ybins, ymin, ymax);
      m_event.display_cds = h2d;
      h2d->SetStats(0);

      // event information
      y1 = y2 - text_h * nlines;
      m_event.info = new TPaveText(x1, y1, x1+xw, y2, "NDC NB");
      m_event.info->SetTextAlign(12);
      x1 += xw + 0.005;
      y1 = y2 - text_h;
      m_event.pixs = new TPaveText(x1, y1, x1+xw, y2, "NDC NB");
      m_event.pixs->SetTextAlign(12);

#if ROOT_VERSION_CODE >= ROOT_VERSION(6, 0, 0)
      m_event.info->SetFillColorAlpha(9, 0.1);
      m_event.pixs->SetFillColorAlpha(9, 0.1);
#else
      m_event.info->SetFillColor(0);
      m_event.pixs->SetFillColor(0);
#endif
      
   }

   //----------------------------------------
   
   if (ievt >= (long)(jevent.size() ) ) {
      LOG << "exceeding max number of events: " << jevent.size() << endl;
      return;
   }

   Long64_t jentry = jevent.at(ievt);
   int ncuts = jcut.at(ievt);
   double cutmin = jcutmin.at(ievt);
   
   fChain->GetEntry(jentry);
   
   adc_t*	padc = (adc_t*)pixel_adc;
   cds_t*	pcds = (cds_t*)pixel_cds;
   double*	pthr = (double*)(m_runinfo->trig_cds);
   double*	pcds_mean = (double*)(m_runinfo->cds_mean);
   double*	pcds_sigma = (double*)(m_runinfo->cds_sigma);
   double*	padc_mean = (double*)(m_runinfo->adc_mean);
   double*	padc_sigma = (double*)(m_runinfo->adc_sigma);
   double	cds_cor, adc_cor;
   double	chi2_cds = 0;
   double	cds_min = 0, cds_max = 0;
   
   
   for (int i=0; i < NROWS; i++) {//++++++++++++++++++++++++++++++++++++start a frame
      for (int j=0; j < NCOLS; j++) {
	 cds_cor = *pcds - *pcds_mean;
	 cds_cor /= *pcds_sigma;		// normalized
	 adc_cor = *padc - *padc_mean;
	 adc_cor /= *padc_sigma;		// normalized

	 if (cds_min > cds_cor)	cds_min = cds_cor;
	 if (cds_max < cds_cor)	cds_max = cds_cor;
	 
	 chi2_cds += cds_cor * cds_cor;
	 
	 m_event.display_cds->SetBinContent(i+1, j+1, -cds_cor);
	 
	 // next pixel
	 padc++;
	 pcds++;
	 pthr++;
	 pcds_mean++;
	 pcds_sigma++;
	 padc_mean++;
	 padc_sigma++;
      }
   }//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~end a frame
   
   double pixcds[npixs];
   for (int i=0; i < npixs; i++) {
      int row = pixid[i] >> NBITS_COL;
      int col = pixid[i] &  MASK_COL;
      double cds_cor = pixel_cds[row][col] - m_runinfo->cds_mean[row][col];
      cds_cor /= m_runinfo->cds_sigma[row][col];	// normalize by sigma
      pixcds[i] = -cds_cor;
   }

   LOG << "total events=" << jevent.size()
       << "/" << ievt << " :";
   cout << " frame=" << frame
	<< " chi2_cds=" << chi2_cds
	<< " ncuts=" << ncuts
	<< " npixs=" << npixs
	<< " [row col cds] ="
      ;
   for (int i=0; i < npixs; i++) {
      int row = pixid[i] >> NBITS_COL;
      int col = pixid[i] &  MASK_COL;
      cout << " [" << row << " " << col << " " << pixcds[i] << "]";
   }
   cout << endl;

   // event information
   ostringstream oss;
   m_event.info->Clear();
   m_event.info->AddText(m_runinfo->time_start_str.c_str() );
   oss.str(""); oss << "chip\t" << m_runinfo->chip_addr;
   m_event.info->AddText(oss.str().c_str() );
   oss.str(""); oss << "threshold\t" << m_runinfo->trig_cds_x << " #sigma";
   m_event.info->AddText(oss.str().c_str() );
   oss.str(""); oss << "frame\t" << frame;
   m_event.info->AddText(oss.str().c_str() );
   oss.str(""); oss << "trigger\t0x" << hex << (short)trig << dec;
   m_event.info->AddText(oss.str().c_str() );

   m_event.pixs->Clear();
   y1 = y2 - text_h * (npixs + 3);	// + chi2_cds, ncuts
   m_event.pixs->SetY1NDC(y1);
   oss.str("");
   oss << "#chi^{2}_{CDS} = " << chi2_cds << "/" << g_chi2_cds_x;
   m_event.pixs->AddText(oss.str().c_str() );
   
   oss.str("");
   oss << "CDS #in [ " << cds_min << ", " << cds_max << " ]";
   m_event.pixs->AddText(oss.str().c_str() );
   
   oss.str(""); oss << npixs << " fired" << ": [row col cds]";
   m_event.pixs->AddText(oss.str().c_str() );
   for (int i=0; i < npixs/2; i++) {


 		int row = pixid[2*i] >> NBITS_COL;
      int col = pixid[2*i] &  MASK_COL;
      oss.str("");
      oss << "  " << 2*i << ":" << setw(4) << row << setw(4) << col
	  << "  " << fixed << setprecision(1) << pixcds[2*i];

		if(npixs == 1)	continue;
      
		 row = pixid[2*i+1] >> NBITS_COL;
       col = pixid[2*i+1] &  MASK_COL;
      oss << "\t\t\t\t" << 2*i+1 << ":" << setw(4) << row << setw(4) << col
	  << "  " << fixed << setprecision(1) << pixcds[2*i+1];
      m_event.pixs->AddText(oss.str().c_str() );
   }

	// the last one for odd pixels fired
	if(npixs % 2 == 1){
		int row = pixid[npixs-1] >> NBITS_COL;
      int col = pixid[npixs-1] &  MASK_COL;
      oss.str("");
		oss << "  " << npixs-1 << ":" << setw(4) << row << setw(4) << col
	  << "  " << fixed << setprecision(1) << pixcds[npixs-1];
      m_event.pixs->AddText(oss.str().c_str() );
		
	}

}

void SupixAnly::special_display(long ievt, int fired)
{
   //   In a ROOT session, you can do:
   //      Root > .L SupixTree.C
   //      Root > SupixTree t
   //      Root > t.GetEntry(12); // Fill t data members with entry number 12
   //      Root > t.Show();       // Show values of entry 12
   //      Root > t.Show(16);     // Read and show values of entry 16
   //      Root > t.Loop();       // Loop on all entries
   //

   //     This is the loop skeleton where:
   //    jentry is the global entry number in the chain
   //    ientry is the entry number in the current Tree
   //  Note that the argument to GetEntry must be:
   //    jentry for TChain::GetEntry
   //    ientry for TTree::GetEntry and TBranch::GetEntry
   //
   //       To read only selected branches, Insert statements like:
   // METHOD1:
   //    fChain->SetBranchStatus("*",0);  // disable all branches
   //    fChain->SetBranchStatus("branchname",1);  // activate branchname
   // METHOD2: replace line
   //    fChain->GetEntry(jentry);       //read all branches
   //by  b_branchname->GetEntry(ientry); //read only this branch
   if (fChain == 0) return;

   Long64_t nentries = fChain->GetEntriesFast();

   // fChain->SetBranchStatus("*",0);		// disable all branches
   // fChain->SetBranchStatus("npixs",1);		// activate branchname
   // fChain->SetBranchStatus("pixel_cds",1);	// activate branchname

   static vector<Long64_t> jevent;	// jentry of events
   static vector<int> jcut;		// ncuts of events
   static vector<double> jcutmin;	// min sigma passing cut
   
   // pave size
   int nlines = 5;
   double text_h = 0.035;
   double x1 = 0.01;
   double xw = 0.27;
   double y1, y2 = 0.995;
   
   // first execute
   // - scan for potential events
   if (jevent.size() == 0) {
      Long64_t nbytes = 0, nb = 0;
      for (Long64_t jentry=0; jentry<nentries;jentry++) {
	 Long64_t ientry = LoadTree(jentry);
	 if (ientry < 0) break;
	 nb = fChain->GetEntry(jentry);   nbytes += nb;
	 // if (Cut(ientry) < 0) continue;
      
	 // pixel-wise histograms
	 cds_t*		pcds = (cds_t*)pixel_cds;
	 double*	pcds_mean = (double*)(m_runinfo->cds_mean);
	 double*	pcds_sigma = (double*)(m_runinfo->cds_sigma);
	 double	cds_cor;
	 int ncuts = 0;
	 double cutmin = 999;
	 double chi2_cds = 0;

	//find the target frames, physics frames
	 if(npixs < fired || trig != 2) continue;


	//Find the seed
	double cds_sum = 0; 
	Int_t seed_row = 0, seed_col = 0;
	double pixel_fired[NROWS][NCOLS] = {0};
	Int_t pix_row[npixs], pix_col[npixs];
	bool special = false;
	for (int i=0; i < npixs; i++) {
	   int row = pixid[i] >> NBITS_COL;
	   int col = pixid[i] &  MASK_COL;

	   double cds_cor = pixel_cds[row][col] - m_runinfo->cds_mean[row][col];
	   cds_cor /= m_runinfo->cds_sigma[row][col];	// normalize by sigma
	
	   pixel_fired[row][col] = cds_cor;
	  
	   cds_sum += -cds_cor;

	   if(cds_cor > -1){
	    cout<<"cds_raw: "<<pixel_cds[row][col]
			<<"\tcds_mean: "<<m_runinfo->cds_mean[row][col]
			<<"\tcds_sigma: "<<m_runinfo->cds_sigma[row][col]
			<<"\tcds_cor: "<<cds_cor<<endl;
	   }




	   pix_row[i] = row;
	   pix_col[i] = col;

	   if(pixel_fired[row][col] < pixel_fired[seed_row][seed_col]){
			seed_row = row;
			seed_col = col;
		}

	}
	//for calibration peak analysis 
	//		if(cds_sum < 60) continue;
		cds_sum = 0;
	//	if (seed_col < 8 || seed_row < 1 || seed_row > 62|| pixel_fired[seed_row][seed_col]> -90/fired ) continue;

	//find pileup events
	bool pileup = false;
	double e_sum =0;
	for(int i =0; i<npixs; i++){
		if(npixs > 9 || TMath::Abs(pix_row[i] - seed_row) > 1 || TMath::Abs(pix_col[i] - seed_col) > 1){
			Int_t p_col = pix_col[i];
			Int_t p_row = pix_row[i];
			special = true;
			e_sum+= pixel_fired[p_row][p_col];
		}
	}
//	if(e_sum < -60) pileup = true;
	e_sum =0;
//	special = special && pileup;
	
	if(!special) continue;
	double pix_raw[NROWS][NCOLS];
	 for (int i=0; i < NROWS; i++) {//++++++++++++++++++++++++++++++start loop a frame
	    for (int j=0; j < NCOLS; j++) {
	       cds_cor = *pcds - *pcds_mean;
	       cds_cor /= *pcds_sigma;		// normalized
	       chi2_cds += cds_cor*cds_cor;
	       
	       if (cds_cor < -g_cds_sum_x) {	// Esum???
		  ncuts++;
		  if (cutmin > -cds_cor)
		     cutmin = -cds_cor;
	       }
	       pix_raw[i][j] = cds_cor;
	       // next pixel
	       pcds++;
	       pcds_mean++;
	       pcds_sigma++;
	    }
	 }//------------------------------------------------------------end loop a frame



	 if (! (npixs > 0 || ncuts > 10 || chi2_cds > g_chi2_cds_x) ) continue;
//	 if(!(npixs > 0)) continue;
	 // special events
	 
//	 if (chi2_cds > g_chi2_cds_x && npixs < 1) {
	 if(special){
		 cout << "\tspecial #" << jevent.size()
		 << " frame=" << frame
		 << " chi2_cds=" << chi2_cds
		 << " npixs=" << npixs
		 << " ncuts=" << ncuts
		 << endl;
	 }

	 jevent.push_back(jentry);
	 jcut.push_back(ncuts);
	 jcutmin.push_back(cutmin);
      }
	
  	  //if no special events	  
	  if(jevent.size() < 1) return;	

      // prepare plots
      //
      // event display
      int xbins, ybins;
      double xmin, xmax, ymin, ymax;
      xmin = 0, xmax = NROWS;
      xbins = xmax - xmin;
      ymin = 0, ymax = NCOLS;
      ybins = ymax - ymin;
      string hname = "event_cds";
      ostringstream oss;
      oss << ";ROW;COL;CDS";
      string htitle = oss.str();
      TH2D* h2d = (TH2D*)gDirectory->Get(hname.c_str() );
      if (h2d) delete h2d;
      h2d = new TH2D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax, ybins, ymin, ymax);
      m_event.display_cds = h2d;
      h2d->SetStats(0);

      // event information
      y1 = y2 - text_h * nlines;
      m_event.info = new TPaveText(x1, y1, x1+xw, y2, "NDC NB");
      m_event.info->SetTextAlign(12);
      x1 += xw + 0.005;
      y1 = y2 - text_h;
      m_event.pixs = new TPaveText(x1, y1, x1+xw, y2, "NDC NB");
      m_event.pixs->SetTextAlign(12);

#if ROOT_VERSION_CODE >= ROOT_VERSION(6, 0, 0)
      m_event.info->SetFillColorAlpha(9, 0.1);
      m_event.pixs->SetFillColorAlpha(9, 0.1);
#else
      m_event.info->SetFillColor(0);
      m_event.pixs->SetFillColor(0);
#endif
      
   }

   //----------------------------------------
   
   if (ievt >= (long)(jevent.size() ) ) {
      LOG << "exceeding max number of events: " << jevent.size() << endl;
      return;
   }

   Long64_t jentry = jevent.at(ievt);
   int ncuts = jcut.at(ievt);
   double cutmin = jcutmin.at(ievt);
   
   fChain->GetEntry(jentry);
   
   adc_t*	padc = (adc_t*)pixel_adc;
   cds_t*	pcds = (cds_t*)pixel_cds;
   double*	pthr = (double*)(m_runinfo->trig_cds);
   double*	pcds_mean = (double*)(m_runinfo->cds_mean);
   double*	pcds_sigma = (double*)(m_runinfo->cds_sigma);
   double*	padc_mean = (double*)(m_runinfo->adc_mean);
   double*	padc_sigma = (double*)(m_runinfo->adc_sigma);
   double	cds_cor, adc_cor;
   double	chi2_cds = 0;
   double	cds_min = 0, cds_max = 0;
   for (int i=0; i < NROWS; i++) {//++++++++++++++++++++++++++++++++++++start a frame
      for (int j=0; j < NCOLS; j++) {
	 cds_cor = *pcds - *pcds_mean;
	 cds_cor /= *pcds_sigma;		// normalized
	 adc_cor = *padc - *padc_mean;
	 adc_cor /= *padc_sigma;		// normalized

	 if (cds_min > cds_cor)	cds_min = cds_cor;
	 if (cds_max < cds_cor)	cds_max = cds_cor;
	 
	 chi2_cds += cds_cor * cds_cor;
	 
	 m_event.display_cds->SetBinContent(i+1, j+1, -cds_cor);
	 
	 // next pixel
	 padc++;
	 pcds++;
	 pthr++;
	 pcds_mean++;
	 pcds_sigma++;
	 padc_mean++;
	 padc_sigma++;
      }
   }//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~end a frame
   
   double pixcds[npixs];
   for (int i=0; i < npixs; i++) {
      int row = pixid[i] >> NBITS_COL;
      int col = pixid[i] &  MASK_COL;
      double cds_cor = pixel_cds[row][col] - m_runinfo->cds_mean[row][col];
      cds_cor /= m_runinfo->cds_sigma[row][col];	// normalize by sigma
      pixcds[i] = -cds_cor;
   }

   LOG << "total events=" << jevent.size()
       << "/" << ievt << " :";
   cout << " frame=" << frame
	<< " chi2_cds=" << chi2_cds
	<< " ncuts=" << ncuts
	<< " npixs=" << npixs
	<< " [row col cds] ="
      ;
   for (int i=0; i < npixs; i++) {
      int row = pixid[i] >> NBITS_COL;
      int col = pixid[i] &  MASK_COL;
      cout << " [" << row << " " << col << " " << pixcds[i] << "]";
   }
   cout << endl;

   // event information
   ostringstream oss;
   m_event.info->Clear();
   m_event.info->AddText(m_runinfo->time_start_str.c_str() );
   oss.str(""); oss << "chip\t" << m_runinfo->chip_addr;
   m_event.info->AddText(oss.str().c_str() );
   oss.str(""); oss << "threshold\t" << m_runinfo->trig_cds_x << " #sigma";
   m_event.info->AddText(oss.str().c_str() );
   oss.str(""); oss << "frame\t" << frame;
   m_event.info->AddText(oss.str().c_str() );
   oss.str(""); oss << "trigger\t0x" << hex << (short)trig << dec;
   m_event.info->AddText(oss.str().c_str() );

   m_event.pixs->Clear();
   y1 = y2 - text_h * (npixs + 3);	// + chi2_cds, ncuts
   m_event.pixs->SetY1NDC(y1);
   oss.str("");
   oss << "#chi^{2}_{CDS} = " << chi2_cds << "/" << g_chi2_cds_x;
   m_event.pixs->AddText(oss.str().c_str() );
   
   oss.str("");
   oss << "CDS #in [ " << cds_min << ", " << cds_max << " ]";
   m_event.pixs->AddText(oss.str().c_str() );
   
   oss.str(""); oss << npixs << " fired" << ": [row col cds]";
   m_event.pixs->AddText(oss.str().c_str() );
   for (int i=0; i < npixs; i++) {
      int row = pixid[i] >> NBITS_COL;
      int col = pixid[i] &  MASK_COL;
      oss.str("");
      oss << "  " << i << ":" << setw(4) << row << setw(4) << col
	  << "  " << fixed << setprecision(1) << pixcds[i];
      m_event.pixs->AddText(oss.str().c_str() );
   }

}

void SupixAnly::frame_display(Long64_t iframe){
	  if (fChain == 0) return;
      
      // frame display
      int xbins, ybins;
      double xmin, xmax, ymin, ymax;
      xmin = 0, xmax = NROWS;
      xbins = xmax - xmin;
      ymin = 0, ymax = NCOLS;
      ybins = ymax - ymin;
      string hname = "frame_adc";
      ostringstream oss;
      oss << ";ROW;COL;ADCu";
      string htitle = oss.str();
      TH2D* h2d = (TH2D*)gDirectory->Get(hname.c_str() );
      if (h2d) delete h2d;
      h2d = new TH2D(hname.c_str(), htitle.c_str(), xbins, xmin, xmax, ybins, ymin, ymax);
      m_iframe.display_frame = h2d;
	  h2d->SetStats(0);
   	  

      Long64_t nbytes, nb;
      Long64_t nentries = fChain->GetEntriesFast();
      if(iframe > nentries) return; 
      for(Long64_t jentry =0; jentry < nentries; jentry++){
   
      Long64_t ientry = LoadTree(jentry);
	 if (ientry < 0) return;
	 nb = fChain->GetEntry(jentry);   nbytes += nb;

	 if(frame == iframe){
	 // pixel-wise histograms
	 adc_t*		padc = (adc_t*)pixel_adc;
	 double*	padc_mean = (double*)(m_runinfo->adc_mean);
	 double*	padc_sigma = (double*)(m_runinfo->adc_sigma);
	 double*	pcds_sigma = (double*)(m_runinfo->cds_sigma);
	 double	adc_cor;
	 for (int i=0; i < NROWS; i++) {//++++++++++++++++++++++++++++++start loop a frame
	    for (int j=0; j < NCOLS; j++) {
	       adc_cor = *padc - *padc_mean;
	       adc_cor /= *pcds_sigma;		// normalized
	       
	  	   
	  	   m_iframe.display_frame -> Fill(i+1, j+1, -adc_cor);
	       // next pixel
	       padc++;
	       padc_mean++;
	       padc_sigma++;
	       pcds_sigma++;
	    }
	 }//------------------------------------------------------------end loop a frame
   
	 }

   }

}

double SupixAnly::charge_correction(double charge_seed, double charge_raw){
	
	//inverse
	double correction;
	double seed = -1*charge_seed;
	double raw = -1*charge_raw;

//	if(seed < 155 && seed > 100)
//	correction = -1 *(raw - 0.83*(0.07 * seed + 2.22))/0.73;
//	else
	// before irradiation
	// correction = -1*(raw -0.95*(0.08 * seed +1.83))/0.78;

	// after irradiation
	correction = -1*(raw - 0.91*(0.04*seed + 1.68))/0.86;
	
	
	//	cout<<raw<<"\t"<<seed<<"\t"<<correction<<endl;
	return correction;	

//	return -1*(-1*charge_raw - 1.003*(0.08 * -charge_seed + 1.27))/0.79;
}


