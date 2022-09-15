/*******************************************************************//**
 * $Id: analyser.C 1229 2020-07-22 06:13:11Z mwang $
 *
 * interface to SupixAnly.
 *
 * Usage:
 * - see README for general use
 * - check common_root.h for print utilities, etc.
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2019-04-03 14:25:55
 * @copyright:  (c)2019 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "SupixAnly.h"
#include "TF1.h"
// ROOT headers and personal utilities
#include "common_root.h"
using namespace wm;

// C/C++ headers
#include "common.h"
using namespace std;

SupixAnly *g_anly = NULL;

//======================================================================

// files: ./data/raw_..._*.root
//        0 = ./data/rawdata.root
//______________________________________________________________________
SupixAnly * analyser(const char *files="data/rawdata.root") {
   TChain *chain = 0;

   if (files) {
      // input: tree name
      chain = new TChain("supix");
      int nfiles = chain->Add(files);
      chain->Print();

      cout << "chain->Add(\"" << files << "\"): "
	   << nfiles << " files added."
	   << endl;
   }

   // LOG << " g_anly=" << g_anly << endl;
   // if (g_anly) delete g_anly;
   
   g_anly = new SupixAnly(chain);
   // gDirectory->ls();
   // g_anly->get_RunInfo();
   //g_anly->Loop();

   return g_anly;
}

// hitmap
void
hitmap()
{
   get_canvas();
   gStyle->SetOptTitle(kTRUE);		// histogram title
   gStyle->SetOptStat(11);		// entries
   gPad->SetLogy(0);
   
   TH2D* h2d = (TH2D*)gDirectory->Get("hitmap_cds");
   h2d->SetStats(1);
   h2d->GetXaxis()->SetNdivisions(-404);
   h2d->GetYaxis()->SetNdivisions(-808);
   h2d->Draw("colz");
   gPad->Update();
   // setW_pave(h2d, 0.35);		// after gPad->Update()
   
   TPaletteAxis *palette = (TPaletteAxis*)h2d->GetListOfFunctions()->FindObject("palette");
   double xw = palette->GetX2NDC() - palette->GetX1NDC();
   double x1 = 0.17;
   gPad->SetRightMargin(x1);	// D=0.1
   x1 = 1.01 - x1;
   palette->SetX1NDC(x1);	// change palette box width
   palette->SetX2NDC(x1+xw);	// change palette box width
   // h2d->GetZaxis()->SetLabelSize(0.01);	// D=0.04

   // set_stats(h2d, "ne");
   gPad->Modified();
   gPad->Update();
   LOG << h2d->GetName() << " on Pad " << gPad->GetName() << endl;

}

// data quality control
//______________________________________________________________________
void control_plots_obsolete()
{
   TCanvas* c1 = get_canvas("c1");
   
   gStyle->SetOptStat(110010);
   gStyle->SetOptTitle(kTRUE);		// histogram title

   TH1D* h1d;
   TProfile* prf;

   // pads order: up->down, left->right
   //
   int nx = 3, ny = 2;
   double ww = 1./nx, hh = 1./ny;
   double x1 = -ww, y1;
   for (int ipad=0; ipad < nx*ny; ipad++) {
      c1->cd();
      x1 += ww * (ipad % ny == 0 ? 1 : 0);
      if (ipad == 4) hh = 0.3;
      if (ipad == 5) {
	 hh = 1 - hh;
	 y1 = 0;
      }
      else
	 y1 = 1 - (1+ipad % ny) * hh;
      TString str = "pad_" + TString::Itoa(ipad, 10);
      TPad* pad = new TPad(str.Data(), str.Data(), x1, y1, x1+ww, y1+hh);
      pad->Draw();
      pad->cd();
      cout << gPad->GetName() << " ipad=" << ipad << " x1=" << x1 << " y1=" << y1 << " " << ipad/ny << endl;

      gPad->SetLogy(0);
      switch (ipad) {
      case 0:
	 h1d = (TH1D*)gDirectory->Get("fid");
	 h1d->Draw();
	 h1d->SetMinimum(0);
	 break;
      case 1:
	 h1d = (TH1D*)gDirectory->Get("trig");
	 h1d->Draw();
	 break;
      case 2:
	 h1d = (TH1D*)gDirectory->Get("adc_raw");
	 h1d->Draw();
	 break;
      case 3:
	 gStyle->SetOptStat(111110);
	 gPad->SetLogy(1);
	 h1d = (TH1D*)gDirectory->Get("cds_raw");
	 h1d->Draw();
	 break;
      case 4:
	 prf = (TProfile*)gDirectory->Get("waveform_cor");
	 prf->Draw();
	 break;
      case 5:
	 hitmap();
	 break;
      }
      gPad->Update();
   }

   if (to_print() ) {
      c1->Print(pdfile(__func__).c_str());
   }

}

// a row of pixels waveforms
//______________________________________________________________________
void waveform_pixs(int iwave, int irow=0, int ivar=0)
{
   gFile->cd("wave_pixs");
   const char* lname;
   double ymin, ymax;
   if (ivar == 0) {	// adc
      lname = "wave_adc";
      ymin = -15;
      ymax =  41;
   }
   else {		// cds
      lname = "wave_cds";
      ymin = -21;
      ymax =  9;
   }
   TMultiGraph* mg_pix;
   TMultiGraph* mg_cols = new TMultiGraph;
   TGraph* gf = NULL;
   ostringstream oss;
   for (int jcol=0; jcol < NCOLS; jcol++) {
      oss.str("");
      oss << lname << "_pix_r" << irow << "_c" << jcol;
      mg_pix = (TMultiGraph*)gDirectory->Get(oss.str().c_str() );
      if (! mg_pix) {
	 LOG << "TMultiGraph " << oss.str() << " NOT exist!!!" << endl;
	 return;
      }
      gf = (TGraph*)(mg_pix->GetListOfGraphs()->At(iwave) );
      mg_cols->Add(gf);
   }

   double xmin = gf->GetXaxis()->GetXmin();
   double xmax = gf->GetXaxis()->GetXmax();
   LOG << "#" << iwave
       << " " << gf->GetTitle()
       << " " << gf->GetXaxis()->GetXmin()
       << " " << gf->GetXaxis()->GetXmax()
       << " " << gf->GetXaxis()->GetTitle()
       << " " << gf->GetYaxis()->GetTitle()
       << endl;
   mg_cols->Draw("A PLC PMC");
   mg_cols->GetXaxis()->SetRangeUser(xmin, xmax);
   mg_cols->GetXaxis()->SetTitle(gf->GetXaxis()->GetTitle());
   mg_cols->GetYaxis()->SetTitle(gf->GetYaxis()->GetTitle());
   mg_cols->SetMinimum(ymin);
   mg_cols->SetMaximum(ymax);
   TLegend* lg = gPad->BuildLegend(0.69, 0.78, 0.99, 0.99);
   lg->SetNColumns(4);
   gPad->Modified();
   gFile->cd();
}


// ADC & CDS waveforms of fired pixels
//______________________________________________________________________
void waveform(int iwave, int irow)
{
   const char* plot[] = { "wave_adc", "wave_cds"	// NOT use
			  , "wave_cds_trig", "wave_chi2_cds" };
   int nplots = sizeof(plot) / sizeof(*plot);
   double ymin[] = { g_adc_cor_min*0.9, -21, -110, g_chi2_min };
   double ymax[] = { g_adc_cor_max,   	  7,   11, g_chi2_max };

   TCanvas* c1 = get_canvas("c1");
   c1->Clear();
   divide(1, nplots, 0, -0.01);
   const char* opt = "AL";
   int color = 4;
   TMultiGraph* mg;
   for (int ip=0; ip < nplots; ip++) {
      c1->cd(ip+1);
      if (ip < 2) {	// wave_adc, wave_cds
	 waveform_pixs(iwave, irow, ip);
      }
      else {
	 mg = (TMultiGraph*)gDirectory->Get(plot[ip]);
	 if (! mg) {
	    LOG << "TMultiGraph " << plot[ip] << " NOT existing!!!" << endl;
	    return;
	 }
	 TGraph* gf = (TGraph*)(mg->GetListOfGraphs()->At(iwave) );
	 gf->Draw(opt);
	 gf->SetMinimum(ymin[ip]);
	 gf->SetMaximum(ymax[ip]);
	 gf->SetLineColor(color);
	 gf->SetMarkerColor(color);
	 set_pave("title", 0.19);
	 LOG << "#" << iwave
	     << " " << gf->GetTitle()
	     << " " << gf->GetXaxis()->GetXmin()
	     << " " << gf->GetXaxis()->GetXmax()
	     << " " << gf->GetXaxis()->GetTitle()
	     << " " << gf->GetYaxis()->GetTitle()
	     << endl;
      }
      if (ip == 3)	draw_line(g_chi2_cds_x, false);

      //gPad->SetGrid();
      gPad->Update();
   }

   c1->Update();
   gDirectory->DeleteAll();	// clean memory???
}

// ADC & CDS waveforms of fired pixels
//______________________________________________________________________
void waveform(int iwmin, int iwmax, int irow, const char* tag="fig/test")
{
   TMultiGraph* mg = (TMultiGraph*)gDirectory->Get("wave_chi2_cds");
   if (iwmax <= iwmin) iwmax = iwmin + 1;
   if (iwmax > mg->GetListOfGraphs()->GetEntries() )
      iwmax = mg->GetListOfGraphs()->GetEntries();

   set_print(tag);
   pdf_open;
   for (int iwave=iwmin; iwave < iwmax; iwave++) {
      waveform(iwave, irow);
      pdf_print;
      usleep(1000);
   }
   pdf_close;
   set_print();
}


void draw_cols()
{
   int nrows = NROWS, ncols = NCOLS;
   double ymin = gPad->GetUymin();
   double ymax = gPad->GetUymax();
   double xcol = nrows;
   TLine* line;
   TText* txt = new TText;
   txt->SetTextAlign(21);
   ostringstream oss;
   for (int i = 1; i < ncols; i++) {
      line = new TLine(xcol, ymin, xcol, ymax);
      line->Draw();
      line->SetLineStyle(2);
      line->SetLineColor(2);
      oss.str("");
      oss << "C" << i-1;
      txt->DrawText(xcol-0.5*nrows, ymax, oss.str().c_str() );
      xcol += nrows;
      //cout << i << " " << xcol << " " << ymin << " " << ymax << endl;
   }
   oss.str("");
   oss << "C" << ncols-1;
   txt->DrawText(xcol-0.5*nrows, ymax, oss.str().c_str() );

}


// visualization of CDS & ADC noise parameters in RunInfo
//______________________________________________________________________
void baseline()
{
   RunInfo* runinfo = (RunInfo*)gDirectory->Get("runinfo");
   if (! runinfo && g_anly)
      runinfo = g_anly->get_RunInfo();
   if (!runinfo) {
      LOG << "RunInfo NOT existing!!!" << endl;
      return;
   }
   
   ostringstream oss;
   TCanvas* c1 = get_canvas();
   c1->Clear();
   c1->Divide(1, 2);
   pdf_open;
   
   // a graph for a column of pixels
   int np = runinfo->nrows;
   double xp[np], yp[np], ey[np];

   // CDS
   c1->cd(1);
   TMultiGraph* mg_cds = new TMultiGraph();
   mg_cds->SetName("baseline_cds");
   mg_cds->SetTitle("CDS baseline;row;<CDS>");
   for (int col=0; col < runinfo->ncols; col++) {
      for (int i=0; i < np; i++) {
	 xp[i] = i * (1+0.03);		// shift for visualization
	 yp[i] = runinfo->cds_mean[i][col];
	 ey[i] = runinfo->cds_sigma[i][col];
      }
      TGraphErrors* gf = new TGraphErrors(np, xp, yp, 0, ey);
      mg_cds->Add(gf);
      oss.str("");
      oss << "CDS col-" << col << ";row;CDS baseline";
      gf->SetTitle(oss.str().c_str() );
      //gf->GetXaxis()->SetLimits(-1, np);
      //gf->Draw("alp");
   }
   
   mg_cds->Draw("A pmc plc");
   gPad->BuildLegend(0.905, 0.15, 0.995, 0.995);
   cout << mg_cds->ClassName() << "( " << mg_cds->GetName() << " ) created." << endl;
   
   // ADC
   c1->cd(2);
   TMultiGraph* mg_adc = new TMultiGraph();
   mg_adc->SetName("baseline_adc");
   mg_adc->SetTitle("ADC baseline;row;<ADC>");
   for (int col=0; col < runinfo->ncols; col++) {
      for (int i=0; i < np; i++) {
	 xp[i] = i * (1+0.03);		// shift for visualization
	 yp[i] = runinfo->adc_mean[i][col];
	 ey[i] = runinfo->adc_sigma[i][col];
      }
      TGraphErrors* gf = new TGraphErrors(np, xp, yp, 0, ey);
      mg_adc->Add(gf);
      oss.str("");
      oss << "ADC col-" << col << ";row;ADC baseline";
      gf->SetTitle(oss.str().c_str() );
      //gf->GetXaxis()->SetLimits(-1, np);
      //gf->Draw("alp");
   }
   
   mg_adc->Draw("A pmc plc");
   gPad->BuildLegend(0.905, 0.15, 0.995, 0.995);
   cout << mg_adc->ClassName() << "( " << mg_adc->GetName() << " ) created." << endl;

   c1->Update();
   pdf_print;

   // print graphs in a file
   if (g_print.size()) {
      // individual graphs
      for (int i=0; i < mg_cds->GetListOfGraphs()->GetEntries(); i++) {
	 c1->cd(1);
	 mg_cds->GetListOfGraphs()->At(i)->Draw("ALP");
	 c1->cd(2);
	 mg_adc->GetListOfGraphs()->At(i)->Draw("ALP");
	 c1->Update();
	 // c1->Print(pdf.c_str() );
	 pdf_print;
      }
      // pdfs = pdf + "]";
      // c1->Print(pdfs.c_str() );
   }
   
   pdf_close;

   cout << "RunInfo:"
	<< " nrows=" << runinfo->nrows
	<< " ncols=" << runinfo->ncols
	<< endl;
}


// ADC
//______________________________________________________________________
void adc_plots()
{
   const char* plot[] = { "adc_raw", "adc_cor", "adc_frame_raw", "adc_frame_cor" };
   zones_2_1x2(plot);

   TH1* obj=0;
  /*
   if ((obj = (TH1*)find_pad("adc_raw") ) ) {
      set_stats(obj, "nerm", 0.45);
      gPad->SetLogy();
      gPad->Update();
   }

   if ((obj = (TH1*)find_pad("adc_cor") ) ) {
      set_stats(obj, "nemruo", 0.13, -0.11);
      gPad->SetLogy();
      gPad->Update();
   }
*/
   if ((obj = (TH1*)find_pad("adc_frame_raw") ) ) {
      draw_cols();
      set_stats(obj, "nemr", 0.905, 0, 0.995);
      obj->SetNdivisions(-416);		// after set_stats()
      obj->SetMarkerSize(0.3);		// after set_stats()
      gPad->Update();
   }

   if ((obj = (TH1*)find_pad("adc_frame_cor") ) ) {
      draw_cols();
      set_stats(obj, "nemr", 0.905, 0, 0.995);
      obj->SetNdivisions(-416);
      obj->SetMarkerSize(0.5);		// after set_stats()
      gPad->Update();
   }
}


