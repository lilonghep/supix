/*******************************************************************//**
 * $Id: rootlogon.C 1274 2021-04-04 18:56:18Z mwang $
 *
 * - libraries defined in .rootmap will be loaded automatically.
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2019-03-05 16:23:42
 * @copyright:  (c)2019 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
{
   cout << "Loading ...... " << __FILE__ << endl;

   // ~/rootlib
   //___________________________________________________________________
   // load my libs
   gSystem->Load("libsupix");

   // add to libsupix after being developed.
   //gSystem->Load("SupixAnly_cxx");

   // add include path
   gSystem->AddIncludePath("-I$HOME/rootlib");

   // personal customisation
   //___________________________________________________________________

   gROOT->SetStyle("Plain");		// predefined: Default, Plain, Bold, Video
   gROOT->ForceStyle();	// force the current style attributes to be set 

   if (gROOT->GetVersionInt() >= 60400)	// 6.04
      gStyle->SetPalette(kRainBow);
   else
      gStyle->SetPalette(1);		// a predefined "pretty" palette
   gStyle->SetOptTitle(kTRUE);		// turn off title
   gStyle->SetOptStat(111111);
   gStyle->SetOptFit(1111);
   gStyle->SetMarkerColor(4);
   gStyle->SetMarkerStyle(20);
   gStyle->SetMarkerSize(0.7);
   gStyle->SetFuncColor(kRed);
   gStyle->SetFuncWidth(1);		// function line width
   gStyle->SetLineWidth(1);		// for c1->Print("xxx.pdf")
   gStyle->SetHistLineColor(kBlue);
   gStyle->SetHistLineWidth(1);

   gStyle->SetFrameFillColor(0);

}
