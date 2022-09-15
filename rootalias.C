////////////////////////////////////////////////////////////////////////////////
// $Id: rootalias.C 1050 2019-04-03 07:01:35Z mwang $
//
// Frequently used functions
//
////////////////////////////////////////////////////////////////////////////////
#ifndef ROOTALIAS
#define ROOTALIAS

#include "TMath.h"
#include "TCanvas.h"
#include "TPaveStats.h"
#include "TH1F.h"

#include <sstream>
using namespace std;


//______________________________________________________________________________
TH1F* logXaxis(const char* name, const char* title, const Int_t nbins, const Double_t xmin, const Double_t xmax)
// create a TH1 with log Xaxis.
// c1->SetLogx(); before drawing.
{
   Double_t logxmin = log10(xmin);
   Double_t logxmax = log10(xmax);
   Double_t binwidth = (logxmax-logxmin)/nbins;
   Double_t xbins[nbins+1];
   xbins[0] = xmin;
   for (Int_t i=1;i<=nbins;i++) {
      xbins[i] = xmin + TMath::Power(10,logxmin+i*binwidth);
      //cout<<i<<": "<<xbins[i]<<endl;
   }
   TH1F *h = new TH1F(name, title, nbins, xbins);
   return h;
}

//______________________________________________________________________________
// (x1, y1): bottom-left
// (x2, y2): top-right
// use top-left corner as reference.
void moveStats(float x1, float y2=-1, float x2=-1, float y1=-1)
// move stats box to a new position.
{
   gPad->Update();	// why need this?
   TPaveStats* stats = (TPaveStats*)gPad->GetPrimitive("stats");

   // change to a new name
   int nn = (int)gRandom->Rndm()*1000;
   stringstream ss;
   ss<<"stats"<<nn;
   cout<<"moveStats: stats -> "<<ss.str()<<endl;
   stats->SetName(ss.str().c_str());

   float w = stats->GetX2NDC() - stats->GetX1NDC();
   float h = stats->GetY2NDC() - stats->GetY1NDC();
   if (x1 < 0) x1 = stats->GetX1NDC();
   if (y2 < 0) y2 = stats->GetY2NDC();
   if (x2 < x1) x2 = x1 + w;
   if (y1<0 || y1>y2) y1 = y2 - h;
   //cout<<x1<<","<<y1<<" -- "<<x2<<","<<y2<<endl;
   stats->SetX1NDC(x1);
   stats->SetX2NDC(x2);
   stats->SetY1NDC(y1);
   stats->SetY2NDC(y2);
   // paint with the current attributes
   stats->Paint("NDC");
}


//______________________________________________________________________________
void drawInset(TH1* hist, float x1, float y1, float w, float h)
// Draw a inset histogram.
//   x1, y1: bottom-left corner in NDC
//   w, h:   width and height in NDC
{

   TPad* npad = new TPad("npad", "", x1, y1, x1+w, y1+h);
   npad->SetFillStyle(0);
   npad->Draw();
   npad->cd();
   hist->Draw();
   npad->GetMother()->cd();
}


// dB (voltage) -> ratio
//______________________________________________________________________________
double dB_ratio(const double& dB, int factor = 20) {
   return TMath::Power(10, dB/factor);
}


// return a canvas
//______________________________________________________________________
TCanvas* getCanvas(const char* name="c1", const char* title="c1", int ww=1300, int wh=700) {
   TCanvas* canvas = (TCanvas*)gROOT->GetListOfCanvases()->FindObject(name);
   if (canvas == NULL) {
      canvas = new TCanvas(name, title, ww, wh);
   }
   else {
      canvas->Clear();
      canvas->cd();
   }
   return canvas;
}


// create a new tree, deleting an old one first if existing.
//______________________________________________________________________
TTree* newTree(const char* name="mytree", const char* title="mytree") {
   TTree *mytree = (TTree*)gROOT->FindObject(name);
   if (mytree != NULL) delete mytree;
   mytree = new TTree(name, title);
   return mytree;
}

#endif //~ ROOTALIAS