// CDS
//______________________________________________________________________
void cds_plots()
{
   const char* plot[] = { "cds_raw", "cds_cor", "cds_frame_raw", "cds_frame_cor" };
   // gStyle->SetOptStat("nermuo");
   zones_2_1x2(plot);

   TH1* obj=0;
   if ((obj = (TH1*)find_pad("cds_raw") ) ) {
       set_stats(obj, "nermuo");
      gPad->SetLogy();
      gPad->Update();
   }

   if ((obj = (TH1*)find_pad("cds_cor") ) ) {
      set_stats(obj, "nermuo");
      gPad->SetLogy();
      gPad->Update();
   }

   if ((obj = (TH1*)find_pad("cds_frame_raw") ) ) {
      draw_cols();
      set_stats(obj, "nemr", 0.905, 0, 0.995);
      obj->SetNdivisions(-416);		// after set_stats()
      obj->SetMarkerSize(0.1);		// after set_stats()
   }

   if ((obj = (TH1*)find_pad("cds_frame_cor") ) ) {
      draw_cols();
      set_stats(obj, "nemr", 0.905, 0, 0.995);
      obj->SetNdivisions(-416);		// after set_stats()
      obj->SetMarkerSize(0.1);		// after set_stats()
   }
   
   //cout << gPad->IsBatch() << endl;
}


