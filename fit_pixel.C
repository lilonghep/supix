/*******************************************************************//**
 * $Id: fit_pixel.C 1277 2021-04-13 08:20:01Z mwang $
 *
 * Fit central pixel
 *
 * Usage:
 * 	.L fit_pixel.C+
 *	find_peaks(h1)
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2021-03-30 10:28:23
 * @copyright:  (c)2021 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "common.h"
#include "common_root.h"

// utilities
//======================================================================

// number of parameters
const int kNcrystal = 5;	// crystal ball
const int kNgaus = 3;
const int kNbipeak = 8;
const int kNmpv = 6;
const int kNped = 2;
const int kNbigaus = 6;
const int kNlandau = 3;

const char* g_names[] =
   { "CB:#alpha", "CB:n",
     "A_{#alpha}", "#mu_{#alpha}", "#sigma_{#alpha}",
     "A_{#beta}", "#mu_{#beta}", "#sigma_{#beta}",
     "E:C", "E:slope",
     "G1:A", "G1:#mu", "G1:#sigma",
     "G2:A", "G2:#mu", "G2:#sigma",
     "G3:A", "G3:#mu", "G3:#sigma",
   };

// global for sharing
struct spectrum_t {
   TH1* h1;
   TH1* hminus;
   double mean;
   double rms;
   double xped;
   double xmpv;
   double xKalpha;
   double valleyL;	// left of MPV
   double valleyR;	// right of MPV
   double Ealpha = 5900;	// eV
   double Ebeta = 6500;	// eV
   TFitResultPtr frp;
   int npar = 0;
   double par[99];	// fit par's: bipeak...mpv..expo
   TF1* f_bipeak = NULL;
   TF1* f_mpv = NULL;
   TF1* f_ped = NULL;
   TF1* f_mpv_ped = NULL;
   TF1* f_full = NULL;
   // component functions
   int nf1s;
   TF1 *f1s[9];
   void init();
   void print();
   void update(TF1 *f0);	// pars of component functions
} g_spectrum;

void spectrum_t::init()
{
   f_bipeak = f_mpv = f_ped = f_mpv_ped = f_full = NULL;
   nf1s = 0;
}

// f0: full function
void spectrum_t::update(TF1 *f0)
{
   // update pars of component functions
   int npar = 0;
   while (npar < f0->GetNpar() ) {
      for (int i=0; i < g_spectrum.nf1s; i++) {
	 auto f1 = g_spectrum.f1s[i];
	 for (int ip=0; ip < f1->GetNpar(); ip++) {
	    f1->SetParameter(ip, f0->GetParameter(npar++) );
	 }
      }
   }
   // g_spectrum.f1s[g_spectrum.nf1s++] = f0;
   g_spectrum.f_full = f0;
}

void spectrum_t::print()
{
   cout << h1->GetName() << ":"
	<< " mean=" << mean
	<< " rms=" << rms
	<< " xped=" << xped
	<< " xmpv=" << xmpv
	<< " xKalpha=" << xKalpha
	<< " valleyL=" << valleyL
	<< " valleyR=" << valleyR
	<< endl;
}

//......................................................................

// whether a par fixed?
bool is_fixed(TF1 *f1, int ipar)
{
   double pmin, pmax;
   f1->GetParLimits(ipar, pmin, pmax);
   if (pmin == pmax && pmin != 0)
      return true;
   else
      return false;
}

void fit_result(TFitResultPtr frp, TF1 *f1)
{
   std::cout
      << f1->GetName()
      << ": Status=" << frp->Status()
      << " Chi2=" << frp->Chi2()
      << " Ndf=" << frp->Ndf()
      << " Prob=" << frp->Prob()
      << " x-range(" << f1->GetXmin() << ", " << f1->GetXmax() << ")"
      << std::endl;
}
void fit_result(TFitResultPtr frp, const char *fname="")
{
   TF1 *f1 = (TF1*)gROOT->GetListOfFunctions()->FindObject(fname);
   fit_result(frp, f1);
}

// scan fit range for max prob.
//______________________________________________________________________
void fit_optimise(TH1 *h1, TF1 *f1, int minbins=10, bool debug=false)
{
   double xmin = f1->GetXmin();
   double xmax = f1->GetXmax();
   double prob_max = -1;
   double xmin_opt = xmin;
   int bmin = h1->FindBin(xmin);
   int bmax = h1->FindBin(xmax);
   minbins = min(minbins, bmax-bmin);
   int bmin_opt = bmin;
   int bmax_opt = bmax;
   int bmin_orig = bmin;
   int bmax_orig = bmax;
   int npar = f1->GetNpar();
   double pars_opt[npar];
   TFitResultPtr frp;

   LOG << "original range (" << xmin << ", " << xmax << ")" << endl;
   
   // ...bmin
   // scan left side of bmin for max prob
   if (debug)
      cout << "scan left side of bmin of (" << bmin << ", " << bmax << ")" << endl;
   do {
      xmin = h1->GetBinCenter(bmin);
      f1->SetRange(xmin, xmax);
      frp = h1->Fit(f1, "SRQ");
      if (debug) fit_result(frp, f1);
      if (frp->Status() )
	 break;
      double prob = frp->Prob();
      if (prob > prob_max) {
	 prob_max = prob;
	 bmin_opt = bmin;
	 f1->GetParameters(pars_opt);
      }
   }
   while (--bmin);

   // bmin...
   // scan right side of bmin for max prob
   bmin = bmin_orig;
   if (debug)
      cout << "scan right side of bmin of (" << bmin << ", " << bmax << ")" << endl;
   do {
      xmin = h1->GetBinCenter(bmin);
      f1->SetRange(xmin, xmax);
      frp = h1->Fit(f1, "SRQ");
      if (debug) fit_result(frp, f1);
      if (frp->Status() )
	 break;
      double prob = frp->Prob();
      if (prob > prob_max) {
	 prob_max = prob;
	 bmin_opt = bmin;
	 f1->GetParameters(pars_opt);
      }
   }
   while (++bmin < bmax - minbins);

   // bmax...
   // scan right side of bmax for max prob
   bmin = bmin_opt;
   xmin = h1->GetBinCenter(bmin);
   if (debug)
      cout << "scan right side of bmax of (" << bmin << ", " << bmax << ")" << endl;
   do {
      xmax = h1->GetBinCenter(bmax);
      f1->SetRange(xmin, xmax);
      frp = h1->Fit(f1, "SRQ");
      if (debug) fit_result(frp, f1);
      if (frp->Status() )
	 break;
      double prob = frp->Prob();
      if (prob > prob_max) {
	 prob_max = prob;
	 bmax_opt = bmax;
	 f1->GetParameters(pars_opt);
      }
   }
   while (++bmax < h1->GetNbinsX() );

   // ...bmax
   // scan left side of bmax for max prob
   bmax = bmax_orig;
   xmax = h1->GetBinCenter(bmax);
   if (debug)
      cout << "scan left side of bmax of (" << bmin << ", " << bmax << ")" << endl;
   do {
      xmax = h1->GetBinCenter(bmax);
      f1->SetRange(xmin, xmax);
      frp = h1->Fit(f1, "SRQ");
      if (debug) fit_result(frp, f1);
      if (frp->Status() )
	 break;
      double prob = frp->Prob();
      if (prob > prob_max) {
	 prob_max = prob;
	 bmax_opt = bmax;
	 f1->GetParameters(pars_opt);
      }
   }
   while (--bmax > bmin + minbins);

   // optimized fit with max prob
   bmax = bmax_opt;
   xmax = h1->GetBinCenter(bmax);
   f1->SetRange(xmin, xmax);
   for (int i=0; i < f1->GetNpar(); i++) {
      if (is_fixed(f1, i) )
	 f1->FixParameter(i, pars_opt[i]);
      else
	 f1->SetParameter(i, pars_opt[i]);
   }
   frp = h1->Fit(f1, "SR");
   if (debug) fit_result(frp, f1);
   LOG << f1->GetName() << " optimised range (" << xmin << ", " << xmax << ")" << endl;
}

void fit_optimise(TH1 *h1, const char *fname="", bool debug=false)
{
   TF1 *f1 = h1->GetFunction(fname);
   fit_optimise(h1, f1, debug);
}


// position at ratio
double xratio(double x1, double x2, double ratio=0.3)
{
   return x1 + ratio * (x2 - x1);
}

//......................................................................


// peaks and other characteristic parameters
//______________________________________________________________________
void find_peaks(TH1* h1)
{
   g_spectrum.init();
   g_spectrum.h1 = h1;
   double bwid = h1->GetBinWidth(1);
   double xmin_save = 0;	//h1->GetXaxis()->GetXmin();
   double xmax_save = 250-bwid*0.5;	//h1->GetXaxis()->GetXmax();
   
   h1->ShowPeaks();
   auto funs = h1->GetListOfFunctions();
   auto pm = (TPolyMarker*)funs->FindObject("TPolyMarker");
   double *pmX = pm->GetX();
   double *pmY = pm->GetY();

   g_spectrum.mean = h1->GetMean();
   g_spectrum.rms = h1->GetRMS();

   // peak of Kalpha & pedestal
   double xmpv = pmX[0];
   double xKalpha = -1;
   double xped = -1;
   for (int i=0; i < pm->Size(); i++) {
      cout << i << " (" << pmX[i] << ", " << pmY[i] << ")" << endl;
      if (pmX[i] < xmpv) {	// pedestal
	 xped = pmX[i];
      }
      else if (pmX[i] > xKalpha) {	// right-most peak
	 xKalpha = pmX[i];
      }
   }
   g_spectrum.xped = xped;
   g_spectrum.xmpv = xmpv;
   g_spectrum.xKalpha = xKalpha;

   // valley
   double xmin = g_spectrum.xmpv;
   double xmax = g_spectrum.xKalpha;
   h1->SetAxisRange(xmin, xmax);
   g_spectrum.valleyR = h1->GetBinCenter(h1->GetMinimumBin());

   xmin = xped;
   xmax = g_spectrum.xmpv;
   h1->SetAxisRange(xmin, xmax);
   g_spectrum.valleyL = h1->GetBinCenter(h1->GetMinimumBin());
   
   h1->SetAxisRange(xmin_save, xmax_save);
   g_spectrum.print();
   
}

// crystal ball function
//______________________________________________________________________
double crystal_ball(double x, double alpha, double nth, double norm, double mu, double sigma)
{
   double xi = (x - mu) / sigma;
   if (alpha < 0) alpha *= -1;

   double rv = 0;
   if (xi > -alpha) {
      rv = exp(-0.5*xi*xi);
   }
   else {
      rv = pow(nth/alpha / (nth/alpha - alpha - xi), nth);
      rv *= exp(-0.5*alpha*alpha);
   }

   return rv*norm;
}

const int kCBnorm = 2;		// norm position in p[]
double crystal_ball(double *x, double *p)
{
   return crystal_ball(*x, p[0], p[1], p[2], p[3], p[4]);
}

// reversed
double crystal_ball_r(double *x, double *p)
{
   return crystal_ball(-*x, p[0], p[1], p[2], -p[3], p[4]);
}


// two peaks of K-alpha/-beta: crystal ball + gaus
//______________________________________________________________________
double bipeak(double *x, double *p)
{
   const int bias = 5;
   double rv = crystal_ball(x, p);
   rv += p[bias] * TMath::Gaus(x[0], p[bias+1], p[bias+2], 0);
   return rv;
}


// fit function: gaus + gaus
//______________________________________________________________________
double bigaus(double *x, double *p)
{
   double rv = p[0] * TMath::Gaus(x[0], p[1], p[2], 0);
   rv += p[3] * TMath::Gaus(x[0], p[4], p[5], 0);
   return rv;
}


// mpv(3) + ped(2)
//______________________________________________________________________
double mpv_ped(double *x, double *par)
{
   double rv = par[0] * TMath::Landau(*x,par[1],par[2],false);
   rv += exp(par[3] + par[4]*x[0]);
   // rv += par[5];
   return rv;
}


// A0 3.5, 
void fit_mpv_ped(double xsigmaR=1/*, double bgd=500*/)
{
   TH1* h1 = g_spectrum.h1;

   if (!g_spectrum.f_mpv || !g_spectrum.f_ped) {
      cout << "Fit MPV and pedestal first!" << endl;
      return;
   }

   double fmin = g_spectrum.f_ped->GetXmin();
   double mpv = g_spectrum.f_mpv->GetParameter(1);
   double sigma = g_spectrum.f_mpv->GetParameter(2);
   double fmax = mpv + sigma * xsigmaR;
   // double fmax = g_spectrum.f_mpv->GetXmax();

   int inext = g_spectrum.f_mpv->GetNpar();
   int npar = g_spectrum.f_ped->GetNpar() + inext;
   // npar++;	// constant
   auto f_mpv_ped = new TF1("mpv_ped", mpv_ped, fmin, fmax, npar);
   for (int i=0; i < npar; i++) {
      if (i < inext) {
	 f_mpv_ped->SetParameter(i, g_spectrum.f_mpv->GetParameter(i));
	 f_mpv_ped->SetParName(i, g_spectrum.f_mpv->GetParName(i));
      }
      else {
	 f_mpv_ped->SetParameter(i, g_spectrum.f_ped->GetParameter(i-inext));
	 f_mpv_ped->SetParName(i, g_spectrum.f_ped->GetParName(i-inext));
      }
   }
   //f_mpv_ped->SetParameter(npar-1, 500);
   // f_mpv_ped->FixParameter(npar-1, bgd);
   
   TFitResultPtr frp;
   frp = h1->Fit(f_mpv_ped, "S", "", fmin, fmax);
   g_spectrum.frp = frp;
   g_spectrum.f_mpv_ped = f_mpv_ped;
   fit_result(frp, f_mpv_ped);
}


