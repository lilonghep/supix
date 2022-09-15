/*******************************************************************//**
 * $Id: common_root.h 1275 2021-04-05 16:16:48Z mwang $
 *
 * common header files of ROOT
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-07-05 10:41:30
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#ifndef common_root_h
#define common_root_h

#include "common.h"

//#define DEBUG

#ifndef LOG
#define LOG std::cout <<__PRETTY_FUNCTION__<<" --- "
#endif

#include <TCanvas.h>
#include <TDirectory.h>
#include <TF1.h>
#include <TFile.h>
#include <TFitResult.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TH1D.h>
#include <TH2D.h>
#include <THStack.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TMarker.h>
#include <TMath.h>
#include <TMultiGraph.h>
#include <TObjArray.h>
#include <TPaletteAxis.h>
#include <TPaveStats.h>
#include <TPolyMarker.h>
#include <TProfile.h>
#include <TROOT.h>
#include <TRandom.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>

#include <cmath>
#include <sstream>

namespace wm
{
   template<typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

   // my styles
   const int colors[] { 1, 2, 4, 6, 7, 8, 9, 11, 21, 31, 41, 46, 49 };
   const int ncolors = sizeof(colors)/sizeof(*colors);

   // math
   //===================================================================
   
   // round to power 10
   inline
   double round10(double x)
   {
      double x10 = x;
      if (x < 0) x10 *= -1;	// positive
      if (x10 > 1) {
	 x10 = pow(10, (int)log10(x10) + 1);
      }
      else if (x10 < 1) {
	 x10 = pow(10, (int)log10(x10) - 1);
      }
      x10 *= sgn(x);
      return x10;
   }
   
   // prototypes
   //===================================================================
   void watermark();
   TCanvas* get_canvas(const char* name="c1", int ww=1400, int wh=787 /* 16:9 */);
   void divide(Int_t nx=1, Int_t ny=1, Float_t xmargin=0.01, Float_t ymargin=0.01, Int_t color=0);
   void set_pave(const char* name, double x1, double y1=0, double x2=0, double y2=0);
   void set_pave(TPave* obj, double x1, double y1=0, double x2=0, double y2=0);
   void set_stats(TH1* obj, const char* mode, double x1=0, double y1=0, double x2=0, double y2=0);
   void draw_line(double x0, bool vertical = true, bool dotxt=true, int stl=9, int clr=49);
   void draw_pad(TF1 *f1, double xmin=0, double xmax=0, bool doYaxis=false);
   void draw_pad(TF1 **f1s, int nf1s, double xmin=0, double xmax=0);

   //______________________________________________________________________
   inline
   TCanvas* get_canvas(const char* name, int ww, int wh)
   {
      TCanvas* c1 = (TCanvas*)gROOT->GetListOfCanvases()->FindObject(name);
      if(c1 == 0) {
	 c1 = new TCanvas(name, name, ww, wh);
	 c1->SetWindowSize(ww + (ww - c1->GetWw()), wh + (wh - c1->GetWh()));	// interactive
	 // c1->SetCanvasSize(w,h);	// batch
      }
      else {
      	 // c1->Clear();
      	 c1->cd();
      }
      return c1;
   }

   // replce TPad::Divide(), its no-margin case is problematic.
   //   xmargin or ymargin < 0 : no margins
   //___________________________________________________________________
   inline
   void divide(Int_t nx, Int_t ny, Float_t xmargin, Float_t ymargin, Int_t color)
   {
      if (! gPad->IsEditable()) return;
 
      TPad *padsav = (TPad*)gPad;
      padsav->GetCanvas()->cd();
      if (nx <= 0) nx = 1;
      if (ny <= 0) ny = 1;
      Double_t x1,y1,x2,y2;
      Double_t dx,dy;
      TPad *pad;
      Int_t nchname  = strlen(padsav->GetName())+6;
      Int_t nchtitle = strlen(padsav->GetTitle())+6;
      char *name  = new char [nchname];
      char *title = new char [nchtitle];
      Int_t n = 0;
      if (color == 0) color = padsav->GetFillColor();

      // determine layout parameters
      Double_t xl = padsav->GetLeftMargin();
      Double_t xr = padsav->GetRightMargin();
      Double_t yb = padsav->GetBottomMargin();
      Double_t yt = padsav->GetTopMargin();
      // cout << "L=" << xl << " R=" << xr << " T=" << yt << " B=" << yb << endl;
      Bool_t xnomargin=false, ynomargin=false;
      if (xmargin >= 0) {		// normal
	 dx = 1/Double_t(nx);
      }
      else {			// NO margin
	 xnomargin = true;
	 xmargin *= -1;
	 xl /= nx;
	 xr /= nx;
	 dx = (1 - 2*xmargin - xl - xr) / Double_t(nx);
      }
   
      if (ymargin >= 0) {
	 dy = 1/Double_t(ny);
      }
      else {
	 ynomargin = true;
	 ymargin *= -1;
	 yt /= ny;
	 yb /= ny;
	 dy = (1 - 2*ymargin - (yt + yb) ) / Double_t(ny);
      }
      // cout << "dx=" << dx << " dy=" << dy << " L=" << xl << " R=" << xr << " T=" << yt << " B=" << yb << endl;

      // create pads
      //
      y2 = 1 - ymargin;
      for (Int_t iy=0; iy<ny; iy++) {
	 // y2 = 1 - iy*dy - ymargin;
	 y1 = y2 - dy;
	 if (ynomargin) {
	    if (iy == 0)		y1 -= yt;
	    else if (iy == ny-1)	y1 -= yb;
	 }
	 else	 			y1 += 2*ymargin;
      
	 x1 = xmargin;
	 for (Int_t ix=0; ix<nx; ix++) {
	    x2 = x1 + dx;
	    if (xnomargin) {
	       if (ix == 0)		x2 += xl;
	       else if (ix==nx-1)	x2 += xr;
	    }
	    else	    		x2 -= 2*xmargin;
	 
	    n++;
	    snprintf(name,nchname,"%s_%d",padsav->GetName(),n);
	    snprintf(title,nchtitle,"%s_%d",padsav->GetTitle(),n);
	    pad = new TPad(name,title,x1,y1,x2,y2,color);
	    pad->SetNumber(n);
	    if (xnomargin || ynomargin) {
	       pad->SetBorderSize(0);
	       pad->SetBorderMode(0);
	    }
	 
	    if (ynomargin) {
	       pad->SetTopMargin(0);
	       pad->SetBottomMargin(0);
	       if (iy == 0) {
		  pad->SetTopMargin(yt*ny);	// scale back
	       }
	       if (iy == ny-1) {
		  pad->SetBottomMargin(yb*ny);	// scale back
	       }
	    }

	    if (xnomargin) {
	       pad->SetLeftMargin(0);
	       pad->SetRightMargin(0);
	       if (ix == 0) {
		  pad->SetLeftMargin(xl*nx);
	       }
	       if (ix == nx-1) {
		  pad->SetRightMargin(xr*nx);
	       }
	    }

	    // cout << ix << "x" << iy << ": x " << x1 << "-" << x2 << " y " << y1 << "-" << y2 << endl;
	    pad->Draw();
	    // next
	    x1 += dx;
	    if (xnomargin && ix == 0)	x1 += xl;
	 }
	 y2 -= dy;
	 if (ynomargin && iy == 0)	y2 -= yt;
      }

      delete [] name;
      delete [] title;
      padsav->Modified();
      if (padsav) padsav->cd();
   }

   // create a new style for individual TObject.
   // return: the default style for restoring.
   //___________________________________________________________________
   inline
   TStyle* save_style()
   {
      static TStyle* saved = gStyle;
      TStyle* astyle = new TStyle("astyle", "new style");
      gStyle->Copy(*astyle);
      astyle->cd();	// set to gStyle
   
#ifdef DEBUG
      LOG << " gStyle: " << gStyle->GetName()
	  << " Stat(X Y W H)=(" << gStyle->GetStatX() << " " << gStyle->GetStatY()
	  << " " << gStyle->GetStatW() << " " << gStyle->GetStatH() << ")"
	  << " saved:" << saved->GetName()
	  << " (" << saved->GetStatX() << " " << saved->GetStatY()
	  << " " << saved->GetStatW() << " " << saved->GetStatH() << ")"
	  << std::endl;
#endif
      return saved;
   }


   // resize or move a pave
   //   x1, y1 = 0 : no change
   //          < 0 : distance to right/top side
   //          > 0 : new position
   //   x2, y2 > 0 : new position
   //         <= 0 : determined automatically
   //
   // title = (TPaveText*)(gPad->GetListOfPrimitives()->FindObject("title"));
   // stats = (TPaveStats*)(h1->FindObject("stats"));
   // legend = (TPave*)(gPad->GetListOfPrimitives()->FindObject("TPave"));
   //___________________________________________________________________
   inline
   void set_pave(const char* name, double x1, double y1, double x2, double y2)
   {
      //LOG << gPad->IsModified() << std::endl;
      if (gPad->IsModified() )		gPad->Update();
      TPave* pave = (TPave*)(gPad->GetListOfPrimitives()->FindObject(name));
      if (! pave) {
	 std::cout << "NO " << name << " found!!!" << std::endl;
	 return;
      }
      set_pave(pave, x1, y1, x2, y2);
   }
   
   inline
   void set_pave(TPave* obj, double x1, double y1, double x2, double y2)
   {
#ifdef DEBUG
      LOG << "new: " << obj->ClassName() << "(" << obj->GetName() << ")"
	  << " " << gPad->GetName()
	  << " " << obj->GetX1NDC()
	  << " " << obj->GetY1NDC()
	  << " " << obj->GetX2NDC()
	  << " " << obj->GetY2NDC()
	  << std::endl;
#endif
      //LOG << gPad->IsModified() << std::endl;
      if (gPad->IsModified() )		gPad->Update();
      double x1_old = obj->GetX1NDC();
      double y1_old = obj->GetY1NDC();
      double x2_old = obj->GetX2NDC();
      double y2_old = obj->GetY2NDC();
      double xw = x2_old - x1_old;
      double yw = y2_old - y1_old;

      if (x1 < 0)		x1 = 1 + x1 - xw;
      else if (x1 == 0)	x1 = x1_old;

      if (y1 < 0)		y1 = 1 + y1 - yw;
      else if (y1 == 0)	y1 = y1_old;
   
      if (x2 <= 0)		x2 = x1 + xw;
      if (y2 <= 0)		y2 = y1 + yw;

      // LOG << "y1=" << y1 << " y2=" << y2 << std::endl;
      
      obj->SetX1NDC(x1);
      obj->SetX2NDC(x2);
      obj->SetY1NDC(y1);
      obj->SetY2NDC(y2);
      gPad->Modified();
      gPad->Update();
#ifdef DEBUG
      LOG << "new: " << obj->ClassName() << "(" << obj->GetName() << ")"
	  << " " << gPad->GetName()
	  << " " << obj->GetX1NDC()
	  << " " << obj->GetY1NDC()
	  << " " << obj->GetX2NDC()
	  << " " << obj->GetY2NDC()
	  << std::endl;
#endif

   }
   
   // // [obsolete] change width of stats box
   // //______________________________________________________________________
   // inline
   // void setW_pave(TH1* obj, double xndc)
   // {
   //    TPaveStats* pave = (TPaveStats*)obj->GetListOfFunctions()->FindObject("stats");
   //    if (!pave) {
   // 	 LOG << "NOT yet having a stats box!!!" << std::endl;
   // 	 return;
   //    }
   //    double x1 = pave->GetX1NDC();
   //    double y1 = pave->GetY1NDC();
   //    double x2 = pave->GetX2NDC();
   //    double y2 = pave->GetY2NDC();
   //    double xw = x2 - x1;
   //    double yw = y2 - y1;
   //    yw = yw/xw * xndc;
   //    x1 = x2 - xndc;
   //    y1 = y2 - yw;
   //    pave->SetX1NDC(x1);
   //    pave->SetY1NDC(y1);
   //    gPad->Modified();
   // }


   // set style, size and position of stats box
   //   mode     : as gStyle->SetOpStat(), set height by text lines
   //         ""   not change
   //   (x0, y0) : new (x1, y1) in NDC
   //	    < 0    margin on the opposite side
   //___________________________________________________________________
   // void set_stats(TObject* obj, const char* mode, double x1, double y1, double x2, double y2)
   // { set_stats((TH1*)obj, mode, x1, y1, x2, y2); }

   // // adjust size of stats box automatically
   // void set_stats(TH1* obj, const char* mode, double x0=-9, double y0=-9);
   // void set_stats(TObject* obj, const char* mode, double x0=-9, double y0=-9)
   // { set_stats((TH1*)obj, mode, x0, y0); }

   // copied from  void TStyle::SetOptStat(Option_t *stat)
   inline
   int int_mode(const char* stat)
   {
      Int_t mode=0;
      TString opt = stat;
      if (opt.Contains("n")) mode+=1;
      if (opt.Contains("e")) mode+=10;
      if (opt.Contains("m")) mode+=100;
      if (opt.Contains("M")) mode+=200;
      if (opt.Contains("r")) mode+=1000;
      if (opt.Contains("R")) mode+=2000;
      if (opt.Contains("u")) mode+=10000;
      if (opt.Contains("o")) mode+=100000;
      if (opt.Contains("i")) mode+=1000000;
      if (opt.Contains("I")) mode+=2000000;
      if (opt.Contains("s")) mode+=10000000;
      if (opt.Contains("S")) mode+=20000000;
      if (opt.Contains("k")) mode+=100000000;
      if (opt.Contains("K")) mode+=200000000;
      if (mode == 1) mode = 1000000001;
      return mode;
   }
   
   inline
   void set_stats(TH1* obj, const char* mode, double x1, double y1, double x2, double y2)
   {
      TPaveStats* stats = (TPaveStats*)obj->FindObject("stats");
      if (!stats) {
	 return;
      }

      // first coordinates
      set_pave(stats, x1, y1, x2, y2);
      
      // then mode
      int nlines = stats->GetSize();
      std::string smode = mode;
      int nmode = smode.size();

      // scale height if necesssary
      if (nmode > 0 && nmode != nlines) {
	 if (dynamic_cast<TProfile*>(obj) ) {
	    if (smode.find('m') != std::string::npos) nmode++;
	    if (smode.find('r') != std::string::npos) nmode++;
	    // if (smode.find('u') != std::string::npos ||
	    // 	smode.find('o') != std::string::npos) nmode += 2;
	 }

	 // new coordinates
	 y1 = stats->GetY1NDC();
	 y2 = stats->GetY2NDC();

	 // LOG << "y1=" << y1 << " y2=" << y2 << std::endl;

	 double yw = y2 - y1;
	 yw *= (double)nmode/nlines;	// scale height
	 stats->SetOptStat(int_mode(mode) );
	 stats->SetY1NDC(y2 - yw);
	 //set_pave(stats, 0, y2-yw, 0, y2);
	 // LOG << "mode lines: " << nmode << " <- " << nlines
	 //     << " height: " << yw << " <- " << y2-y1
	 //     << std::endl;
      }
   }
   
   //    inline
   //    void set_stats(TH1* obj, const char* mode, double x0, double y0)
   //    {
   //       TPaveStats* stats = (TPaveStats*)obj->FindObject("stats");
   //       if (!stats) {
   // 	 return;
   //       }
   //       double x1 = stats->GetX1NDC();
   //       double y1 = stats->GetY1NDC();
   //       double x2 = stats->GetX2NDC();
   //       double y2 = stats->GetY2NDC();
   //       double xw = x2 - x1;
   //       double yw = y2 - y1;
   //       int nlines = stats->GetSize();
   //       std::string smode = mode;
   //       int nmode = smode.size();

   // #ifdef DEBUG
   //       LOG << "stats: " << obj
   // 	  << " box=" << x1 << " " << y1 << " " << x2 << " " << y2
   // 	  << " nlines=" << nlines
   // 	  << " nmode=" << nmode
   // 	  << " mode=" << mode
   // 	  << " " << gStyle->GetOptStat()
   // 	  << std::endl;
   // #endif

   //       // scale height if necesssary
   //       if (nmode > 0 && nmode != nlines) {
   // 	 if (dynamic_cast<TProfile*>(obj) ) {
   // 	    if (smode.find('m') != std::string::npos) nmode++;
   // 	    if (smode.find('r') != std::string::npos) nmode++;
   // 	    // if (smode.find('u') != std::string::npos ||
   // 	    // 	smode.find('o') != std::string::npos) nmode += 2;
   // 	 }
   // 	 yw *= (double)nmode/nlines;	// scale height
   // 	 y1 = y2 - yw;			// shift bottom line
   //       }
      
   //       // new position
   //       if (x0 > 0)
   // 	 x1 = x0;
   //       else if (x0 > -9)
   // 	 x1 = 1 + x0 - xw;
   //       x2 = x1 + xw;
      
   //       if (y0 > 0)
   // 	 y1 = y0;
   //       else if (y0 > -9)
   // 	 y1 = 1 + y0 - yw;
   //       y2 = y1 + yw;

   //       set_stats(obj, mode, x1, y1, x2, y2);
      
   // #ifdef DEBUG
   //       LOG << "stats: " << obj
   // 	  << " box=" << x1 << " " << y1 << " " << x2 << " " << y2
   // 	  << " nlines=" << nlines
   // 	  << " nmode=" << nmode
   // 	  << " mode=" << mode
   // 	  << " " << gStyle->GetOptStat()
   // 	  << std::endl;
   // #endif
   //    }
   
   // draw a TObject according to its ClassName
   inline
   void draw_obj(TObject* obj)
   {
      if (obj->InheritsFrom("TH2") ) {
	 ((TH2*)obj)->Draw("COLZ");
      }
      else
	 obj->Draw();
      gPad->Update();
   }

   // find & draw by name
   inline
   void draw_obj(const char* name)
   {
      TObject* obj = gDirectory->Get(name);
      if (! obj) {
	 LOG << name << " NOT exists!!!" << std::endl;
      }
      else {
	 LOG << "draw:"
	     << " " << obj->ClassName()
	     << "\t" << obj->GetName()
	     << " on Pad " << gPad->GetName()
	     << std::endl;
	 draw_obj(obj);
      }
   }

   // zones for plots
   //______________________________________________________________________
   inline
   void zones_auto(const char** plot, int nplots, int ny=0)
   {
      // determine zones
      if (! ny)
	 ny = std::sqrt((double)nplots) + 0.5;	// smaller in y
      int nx = nplots/ny + (nplots%ny == 0 ? 0 : 1);
   
      TCanvas* c1 = get_canvas("c1");
      c1->Clear();
      c1->Divide(nx, ny);

      int ip = 0;		// plot index
      while(ip < nplots) {
	 c1->cd(ip+1);
	 // draw a plot
	 draw_obj(plot[ip]);
	 // next
	 ip++;
      }
      c1->cd();
      c1->Update();
   }

   // zones for plots
   // _____
   // |_|_|
   // |___|
   // |___|
   //______________________________________________________________________
   inline
   void zones_2_1x2(const char** plot, float xmargin=0.01, float ymargin=0.01)
   {
      int ip = 0;

      TCanvas* c1 = get_canvas("c1");
      c1->Clear();
      TPad* pad;
      int nx= 2, ny = 3;
      double y1 = 1, yw = 1./ny;
      for (int iy=1; iy <= ny; iy++) {
	 y1 -= yw;
	 if (iy == 1) nx = 2;
	 else nx = 1;
	 double x1 = 0, xw = 1./nx;
	 for (int ix=1; ix <= nx; ix++) {
	    std::string name = c1->GetName();
	    name += "_";
	    name += plot[ip];
	    pad = new TPad(name.c_str(), name.c_str(), x1+0.5*xmargin, y1+0.5*ymargin,
			   x1+xw-0.5*xmargin, y1+yw-0.5*ymargin);
	    pad->Draw();
	    pad->cd();
#ifdef DEBUG
	    pad->SetFillColor(11+ip);
#endif
	    // draw a plot
	    draw_obj(plot[ip]);
	    // next
	    ip++;
	    x1 += xw;
	    c1->cd();
	 }
      }
      c1->cd();
      c1->Update();
   }


   // find the pad where a plot is drawn
   //______________________________________________________________________
   inline
   TObject* find_pad(const char* name)
   {
      TObject* obj = 0;
      TCanvas* c1 = gPad->GetCanvas();
      for (int i=0; i < c1->GetListOfPrimitives()->GetEntries(); i++) {
	 obj = c1->GetListOfPrimitives()->At(i)->FindObject(name);
	 if (obj) {
	    TPad* pad = (TPad*)(c1->GetListOfPrimitives()->At(i) );
	    pad->cd();
#ifdef DEBUG
	    LOG << "Found: " << name << " " << obj->ClassName()
		<< " on " << pad->GetName() << std::endl;
#endif
	    break;
	 }
      }
      return obj;
   }

   //======================================== print-utility
   // automatic PDF printing
   //
   // pdf-file name: "path/to/tag" + "_" + __func__ + ".pdf"
   //
   // usage:
   // * single pdf, put the following line in a function
   //   pdf_print
   // * multiple pdfs in one file
   //   pdf_open
   //   pdf_print
   //   ...
   //   pdf_close
   // * enable print
   //   set_print("path/to/tag")
   // * function name is part of pdf file name
   //________________________________________