// physics plots
//______________________________________________________________________
void physics()
{
   const char* plot[] = { "trig_cds", "trig_cds_sum", "npixs", "trig_cds_npixs" };
   int nplots = sizeof(plot) / sizeof(*plot);
   zones_auto(plot, nplots);

   TH1* obj=0;
   if ((obj = (TH1*)find_pad("trig_cds") ) ) {
      set_stats(obj, "", -0.25);
      // gPad->Update();
   }
   if ((obj = (TH1*)find_pad("trig_cds_sum") ) ) {
      set_stats(obj, "", 0.135, -0.115);
      // gPad->Update();
   }
   if ((obj = (TH1*)find_pad("trig_cds_npixs") ) ) {
      set_stats(obj, "", -0.105);
      obj->Draw("COLZ");
      gPad->Update();
   }
   
}



// event display of an event
//______________________________________________________________________
void event_display(int ievt)
{
   if (! g_anly) {
      LOG << "analyser(...) first!!!" << endl;
      return;
   }

   event_t evt = g_anly->get_event(ievt);
   TH2D* h2d = evt.display_cds;
   h2d->GetXaxis()->SetNdivisions(-808);
   h2d->GetYaxis()->SetNdivisions(-404);
   h2d->GetXaxis()->SetTitleOffset(1.5);
   h2d->GetYaxis()->SetTitleOffset(1.5);
   h2d->SetLineColor(1);
   h2d->Draw("LEGO2Z");
//   evt.info->Draw();
//   evt.pixs->Draw();

   gPad->Update();
   TPaletteAxis *palette = (TPaletteAxis*)h2d->GetListOfFunctions()->FindObject("palette");
   double xw = 0.025; // palette->GetX2NDC() - palette->GetX1NDC();
   double x1 = 0.93;
   palette->SetX1NDC(x1);	// change palette box width
   palette->SetX2NDC(x1+xw);	// change palette box width
   // h2d->GetZaxis()->SetLabelSize(0.01);	// D=0.04
   palette->GetAxis()->SetTickSize(0.02);	// D=0.03
   gPad->Modified();
}