// pedestal
void fit_ped()
{
   TH1* h1 = g_spectrum.h1;
   double fmin = g_spectrum.xped;
   double fmax = xratio(fmin, g_spectrum.valleyL, 0.7);
   TFitResultPtr frp;
   frp = h1->Fit("expo", "S", "", fmin, fmax);
   auto f1 = (TF1*)h1->GetFunction("expo")->Clone("c_ped");
   // fit_optimise(h1, f1);
   frp = h1->Fit(f1, "SRI");
   fit_result(frp, f1);

   g_spectrum.f_ped = f1;
   g_spectrum.f1s[g_spectrum.nf1s++] = f1;
}



// fit mpv
//______________________________________________________________________
void fit_mpv()
{
   TH1* h1 = g_spectrum.h1;
   TFitResultPtr frp;
   double xmin, xmax;
   xmin = xratio(g_spectrum.valleyL, g_spectrum.xmpv, 0.5);
   xmax = xratio(g_spectrum.xmpv, g_spectrum.valleyR, 0.2);
   frp = h1->Fit("gaus", "S", "", xmin, xmax);
   auto f1 = (TF1*)h1->GetFunction("gaus")->Clone("G0");
   g_spectrum.f1s[g_spectrum.nf1s++] = f1;
   g_spectrum.f1s[g_spectrum.nf1s++] = (TF1*)f1->Clone("G1");

   xmax = xratio(g_spectrum.xmpv, g_spectrum.valleyR, 0.5);
   auto f_mpv = new TF1("mpv", bigaus, xmin, xmax, kNmpv);
   int npar = frp->NPar();
   for (int i=0; i < npar; i++) {
      f_mpv->SetParameter(i, frp->Parameter(i) );
   }
   double norm = frp->Parameter(0);
   double mean = frp->Parameter(1);
   double sigma = frp->Parameter(2);
   f_mpv->SetParameter(npar++, norm * 0.5);
   f_mpv->SetParameter(npar++, mean + sigma*2);
   f_mpv->SetParameter(npar++, sigma*2);
   frp = h1->Fit(f_mpv, "S", "", xmin, xmax);
   fit_result(frp, f_mpv);

   g_spectrum.f_mpv = f_mpv;
}



