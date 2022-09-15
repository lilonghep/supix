/*******************************************************************//**
 * $Id: Nup_CL.cxx 1227 2020-07-20 16:16:58Z mwang $
 *
 * Estimate pipeline length based on Poisson distribution of events
 * with fixed rate.
 *
 * Tevt	: 1 / event_rate
 * Tin	: time for reading an event into pipeline
 * Tout	: time for writing an event out of pipeline
 * Xwave: Nframes per event for waveform write-out
 * CL	: 1 - p-value
 *
 * mean events in pipeline:
 * 	mu = Npipe * Tin / Tevt
 * resolve Nup with condition:
 * 	CL >= sum_{n=0}^{Nup} Poisson(n; mu)
 * resolve min Npipe with condition:
 *	Tout * Nup * Xwave < Tin * Npipe
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-07-19 18:21:04
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "common_root.h"
using namespace wm;
#include "common.h"
using namespace std;



// min N with prob > 1 - p
//______________________________________________________________________
int poisson_quantile(double mu, double pvalue)
{
   if (mu <= 0) return 0;
   double prob_min = 1 - pvalue;
   double prob = 0;
   int nmin = -1;
   while(prob < prob_min) {
      nmin++;
      prob += TMath::PoissonI((double)nmin, mu);
   }
   return nmin;
}

// // double version
// double xpoisson_quantile(double mu, double pvalue, double dx=0.1)
// {
//    if (mu <= 0) return 0;
//    double prob_min = 1 - pvalue;
//    double prob = 0;
//    double nmin = 0;
//    double flast = TMath::Poisson(0, mu);
//    double fx;
//    while(prob < prob_min) {
//       nmin += dx;
//       fx = TMath::Poisson(nmin, mu);
//       prob += 0.5 * dx * (flast + fx);
//       cout << nmin << " " << prob << " " << flast << " " << fx << endl;
//       flast = fx;
//       if (nmin > 10*mu) break;
//    }
//    return nmin;
// }


// min Nevts in pipeline with prob > 1 - p
//______________________________________________________________________
double Nup_CL(int Npipe, double Tevt, double Tin, double pvalue)
{
   double mu = Npipe * Tin / Tevt;
   return poisson_quantile(mu, pvalue);
}

double Nup_CL(double *x, double *par)
{
   return Nup_CL(*x, par[0], par[1], par[2]);
}

// example: Nup_CL(126, 1, 15, 5, 1e-6, 0, 10000)
//______________________________________________________________________
TObject* Nup_CL(double Tevt, double Tin, double Tout, int Xwave, double pvalue, double xmin=0, double xmax=10000)
{
   // read in to pipeline
   TF1* fin = new TF1("fin", Nup_CL, xmin, xmax, 3);
   fin->SetParameters(Tevt, Tin, pvalue);
   fin->Draw();
   fin->SetLineColor(4);
   fin->SetMinimum(0);
   fin->SetNpx(1000);
   fin->GetXaxis()->SetTitle("N_{pipeline}");
   ostringstream oss;
   //oss << "N_{up, CL = " << 1-pvalue << "}";
   oss << "N_{up, CL}";
   fin->GetYaxis()->SetTitle(oss.str().c_str() );
   
   // write out of pipeline, N*Tin/Tout/Xwave
   TF1* fout = new TF1("fout", "x * [0]/[1]/[2]", xmin, xmax);
   fout->SetParameters(Tin, Tout, Xwave);
   fout->Draw("same");
   fout->SetLineColor(2);
   fout->SetNpx(min(1e4, xmax-xmin) );

   // min pipeline
   int nmin = 1;
   do {
      nmin++;
      if (nmin > xmax)	break;
   }
   while(fout->Eval(nmin) < fin->Eval(nmin) );
   wm::draw_line(nmin);
   double mu = nmin * Tin/Tevt;	// mean Nevt
   
   // estimate slope
   int Nx2 = xmax;
   int Ny2 = fin->Eval(Nx2);
   while( Ny2 == fin->Eval(--Nx2) );
   int Nx1 = Nx2++;
   int Ny1 = fin->Eval(Nx1);
   while( Ny1 == fin->Eval(--Nx1) );
   Nx1++;
   int Nslope = Nx2 - Nx1;	// 1/slope
   double Xwave_max = Tin/Tout * Nslope;
   
   cout << "1/slope=" << Nslope
	<< " " << Nx2 << " " << Ny2
	<< " " << Nx1 << " " << Ny1
	<< endl;

   double xw = 0.23, yw = 0.23;
   double x1 = 0.13, y1 = 0.665;
   TPaveText* note = new TPaveText(x1, y1, x1+xw, y1+yw, "br NDC");
   note->SetTextAlign(12);
   note->SetFillColor(0);

   x1 += xw + 0.01;
   TLegend* lg = new TLegend(x1, y1, x1+xw, y1+yw);
   // lg->SetHeader(oss.str().c_str() );
   lg->AddEntry(fin, "N_{up, CL} vs N_{pipeline}", "L");
   lg->AddEntry(fout, "N_{out} = #frac{T_{in} N_{pipeline}}{T_{out} X_{wave}} > N_{up, CL}", "L");
   
   oss.str("");
   oss << "T_{evt} = " << Tevt;
   note->AddText(oss.str().c_str() );

   oss.str("");
   oss << "T_{in} = " << Tin
       << ", T_{out} = " << Tout
       << ", X_{wave} = " << Xwave
      ;
   note->AddText(oss.str().c_str() );

   oss.str("");
   oss << "CL = " << 1-pvalue;
   note->AddText(oss.str().c_str() );

   note->AddText("");
   note->AddLine();
   
   oss.str("");
   oss << "N_{pipeline} > " << nmin;
   note->AddText(oss.str().c_str() );
   
   oss.str("");
   oss << "#LT N_{evt} #GT = " << fixed << setprecision(1) << mu
       << ", N_{evt} #leq " << setprecision(0) << fin->Eval(nmin);
   note->AddText(oss.str().c_str() );

   oss.str("");
   oss << "X_{wave} < " << setprecision(1) << Xwave_max;
   note->AddText(oss.str().c_str() );
   
   lg->Draw();
   note->Draw();
   gStyle->SetOptTitle(0);		// turn off title
   gPad->Update();
   
   // double  Nlimit = (xmax - xmin) / (fin->Eval(xmax) - fin->Eval(xmin));
   cout << fin->GetName() << ":"
	<< " dN/dfin=" << Nslope
	<< " " << fout->GetName() << ":"
	<< " dN/dfout=" << Tout/Tin*Xwave
	<< " mu=" << mu
	<< " min_pipeline > " << nmin
	<< " Xwave < " << Xwave_max
	<< endl;
   
   return lg;
}