// for special events
void special_display(int ievt, int fired)
{
   if (! g_anly) {
      LOG << "analyser(...) first!!!" << endl;
      return;
   }

   event_t evt = g_anly->get_special(ievt, fired);
   TH2D* h2d = evt.display_cds;
   h2d->GetXaxis()->SetNdivisions(-808);
   h2d->GetYaxis()->SetNdivisions(-404);
   h2d->GetXaxis()->SetTitleOffset(1.5);
   h2d->GetYaxis()->SetTitleOffset(1.5);
   h2d->SetLineColor(1);
   h2d->Draw("LEGO2Z");
   evt.info->Draw();
   evt.pixs->Draw();

   gPad->Update();
   TPaletteAxis *palette = (TPaletteAxis*)h2d->GetListOfFunctions()->FindObject("palette");
   double xw = 0.025; // palette->GetX2NDC() - palette->GetX1NDC();
   double x1 = 0.93;
   palette->SetX1NDC(x1);	// change palette box width
   palette->SetX2NDC(x1+xw);	// change palette box width
   // h2d->GetZaxis()->SetLabelSize(0.01);	// D=0.04
   palette->GetAxis()->SetTickSize(0.02);	// D=0.03
   gPad->Modified();
}

void frame_display(Long64_t iframe){
	if(! g_anly){
		LOG<<"analyser(...) first!!!"<<endl;
		return;
	}

	frame_t fr = g_anly ->get_frame(iframe);
	TH2D* h2d = fr.display_frame;
   	h2d->GetXaxis()->SetNdivisions(-808);
   	h2d->GetYaxis()->SetNdivisions(-404);
   	h2d->GetXaxis()->SetTitleOffset(1.5);
   	h2d->GetYaxis()->SetTitleOffset(1.5);
   	h2d->SetLineColor(1);
   	h2d->Draw("LEGO2Z");

}