void fit_landau(double xsigmaL=1, double xsigmaR=1)
{
   TH1* h1 = g_spectrum.h1;
   // fit results
   TFitResultPtr frp;
   double chi2, ndf;
   double fmin, fmax;
   
   fmin = xratio(g_spectrum.valleyL, g_spectrum.xmpv, 0.5);
   fmax = xratio(g_spectrum.xmpv, g_spectrum.valleyR, 0.1);
   frp = h1->Fit("landau", "SQ", "", fmin, fmax);
   auto f1 = (TF1*)h1->GetFunction("landau");
   fit_result(frp, f1);
   
   // update fit range
   fmin = frp->Parameter(1) - frp->Parameter(2) * xsigmaL;
   fmax = frp->Parameter(1) + frp->Parameter(2) * xsigmaR;
   frp = h1->Fit("landau", "S", "", fmin, fmax);
   f1 = (TF1*)h1->GetFunction("landau");
   g_spectrum.frp = frp;
   g_spectrum.f_mpv = (TF1*)h1->GetFunction("landau")->Clone("c_mpv");
   fit_result(frp, f1);
}



// fit with crystal ball
//   A0/A2/A5 3, A7 1.7
//______________________________________________________________________
int fit_bipeak(double xsigmaL=2, double xsigmaR=1)
{
   TH1* h1 = g_spectrum.h1;
   // fit results
   TFitResultPtr frp;
   double xmin, xmax;
   int npar, ip;	// index of par

   // initial pars
   //
   // fit 5.9 keV (k-alpha)
   xmin = xratio(g_spectrum.valleyR, g_spectrum.xKalpha);
   xmax = 2*g_spectrum.xKalpha - xmin;
   frp = h1->Fit("gaus", "SQ", "", xmin, xmax);
   double A0 = frp->Parameter(0);
   double mean0 = frp->Parameter(1);
   double sigma0 = frp->Parameter(2);

   // update range
   xmin = mean0 - sigma0 * xsigmaL;
   xmax = mean0 + sigma0 * xsigmaR;
   double alpha = 0.9;
   double nth = 0.01;
   auto f_Kalpha = new TF1("Kalpha", crystal_ball, xmin, xmax, 5);
   f_Kalpha->SetParameters(alpha, nth, A0, mean0, sigma0);
   frp = h1->Fit(f_Kalpha, "S", "", xmin, xmax);
   fit_result(frp, f_Kalpha);
   ip = kCBnorm;
   A0		= frp->Parameter(ip++);
   mean0	= frp->Parameter(ip++);
   sigma0	= frp->Parameter(ip++);
   g_spectrum.f1s[g_spectrum.nf1s++] = f_Kalpha;	// save component function

   // save parameters
   npar = frp->NPar();
   for (int i=0; i < npar; i++) {
      g_spectrum.par[i] = frp->Parameter(i);
   }
   g_spectrum.npar = npar;
   
   // K-beta
   //   pedestal ~ sigma0
   double A1, mean1, sigma1;
   double ratio = g_spectrum.Ebeta / g_spectrum.Ealpha;
   mean1 = mean0 * ratio - (ratio-1)*sigma0;
   sigma1 = sigma0;
   xmin = mean1 - sigma1 * 1.5;
   xmax = mean1 + sigma1 * 3;
   frp = h1->Fit("gaus", "S", "", xmin, xmax);
   npar += frp->NPar();
   auto f_Kbeta = (TF1*)h1->GetFunction("gaus")->Clone("Kbeta");
   fit_result(frp, f_Kbeta);
   g_spectrum.f1s[g_spectrum.nf1s++] = f_Kbeta;		// save component function
   
   // two peaks
   xmin = f_Kalpha->GetXmin();	// update left end
   auto f_bipeak = new TF1("bipeak", bipeak, xmin, xmax, npar);
   auto f1 = f_Kalpha;
   int bias = 0;
   for (int i=0; i < npar; i++) {
      if (i >= f_Kalpha->GetNpar() ) {
	 f1 = f_Kbeta;
	 bias = f_Kalpha->GetNpar();
      }
      f_bipeak->SetParameter(i, f1->GetParameter(i-bias) );
      f_bipeak->SetParName(i, g_names[i]);
   }
   // limits
   f_bipeak->SetParLimits(0, 0, 10);
   f_bipeak->SetParLimits(1, 0, 999);
   frp = h1->Fit(f_bipeak, "S", "", xmin, xmax);
   fit_result(frp, f_bipeak);

   // // optimise range
   // fit_optimise(h1, f_bipeak);
   // xmin = f_bipeak->GetXmin();
   // xmax = f_bipeak->GetXmax();
   // xmax = max(xmax, f_bipeak->GetParameter(3) + f_bipeak->GetParameter(4)*6);
   // f_bipeak->SetRange(xmin, xmax);
   // frp = h1->Fit(f_bipeak, "SRI");
   // fit_result(frp, f_bipeak);

   // save
   g_spectrum.frp = frp;
   g_spectrum.f_bipeak = f_bipeak;
   
   // update component functions
   npar = f_Kalpha->GetNpar();
   for (int i=0; i < f_bipeak->GetNpar(); i++) {
      double par = f_bipeak->GetParameter(i);
      if (i < npar) {
	 f_Kalpha->SetParameter(i, par);
      }
      else {
	 f_Kbeta->SetParameter(i-npar, par);
      }
   }

   gPad->Update();
   wm::set_stats(g_spectrum.h1, "", 0.75, 0.45, 0.99, 0.99);

   return (int)frp;
}