#define pdf_print	if (g_print.size()) {	\
      TCanvas* c1 = gPad->GetCanvas();		\
      c1->cd(); watermark();			\
      c1->Print(pdfile(__func__).c_str());	\
      gSystem->Sleep(1);			\
   }

#define pdf_open	if (g_print.size()) {		\
      std::string pdfs = pdfile(__func__) + "[";	\
      gPad->GetCanvas()->Print(pdfs.c_str());		\
   }

#define pdf_close	if (g_print.size()) {		\
      std::string pdfs = pdfile(__func__) + "]";	\
      gPad->GetCanvas()->Print(pdfs.c_str());		\
   }

   // path and head of pdf file
   //   empty = NOT print
   std::string g_print = "";

   inline bool to_print()
   { return g_print.size() > 0; }
   
   inline void set_print(const char* msg="")
   {
      g_print = msg;
      if (g_print.size() == 0) {
	 std::cout
	    << "   Usage: " << __func__
	    << "(\"path/to/tag\") to enable automatic PDF-print, which is disabled now by empty tag."
	    << std::endl;
      }
   }

   // pdf pathname
   inline std::string pdfile(const char* ss)
   { return g_print + "_" + ss + ".pdf"; }

   inline void watermark() {
      TString watermark = gSystem->Getenv("USER");
      watermark += " @ " + gSystem->GetFromPipe("date");	// Execute command and return output in TString
      watermark += " - ROOT ";
      watermark += gROOT->GetVersion();
      TLatex* tex = new TLatex(0.01, 0.01, watermark);
      tex->SetNDC();		// NDC coordinates [0,1]
      tex->SetTextColor(17);
      tex->SetTextAlign(11);
      tex->SetTextSize(0.015);
      // tex->SetTextAngle(26.15998);
      // tex->SetLineWidth(2);
      tex->Draw();
   }
   //----------------------------------------end of print-utility

   // draw a straight line
   inline
   void draw_line(double x0, bool vertical, bool dotxt, int stl, int clr)
   {
      gPad->Update();
      double x1, y1, x2, y2;
      if (vertical) {
	 x1 = x2 = x0;
	 y1 = gPad->GetUymin();
	 y2 = gPad->GetUymax();
	 if (gPad->GetLogy()) {
	    y1 = pow(10, y1);
	    y2 = pow(10, y2);
	 }
      }
      else {
	 y1 = y2 = x0;
	 x1 = gPad->GetUxmin();
	 x2 = gPad->GetUxmax();
	 if (gPad->GetLogx()) {
	    x1 = pow(10, x1);
	    x2 = pow(10, x2);
	 }
      }
      TLine* line = new TLine(x1, y1, x2, y2);
      line->Draw();
      line->SetLineStyle(stl);
      line->SetLineColor(clr);

      // text
      if (dotxt) {
	 std::ostringstream oss;
	 oss << x0;
	 TText* txt = new TText(x2, y2, oss.str().c_str() );
	 txt->SetTextColor(clr);
	 if (vertical)
	    txt->SetTextAlign(21);
	 else
	    txt->SetTextAlign(12);
	 txt->Draw();
	 //cout << txt->GetTextSize() << endl;
	 txt->SetTextSize(0.03);
      }
   }


   // draw a function on a transparent pad
   //______________________________________________________________________
   inline
   void draw_pad(TF1 *f1, double xmin, double xmax, bool doYaxis)
   {
      // x-axis range
      if (xmax <= xmin) {
	 xmin = f1->GetXmin();
	 xmax = f1->GetXmax();
      }

      TPad *pad2 = new TPad("pad2", "", 0, 0, 1, 1);
      pad2->SetFillStyle(4000);	// transparent

      // pad global range
      double ymin = 0;
      double ymax = f1->GetMaximum(xmin, xmax) *1.1;
      double dx = (xmax - xmin)/0.8;
      double dy = (ymax - ymin)/0.8;
      pad2->Range(xmin-0.1*dx, ymin-0.1*dy, xmax+0.1*dx, ymax+0.1*dy);
      pad2->Draw();
      pad2->cd();

      // draw
      f1->Draw("same");

      // draw Yaxis
      if (doYaxis) {
	 TGaxis* axis = new TGaxis(xmax, ymin, xmax, ymax, ymin, ymax, 510, "+L");
	 axis->SetTitle(f1->GetYaxis()->GetTitle());
	 //axis->SetTitleOffset(1.2);
	 axis->SetMaxDigits(4);
	 int clr = f1->GetLineColor();
	 axis->SetLineColor(clr);
	 axis->SetTitleColor(clr);
	 axis->SetLabelColor(clr);
	 axis->Draw();
	 std::cout << axis->GetTitleOffset()
		   << " f1 " << f1->GetYaxis()->GetTitleOffset()
		   << std::endl;
      }
      
      std::cout << "pad2:"
		<< " xmin=" << xmin
		<< " ymin=" << ymin
		<< " xmax=" << xmax
		<< " ymax=" << ymax
		<< std::endl;

   }

   // draw functions on a transparent pad
   //______________________________________________________________________
   inline
   void draw_pad(TF1 **f1s, int nf1s, double xmin, double xmax)
   {
      TF1 *f1 = *f1s;
      if (xmax <= xmin) {
	 xmin = f1->GetXmin();
	 xmax = f1->GetXmax();
      }

      TPad *pad3 = new TPad("pad3", "", 0, 0, 1, 1);
      pad3->SetFillStyle(4000);	// transparent

      double ymin = 0;
      double ymax = f1->GetMaximum(xmin, xmax) *1.1;
      double dx = (xmax - xmin)/0.8;
      double dy = (ymax - ymin)/0.8;
      pad3->Range(xmin-0.1*dx, ymin-0.1*dy, xmax+0.1*dx, ymax+0.1*dy);
      pad3->Draw();
      pad3->cd();

      // draw functions
      for (int i=0; i < nf1s; i++) {
	 f1 = *(f1s+i);
	 f1->Draw("same");
      }
   }


   inline
   TMarker* draw_marker(double x, double y, int marker=20, int clr=2, double sz=0.8)
   {
      auto mkr = new TMarker(x, y, marker);
      mkr->SetMarkerColor(clr);
      mkr->SetMarkerSize(sz);
      mkr->Draw();
      return mkr;
   }

   
   // point + text
   inline
   TLatex* draw_note(double x, double y, const char *text
		    , double tsz=0.05, int marker=20, int clr=2, double msz=0.8)
   {
      auto mkr = new TMarker(x, y, marker);
      mkr->SetMarkerColor(clr);
      mkr->SetMarkerSize(msz);
      mkr->Draw();

      auto txt = new TLatex(x, y, text);
      txt->SetTextColor(clr);
      txt->SetTextSize(tsz);
      txt->Draw();
      return txt;
   }

   
} //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~namespace wm

#endif //~ common_root_h