// save multiple events display
void event_display(int ievt, int nevts, const char* tag="fig/test")
{
   set_print(tag);
   pdf_open;
   for (int i=0; i < nevts; i++) {
      event_display(ievt++);
      pdf_print;
	  sleep(1);
   }
   pdf_close;
   set_print();
}
// save multiple  special events
void special_display(int ievt, int nevts, int fired, const char* tag="fig/test")
{
   set_print(tag);
   pdf_open;
   for (int i=0; i < nevts; i++) {
      special_display(ievt++, fired);
      pdf_print;
   }
   pdf_close;
   set_print();
}

void spectrum_rec(int i){
	TH1D* h1d;
	TList* list;
	list = (TList*)gDirectory ->Get("cluster_abnormal_exclude");
	h1d = (TH1D*) list -> At(i);
	h1d -> GetXaxis() -> SetRangeUser(30,250);
	h1d -> Draw();
	gPad -> Update();
}

void spectrum_rec(const char* tag = "fig/spectra"){
	set_print(tag);
	pdf_open;
	TList* list;
	list = (TList*)gDirectory ->Get("cluster_abnormal_exclude");
	int tot = list ->GetEntries();
	for(int i =0; i < tot; i++){
		spectrum_rec(i++);
		pdf_print;
	}
	pdf_close;
	set_print();
}

// chi2 of CDS*CDS of all pixels per frame
//______________________________________________________________________
void chi2(bool logy=true)
{
   gStyle->SetOptStat(111111);
   TH1D* h1d = (TH1D*)gDirectory->Get("chi2_cds");
   h1d->Draw();
   gPad->SetLogy(logy);
   gPad->Update();
   TPaveStats* stats = (TPaveStats*)h1d->FindObject("stats");
   double y1 = stats->GetY1NDC();
   double y2 = stats->GetY2NDC();

   h1d = (TH1D*)gDirectory->Get("chi2_trig");
   h1d->Draw("SAMES");
   h1d->SetLineColor(2);
   gPad->Update();
   stats = (TPaveStats*)h1d->FindObject("stats");
   stats->SetY1NDC(2*y1 - y2);
   stats->SetY2NDC(y1);
   
   // notes
   int ndf = 1024;
   double sigma = sqrt(2*ndf);
   draw_line(ndf, true);
   // draw_line(ndf+sigma);
   draw_line(ndf+sigma*8, true);
   
   gPad->Modified();
}