void note()
{
   double E0 = g_spectrum.Ealpha;
   double E1 = g_spectrum.Ebeta;
   int ip = kCBnorm;
   double A0	= g_spectrum.f_bipeak->GetParameter(ip++);
   double mean0	= g_spectrum.f_bipeak->GetParameter(ip++);
   double s0	= g_spectrum.f_bipeak->GetParameter(ip++);
   double A1	= g_spectrum.f_bipeak->GetParameter(ip++);
   double mean1	= g_spectrum.f_bipeak->GetParameter(ip++);
   double s1	= g_spectrum.f_bipeak->GetParameter(ip++);
   
   double pedestal = (mean0*E1 - mean1*E0)/(E1-E0);
   double calib = (mean1-mean0) / (E1 - E0);
   double ratio = A0 * s0 / (A1 * s1);
   
   // note
   gPad->Update();
   auto stats = (TPaveStats*)g_spectrum.h1->FindObject("stats");
   double x2 = stats->GetX1NDC() - 0.01;
   double y2 = stats->GetY2NDC();
   auto note = new TPaveText(x2-0.3, y2-0.25, x2, y2, "nb NDC");
   note->SetTextAlign(12);
   // note->SetFillColor(kWhite);

   ostringstream oss;
   oss << setprecision(3);
   
   oss << "Pedestal = #frac{#mu_{#alpha}E_{#beta} - #mu_{#beta}E_{#alpha}}{E_{#beta} - E_{#alpha}} = " << pedestal << " [ADC]";
   note->AddText(oss.str().c_str() );
   
   oss.str("");
   oss << "#frac{#mu_{#alpha,#beta} - P}{E_{#alpha,#beta}} = #frac{#mu_{#beta} - #mu_{#alpha}}{E_{#beta} - E_{#alpha}} = " << calib << " [ADC/eV]";
   note->AddText(oss.str().c_str() );

   // branching ratios of k-alpha and k-beta
   oss.str("");
   oss << "I_{#alpha} / I_{#beta} = (A_{#alpha}#sigma_{#alpha}) / (A_{#beta}#sigma_{#beta}) = " << ratio
      ;
   note->AddText(oss.str().c_str() );
   
   // // landau
   // double mpv = g_spectrum.f_mpv_ped->GetParameter(1);
   // double sigma = g_spectrum.f_mpv_ped->GetParameter(2);
   // oss.str("");
   // oss << "MPV = " << mpv
   //     << ", #sigma = " << sigma;
   // note->AddText(oss.str().c_str() );

   // // pedestal
   // oss.str("");
   // oss << "expo: p0 = " << g_spectrum.f_ped->GetParameter(0)
   //     << ", slope = " << g_spectrum.f_ped->GetParameter(1)
   //    ;
   // note->AddText(oss.str().c_str() );

   note->Draw();
}


