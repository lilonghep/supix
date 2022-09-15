// @(#) Version: $Id: test_main.cxx,v 0.0 2016/05/03 00:49:42 mwang Exp$
// Author: mwang@sdu.edu.cn 2016/05/03
#ifdef __ACLIC__
#include "SupixAnly.h"
#endif

#include "Timer.h"
#include "common_root.h"
using namespace wm;

#include "common.h"
using namespace std;


// test RecurStats
void test_RecurStats(int nn)
{
   TH1D* h1d = new TH1D("stats", "test RecurStats", 100, 0, 1);
   RecurStats rstats;
   for (int i=0; i < nn; i++) {
      double xi = gRandom->Rndm();
      rstats.add(xi);
      h1d->Fill(xi);
   }

   unsigned long N;
   double mean, sigma;
   rstats.get_results(N, mean, sigma);
   cout	<< __PRETTY_FUNCTION__ << " ---"
	<< " N=" << N << "/" << nn
	<< " RecurStats: mean=" << mean << " sigma=" << sigma * sqrt((double)(N-1)/N)
	<< " ROOT: mean=" << h1d->GetMean() << " sigma=" << h1d->GetRMS()
	<< " UnderFlow=" << h1d->GetBinContent(0)
	<< " OverFlow=" << h1d->GetBinContent(h1d->GetNbinsX()+1)
	<< endl;

}


void check_root()
{
   cout << __PRETTY_FUNCTION__;
#ifdef __ACLIC__
   cout << " __ACLIC__=" << __ACLIC__;
#endif
#ifdef __CLING__
   cout << " __CLING__=" << __CLING__;
#endif
#ifdef __ROOTCLING__
   cout << " __ROOTCLING__=" << __ROOTCLING__;
#endif
#ifdef __CINT__
   cout << " __CINT__=" << __CINT__;
#endif
#ifdef __MAKECINT__
   cout << " __MAKECINT__=" << __MAKECINT__;
#endif
   cout << endl;
}

//======================================================================
#if !defined(__CLING__) && !defined(__ROOTCLING__) && !defined(__ACLIC__)

int
main(int argc, char **argv)
{
   string pathbase = argv[0];
   pathbase = pathbase.substr(0, pathbase.find(".exe") );

   int nn = atoi(argv[1]);

   // string rootfile = pathbase + ".root";
   // TFile* tfile = new TFile(rootfile.c_str(), "NEW");
   // tfile->Close();

   test_RecurStats(nn);
   check_root();

   return 0;
}

#endif
//======================================================================

void
test()
{
   int foo = 1;
   bool yes;
   if ( ( yes = (foo == 1) ) ) {
      cout << yes << " " << foo << endl;
   }

}