// row pixels ADC & CDS
//______________________________________________________________________
void pixels_row(int irow, const char* var, bool legend=false)
{
   string lname = "pix_";
   lname += var;
   TList* list = (TList*)gDirectory->Get(lname.c_str() );
   if (! list) {
      LOG << "TList " << lname << " NOT exist!!!" << endl;
      return;
   }
   cout << list << " " << lname << " " << list->GetEntries() << endl;
   
   THStack* hs = new THStack;
   TH1D* h1d = NULL;
   ostringstream oss;
   for (int jcol = 0; jcol < NCOLS; jcol++) {
      // oss.str("");
      // oss << var << "_" << irow << "_" << jcol;
      // h1d = (TH1D*)gDirectory->Get(oss.str().c_str() );
      h1d = (TH1D*)list->At(jcol+irow*NCOLS);
      // h1d->SetLineStyle( (jcol%9) +1);
      h1d->SetMarkerStyle(20+(jcol%10) );
      h1d->SetMarkerSize(0.1);
      hs->Add(h1d);
      //cout << h1d->GetName() << endl;
   }
   //cout << hs->GetNhists() << endl;
   hs->Draw("noSTACK PLC PMC PH");
   hs->GetXaxis()->SetTitle(h1d->GetXaxis()->GetTitle() );
   if (legend) {
      TLegend* lg = gPad->BuildLegend(0.03, 0.72, 0.39, 0.97);
      lg->SetNColumns(4);
   }
   oss.str("");
   oss << "row " << irow;
   TLatex* ltx = new TLatex(0.99, 0.95, oss.str().c_str() );
   ltx->SetNDC();
   ltx->SetTextAlign(33);
   ltx->Draw();
   gPad->Modified();
}

// 16 rows of pixels ADC & CDS
//______________________________________________________________________
void pixels(int rowstart, const char* var, bool logy=false)
{
   TCanvas* c1 = wm::get_canvas();
   c1->Clear();
   int nx=4, ny=4;
   int nrows = nx * ny;
   divide(nx, ny, -0.01, -0.01);

   TLegend* lg;
   int ip = 0;
   for (int irow=rowstart; irow < rowstart+nrows; irow++) {
      c1->cd(++ip);
      gPad->SetLogy(logy);
      pixels_row(irow, var, ip == nrows);
      //gPad->Update();
   }
   c1->cd();
   c1->Update();
   
}

void pixels_adc(bool logy=true, const char* tag="fig/test")
{
   TCanvas* c1 = get_canvas();	// MUST!!!
   set_print(tag);
   pdf_open;
   for (int i=0; i < 4; i++) {
      pixels(i*16, "adc", logy);
      pdf_print;
   }
   pdf_close;
   set_print();
}

void pixels_cds(bool logy=true, const char* tag="fig/test")
{
   TCanvas* c1 = get_canvas();	// MUST!!!
   set_print(tag);
   pdf_open;
   for (int i=0; i < 4; i++) {
      pixels(i*16, "cds", logy);
      pdf_print;
   }
   pdf_close;
   set_print();
}

void dTevt_fit()
{
   TH1D* h1d = (TH1D*)gDirectory->Get("dTevt");
   //h1d->Draw();
   TF1* f1 = new TF1("Exp", "[0]*TMath::Exp(-x/[1])", 2, 1000);
   f1->SetParameters(1, 100);
   f1->SetParNames("N_{0}", "#tau");
   f1->SetLineColor(2);
   h1d->Fit(f1, "R");
   gStyle->SetOptTitle(0);
   gPad->SetLogy();
   gPad->Update();
}


void misc()
{
   gStyle->SetOptTitle(1);
   const char* plot[] = { "trig", "fid", "none" };
   int nplots = sizeof(plot) / sizeof(*plot);
   zones_auto(plot, nplots);

   //cout << gPad->GetName() << endl;

   TH1* obj=0;
   if ((obj = (TH1*)find_pad("trig") ) ) {
      gPad->SetLogy();
      set_stats(obj, "neuo");
      // gPad->Update();
   }

   if ((obj = (TH1*)find_pad("fid") ) ) {
      gPad->SetLogy(0);
      obj->SetMinimum(0);
      set_stats(obj, "ne", 0, -0.3);
   }

   gPad->GetCanvas()->cd(gPad->GetNumber()+1);
   chi2(true);

   gPad->GetCanvas()->cd(gPad->GetNumber()+1);
   dTevt_fit();
   
}


//======================================================================

// ??? c1 first???
void control_plots(const char* msg="fig/test")
{
   TCanvas* c1 = get_canvas();	// MUST!!!
   set_print(msg);
   pdf_open;

   physics();
   pdf_print;

   c1->Clear();
   hitmap();
   pdf_print;
   
   misc();
   pdf_print;

   adc_plots();
   pdf_print;

   cds_plots();
   pdf_print;
   
   pdf_close;
   set_print();
   c1->cd();
}