// fit mpv peak with multi-gaus
//______________________________________________________________________
int fit_mpv_gaus(int ngaus=3)
{
   TH1* h1 = g_spectrum.h1;
   h1->Draw();

   // x-range limits
   double xxmin = xratio(g_spectrum.valleyL, g_spectrum.xmpv, 0.1);
   double xxmax = xratio(g_spectrum.xmpv, g_spectrum.valleyR, 0.8);
   
   // initial gaus
   double xmin = xratio(g_spectrum.valleyL, g_spectrum.xmpv, 0.5);
   double xmax = xratio(g_spectrum.xmpv, g_spectrum.valleyR, 0.2);
   double pars[ngaus*3];

   TF1 *f1=NULL, *g1=NULL;
   ostringstream sname;
   for (int ng=0; ng < ngaus; ng++) {
      ostringstream fname;
      fname << "mpv_G" << ng+1;
      sname << "gaus(" << 3*ng << ")";
      cout << fname.str() << " " << sname.str()
	   << " range(" << xmin << ", " << xmax << ")"
	   << endl;
      f1 = new TF1(fname.str().c_str(), sname.str().c_str(), xmin, xmax);
      // // pars limits
      // for (int i=0; i < ngaus; i++) {
      // 	 f1->SetParLimits(i*3+1, xxmin, xxmax);
      // }
      
      // initial pars
      if (ng > 0) {	// after G1
	 int ip = 3*ng;
	 pars[ip-3] *= 0.6;
	 pars[ip] = pars[ip-3] * 0.9;
	 pars[ip+1] = pars[ip-2] + pars[ip-1];
	 pars[ip+2] = pars[ip-1] * 1.1;
	 f1->SetParameters(pars);
	 xxmin = pars[ip-2];
	 f1->SetParLimits(ip+1, xxmin, xxmax);
	 f1->SetParLimits(ip+2, 0, xxmax);
      }
      else {
	 g1 = f1;
      }
      
      TFitResultPtr frp = h1->Fit(f1, "RS");
      // fit_result(frp, f1);
      f1->GetParameters(pars);
      // update fit range
      int ipmu = 3*ng + 1;	// last mean
      xmax = min(pars[ipmu] + pars[ipmu+1]*3, xxmax);
      
      // end
      sname << "+";
   }

   // par names
   for (int n=0; n < ngaus; n++) {
      ostringstream oss;
      oss << "G" << n+1;
      // par names
      string ss = oss.str();
      ss = oss.str() + ":A";
      f1->SetParName(n*3, ss.c_str() );
      ss = oss.str() + ":#mu";
      f1->SetParName(n*3+1, ss.c_str() );
      ss = oss.str() + ":#sigma";
      f1->SetParName(n*3+2, ss.c_str() );
   }
   TFitResultPtr frp = h1->Fit(f1, "RS");
   fit_result(frp, f1);
   g_spectrum.f_mpv = f1;
   
   // component functions
   f1->GetParameters(pars);
   for (int n=0; n < ngaus; n++) {
      ostringstream oss;
      oss << "G" << n+1;
      auto g1 = new TF1(oss.str().c_str(), "gaus", xmin, xmax);
      g_spectrum.f1s[g_spectrum.nf1s++] = g1;
      for (int j=0; j < 3; j++) {
   	 g1->SetParameter(j, pars[3*n+j] );
      }
      g1->SetLineColor(wm::colors[3+n]);
      g1->SetLineStyle(2+n);
      g1->Draw("same");
   }

   return (int)frp;
}


// full spectrum
//______________________________________________________________________
double full(double *x, double *p)
{
   double rv = bipeak(x, p);
   int bias = 8;
   rv += mpv_ped(x, p+bias);
   return rv;
}

// bipeak + pedestal
double full_ped(double *x, double *p)
{
   double rv = bipeak(x, p);
   int bias = kNbipeak;
   rv += exp(p[bias] + p[bias+1]*x[0]);
   return rv;
}

// bipeak + pedestal + gaus2
double full_gaus2(double *x, double *p)
{
   double rv = full_ped(x, p);
   int bias = kNbipeak + kNped;
   for (int i=0; i < 2; i++) {
      rv += p[bias] * TMath::Gaus(x[0], p[bias+1], p[bias+2]);
      bias += 3;
   }
   return rv;
}

// bipeak + pedestal + gaus3
double full_gaus3(double *x, double *p)
{
   double rv = full_ped(x, p);
   int bias = kNbipeak + kNped;
   for (int i=0; i < 3; i++) {
      rv += p[bias] * TMath::Gaus(x[0], p[bias+1], p[bias+2]);
      bias += 3;
   }
   return rv;
}


// ctrl = 1 : only +pedestal
void fit_full(int ctrl=0, double fmin=-1, double fmax=-1)
{
   TH1* h1 = g_spectrum.h1;
   if (!g_spectrum.f_bipeak || !g_spectrum.f_mpv || !g_spectrum.f_ped) {
      cout << "Fit component functions first!" << endl;
      return;
   }

   TFitResultPtr frp;

   // bipeak...
   // +pedestal
   double xmin = g_spectrum.f_ped->GetXmin();
   double xmax = g_spectrum.valleyL;
   int npar = g_spectrum.f_bipeak->GetNpar();
   auto f1 = new TF1("full_ped", full_ped, xmin, xmax, npar+kNped);
   for (int i=0; i < npar; i++) {
      f1->FixParameter(i, g_spectrum.f_bipeak->GetParameter(i));
      f1->SetParName(i, g_spectrum.f_bipeak->GetParName(i));
   }
   f1->SetParameter(npar++, 1);
   f1->SetParameter(npar++, -1);
   frp = h1->Fit(f1, "SR");
   fit_result(frp, f1);

   // optimise pedestal range
   fit_optimise(h1, f1);
   xmin = f1->GetXmin();
   xmax = f1->GetXmax();
   g_spectrum.f_ped->SetRange(xmin, xmax);
   
   if (ctrl == 1) {	// check residual of MPV
      g_spectrum.update(f1);
      return;
   }
   
   // ...+ gaus's
   int ngaus = g_spectrum.f_mpv->GetNpar() / 3;
   npar = kNbipeak + kNped + ngaus*3;
   TF1 *f_full = NULL;
   if (ngaus == 2) {
      f_full = new TF1("full_gaus2", full_gaus2, xmin, xmax, npar);
   }
   else if (ngaus == 3) {
      f_full = new TF1("full_gaus3", full_gaus3, xmin, xmax, npar);
   }
   f_full->SetNpx(1000);
   
   // par names
   for (int i=0; i < npar; i++) {
      f_full->SetParName(i, g_names[i]);
   }

   // init. pars
   npar = f1->GetNpar();
   for (int i=0; i < npar; i++) {
      f_full->FixParameter(i, f1->GetParameter(i));
   }
   for (int i=0; i < g_spectrum.f_mpv->GetNpar(); i++) {
      f_full->SetParameter(npar++, g_spectrum.f_mpv->GetParameter(i) );
   }
   
   // limited range
   xmin = g_spectrum.f_mpv->GetXmin();
   xmax = g_spectrum.f_mpv->GetXmax();
   frp = h1->Fit(f_full, "S", "", xmin, xmax);
   fit_result(frp, f_full);

   // extend fit range
   xmin = g_spectrum.valleyL;
   xmax = g_spectrum.valleyR;
   frp = h1->Fit(f_full, "S", "", xmin, xmax);
   fit_result(frp, f_full);

   // release pars of pedestal
   for (int i=0; i < kNped; i++) {
      f_full->ReleaseParameter(kNbipeak + i);
   }
   if (fmin > 0)
      xmin = fmin;	// manuel
   else
      xmin = g_spectrum.f_ped->GetXmin();
   //xmax = g_spectrum.f_mpv->GetXmax();
   frp = h1->Fit(f_full, "S", "", xmin, xmax);
   fit_result(frp, f_full);

   // release pars of bipeak
   for (int i=0; i < kNbipeak; i++) {
      f_full->ReleaseParameter(i);
   }
   if (fmax > xmin)
      xmax = fmax;	// manuel
   else
      xmax = g_spectrum.f_bipeak->GetXmax();
   f_full->SetRange(xmin, xmax);
   frp = h1->Fit(f_full, "SRI");
   fit_result(frp, f_full);

   // // optimise
   // fit_optimise(h1, f_full);
   // frp = h1->Fit(f_full, "SRI");
   // fit_result(frp, f_full);

   g_spectrum.frp = frp;
   g_spectrum.update(f_full);
   
   gPad->Update();
   wm::set_stats(g_spectrum.h1, "", 0.75, 0.25, 0.99, 0.99);
   
}