void adc_frame(){
   
   	TCanvas* c1 = new TCanvas("c1", "c1", 1200, 400);	
   	TPad* p1 = new TPad("p1", "p1", 0.0, 0.0, 1., 0.5, 0); p1 ->Draw();
   	TPad* p2 = new TPad("p2", "p2", 0.0, 0.5, 1., 1., 0); p2 ->Draw();

	
   	TProfile* prf;
   	prf = (TProfile*)gDirectory ->Get("adc_frame_cor");
  	
 	p1 ->SetLeftMargin(0.19);
 	p1 ->SetBottomMargin(0.3);
 	p1 ->SetTopMargin(0);

 	p2 ->SetTopMargin(0.15);
 	p2 ->SetLeftMargin(0.19);
 	p2 ->SetBottomMargin(0);
	p1 ->cd();
	prf ->SetTitle(";64#timesCOL + ROW;ADC corrected");	
	prf ->Draw();
	prf ->GetXaxis()->SetLabelSize(0.1);
	prf ->GetXaxis()->SetTitleSize(0.1);
//	prf ->GetXaxis()->SetTitleOffset(0.5);
	prf ->GetYaxis()->SetLabelSize(0.1);
	prf ->GetYaxis()->SetTitleSize(0.1);
	prf ->GetYaxis()->SetTitleOffset(0.5);
  //  set_stats(obj, "nemr", 0.905, 0, 0.995);

   

	gStyle ->SetOptStat(0000);
	gStyle ->SetOptFit(000000);
    prf->SetNdivisions(-416);		// after set_stats()
    prf->SetMarkerSize(0.3);		// after set_stats()
  //  gPad->Update();

	p2 ->cd();
   	prf = (TProfile*)gDirectory ->Get("adc_frame_raw");
	prf ->SetTitle(";64#timesCOL + ROW;ADC raw");
	prf ->Draw();
	prf ->GetXaxis()->SetLabelSize(0.1);
	prf ->GetXaxis()->SetTitleSize(0.1);
//	prf ->GetXaxis()->SetTitleOffset(0.5);
	prf ->GetYaxis()->SetLabelSize(0.1);
	prf ->GetYaxis()->SetTitleSize(0.1);
	prf ->GetYaxis()->SetTitleOffset(0.5);

//  set_stats(obj, "nemr", 0.905, 0, 0.995);
    prf->SetNdivisions(-416);		// after set_stats()
    prf->SetMarkerSize(0.3);		// after set_stats()
    gPad->Update();


	gStyle ->SetOptStat(0000);
	gStyle ->SetOptFit(000000);

	
	int nrows = NROWS, ncols = NCOLS;
   	double ymin = p1 ->GetUymin();
   	double ymax = p2 ->GetUymax();
   	double xcol = nrows;
   	TLine* line;
   	TText* txt = new TText;
   	txt->SetTextAlign(21);
   	ostringstream oss;
   	for (int i = 1; i < ncols; i++) {
      line = new TLine(xcol, ymin, xcol, ymax);
      line->Draw();
      line->SetLineStyle(2);
      line->SetLineColor(2);
      oss.str("");
      oss << "C" << i-1;
	  txt->SetTextSize(0.1);
      txt->DrawText(xcol-0.5*nrows, ymax, oss.str().c_str() );
      xcol += nrows;
      //cout << i << " " << xcol << " " << ymin << " " << ymax << endl;
   	}
   	oss.str("");
   	oss << "C" << ncols-1;
	txt->SetTextSize(0.1);
   	txt->DrawText(xcol-0.5*nrows, ymax, oss.str().c_str() );

	p1 ->cd();
	int nrows2 = NROWS, ncols2 = NCOLS;
   	double ymin2 = p1 ->GetUymin();
   	double ymax2 = p1 ->GetUymax();
   	double xcol2 = nrows2;
   	TLine* line2;
   	for (int i = 1; i < ncols; i++) {
      line2 = new TLine(xcol2, ymin2, xcol2, ymax2);
      line2->Draw();
      line2->SetLineStyle(2);
      line2->SetLineColor(2);
      xcol2 += nrows2;
      //cout << i << " " << xcol << " " << ymin << " " << ymax << endl;
   	}
}