// draw function components
//______________________________________________________________________
void draw_all()
{
   double xmin, xmax;
   if (g_spectrum.f_ped)
      xmin = g_spectrum.f_ped->GetXmin();
   else
      xmin = g_spectrum.xped;
   
   if (g_spectrum.f_bipeak)
      xmax = g_spectrum.f_bipeak->GetXmax();
   else {	// h1 viewing range
      int blast = g_spectrum.h1->GetXaxis()->GetLast();
      xmax = g_spectrum.h1->GetBinCenter(blast);
   }

   g_spectrum.h1->Draw("E");
   g_spectrum.h1->SetMarkerSize(0.25);
   // g_spectrum.h1->SetMarkerColor(1);
   // g_spectrum.h1->SetLineColor(1);

   TF1 *f1;
   for (int i = 0; i < g_spectrum.nf1s; i++) {
      f1 = g_spectrum.f1s[i];
      f1->Draw("same");
      f1->SetRange(xmin, xmax);
      f1->SetLineColor(wm::colors[i+2]);
      f1->SetLineStyle(2+i);
      f1->SetNpx(1000);
      // cout << i << " -- " << f1->GetName() << endl;
   }

   // g_spectrum.f_bipeak->Draw("same");
   // g_spectrum.f_mpv_ped->Draw("same");

   // note();
}


// subtract fit functions
TH1D* residual()
{
   if (!g_spectrum.f_full) {
      cout << "fit full spectrum first!!!" << endl;
      return 0;
   }
   TH1* h1 = g_spectrum.h1;
   
   int nbins = h1->GetNbinsX();
   double xmin = h1->GetXaxis()->GetXmin();
   double xmax = h1->GetXaxis()->GetXmax();
   auto h2 = (TH1D*)gROOT->FindObject("hminus");
   if (h2) delete h2;
   h2 = new TH1D("hminus", "residual", nbins, xmin, xmax);
   // auto h2 = (TH1D*)h1->Clone("hnimus");
   // h2->Reset();

   // viewing range
   xmin = g_spectrum.f_ped->GetXmin();
   xmax = g_spectrum.f_bipeak->GetXmax();
   int bFirst = h1->FindBin(xmin);	// h1->GetXaxis()->GetFirst();
   int bLast  = h1->FindBin(xmax);	// h1->GetXaxis()->GetLast();
   //cout << "bins: " << bFirst << " -- " << bLast << endl;
   //h2->SetAxisRange(xmin, xmax);
   
   for (int ib=bFirst; ib <= bLast; ib++) {
      //if (h1->GetBinContent(ib) < 1) continue;
      double xv = h1->GetBinCenter(ib);
      double yv = h1->GetBinContent(ib);

      yv -= g_spectrum.f_full->Eval(xv);
      h2->SetBinContent(ib, yv);
   }

   g_spectrum.hminus = h2;
   return (TH1D*)h2;
}


void compare()
{
   auto h2 = residual();
   double ymin = h2->GetMinimum();
   if (ymin < 0) {
      ymin *= 1.1;
      g_spectrum.h1->SetMinimum(ymin);
   }
   draw_all();
   h2->Draw("same");
   h2->SetLineColor(49);
   //h2->SetStats(0);
}

// p0: A2/2, A8/1
void runme(TH1* h1, int ngaus=3, int ctrl=0)
{
   find_peaks(h1);
   if (fit_bipeak() ) return;
   fit_ped();
   if (fit_mpv_gaus(ngaus) ) return;
   //fit_mpv();
   //fit_mpv_ped();
   fit_full(ctrl);
   compare();
   note();
}