void cds_frame(){
   
   TCanvas* c1 = new TCanvas("c1", "c1", 1200, 400);	
   TPad* p1 = new TPad("p1", "p1", 0.0, 0.0, 1., 0.5, 0); p1 ->Draw();
   TPad* p2 = new TPad("p2", "p2", 0.0, 0.5, 1., 1., 0); p2 ->Draw();

	
   TProfile* prf;
   prf = (TProfile*)gDirectory ->Get("cds_frame_raw");
  	
 	p1 ->SetLeftMargin(0.19);
 	p1 ->SetBottomMargin(0.3);
 	p1 ->SetTopMargin(0);

 	p2 ->SetTopMargin(0.15);
 	p2 ->SetLeftMargin(0.19);
 	p2 ->SetBottomMargin(0);
	p1 ->cd();
	prf ->SetTitle(";64#timesCOL + ROW;CDS raw");	
	prf ->Draw();
	prf ->GetXaxis()->SetLabelSize(0.1);
	prf ->GetXaxis()->SetTitleSize(0.1);
//	prf ->GetXaxis()->SetTitleOffset(0.5);
	prf ->GetYaxis()->SetLabelSize(0.1);
	prf ->GetYaxis()->SetTitleSize(0.1);
	prf ->GetYaxis()->SetTitleOffset(0.5);
  //  set_stats(obj, "nemr", 0.905, 0, 0.995);

   

	gStyle ->SetOptStat(0000);
	gStyle ->SetOptFit(000000);
    prf->SetNdivisions(-416);		// after set_stats()
    prf->SetMarkerSize(0.3);		// after set_stats()
  //  gPad->Update();

	p2 ->cd();
   prf = (TProfile*)gDirectory ->Get("adc_frame_raw");
	prf ->SetTitle(";64#timesCOL + ROW;ADC raw");
	prf ->Draw();
	prf ->GetXaxis()->SetLabelSize(0.1);
	prf ->GetXaxis()->SetTitleSize(0.1);
//	prf ->GetXaxis()->SetTitleOffset(0.5);
	prf ->GetYaxis()->SetLabelSize(0.1);
	prf ->GetYaxis()->SetTitleSize(0.1);
	prf ->GetYaxis()->SetTitleOffset(0.5);

//  set_stats(obj, "nemr", 0.905, 0, 0.995);
    prf->SetNdivisions(-416);		// after set_stats()
    prf->SetMarkerSize(0.3);		// after set_stats()
    gPad->Update();


	gStyle ->SetOptStat(0000);
	gStyle ->SetOptFit(000000);

	
	int nrows = NROWS, ncols = NCOLS;
   	double ymin = p1 ->GetUymin();
   	double ymax = p2 ->GetUymax();
   	double xcol = nrows*9;
   	TLine* line;
   	TText* txt = new TText;
   	txt->SetTextAlign(21);
   	ostringstream oss;
   	for (int i = 9; i < ncols; i++) {
      line = new TLine(xcol, ymin, xcol, ymax);
      line->Draw();
      line->SetLineStyle(2);
      line->SetLineColor(2);
      oss.str("");
      oss << "C" << i-1;
	   txt->SetTextSize(0.1);
      txt->DrawText(xcol-0.5*nrows, ymax, oss.str().c_str() );
      xcol += nrows;
      //cout << i << " " << xcol << " " << ymin << " " << ymax << endl;
   	}
   	oss.str("");
   	oss << "C" << ncols-1;
		txt->SetTextSize(0.1);
   	txt->DrawText(xcol-0.5*nrows, ymax, oss.str().c_str() );

		p1 ->cd();
		int nrows2 = NROWS, ncols2 = NCOLS;
   	double ymin2 = -0.012;//p1 ->GetUymin();
   	double ymax2 = 0.012;//p1 ->GetUymax();
   	double xcol2 = nrows2*9;
   	TLine* line2;
   	for (int i = 9; i < ncols; i++) {
      line2 = new TLine(xcol2, ymin2, xcol2, ymax2);
      line2->Draw();
      line2->SetLineStyle(2);
      line2->SetLineColor(2);
      xcol2 += nrows2;
      //cout << i << " " << xcol << " " << ymin << " " << ymax << endl;
   	}
}
void pix_baseline(){
   
   TCanvas* c1 = new TCanvas("c1", "c1", 1600, 1600);	
	c1->Divide(4,4);
	ostringstream oss;
	string hname, htitle;
   TH1D* gbase[16];
	double hrange;
	for(int i=0; i<16; i++){
		oss.str("");
		oss<<"gbase"<<i;
		hname = oss.str();
   		gbase[i] = (TH1D*)gDirectory ->Get(hname.c_str());
		gbase[i]->GetXaxis()->SetTitle("ROW");
		gbase[i]->GetYaxis()->SetTitle("ADC");
		hrange = gbase[i]->GetBinContent(1);
		gbase[i]->GetYaxis()->SetRangeUser(hrange-3000, hrange+3000);

		c1->cd(i+1);
		gbase[i]->Draw();
	}
	
	
}  	
