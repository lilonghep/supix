/* $Id: test_random_func.C 1050 2019-04-03 07:01:35Z mwang $
 * test function of randoms
 *
 */

// Z = X^2 + Y^2
// (X, Y) in a uniform rectangle
//______________________________________________________________________

// pdf(Z)
Double_t test1_pdfZ(Double_t *x, Double_t *par) {
   Double_t z = x[0];
   Double_t a = par[0], b = par[1], norm = par[2];
   Double_t pdf = 0;

   // let a < b
   if (a > b) {
      a = par[1];
      b = par[0];
   }

   // calculate: phi_max - phi_min
   if (z <= 0)
      pdf = 0;
   else if (z <= a*a)
      pdf = TMath::PiOver2();
   else if (z <= b*b)
      pdf = asin(a/sqrt(z));
   else	if (z <= a*a+b*b)
      pdf = asin(a/sqrt(z)) + asin(b/sqrt(z)) - TMath::PiOver2();
   else
      pdf = 0;

   return pdf*norm/(2*a*b);
}

void test1_func(Double_t a=3, Double_t b=4, Double_t nevts=1) {
   TF1 *f1 = new TF1("pdfZ", test1_pdfZ, 0, 1.1*(a*a+b*b), 3);
   f1->SetParameters(a, b, nevts);
   f1->SetParNames("a", "b", "norm");
   f1->Draw();
}

void test1(Double_t xmax=3, Double_t ymax=4, Int_t nevts=1e5) {

   // create a tree
   TTree* tree1 = newTree("tree1", "test function of randoms");

   // tree variables
   Double_t x, y, z;
   tree1->Branch("x", &x, "x/D");
   tree1->Branch("y", &y, "y/D");
   tree1->Branch("z", &z, "z/D");

   TRandom3 random;

   for (Int_t ievt=0; ievt < nevts; ievt++) {
      x = random.Uniform(0, xmax);
      y = random.Uniform(0, ymax);
      z = x*x + y*y;

      tree1->Fill();
   }

   c1 = getCanvas();
   c1->Divide(2);
   c1->cd(1);
   //tree1->Draw("y:x","","colz");
   tree1->Draw("y:x");

   c1->cd(2);
   tree1->Draw("z>>h1z(100)");

   TF1 *f1 = gROOT->GetFunction("pdfZ");
   if (f1 == NULL)
      test1_func(xmax, ymax, nevts*0.1);
   else
      f1->SetParameters(xmax, ymax, nevts*0.1);
   TH1F *h1z = (TH1F*)gDirectory->Get("h1z");
   h1z->Fit("pdfZ");
}


// (r, phi): uniform vs isotropic
// Z = X^2 + Y^2
//______________________________________________________________________
void test2(Int_t nevts=1e5, Double_t radius=1, const char* opt="") {
   TTree* tree2 = newTree("tree2", "test r-phi randoms");

   // tree variables
   Double_t r1, phi1, r2, phi2;
   tree2->Branch("r1", &r1, "r1/D");
   tree2->Branch("phi1", &phi1, "phi1/D");
   tree2->Branch("r2", &r2, "r2/D");
   tree2->Branch("phi2", &phi2, "phi2/D");

   TRandom3 random;

   for (Int_t ievt=0; ievt < nevts; ievt++) {
      r1	= random.Uniform(0, radius);
      phi1	= random.Uniform(0, TMath::TwoPi());

      r2	= sqrt(random.Uniform(0, radius*radius));	// r^2 in uniform
      phi2	= random.Uniform(0, r1*TMath::TwoPi())/r1;

      tree2->Fill();
   }
   //tree2->Print();

   c1 = getCanvas();
   c1->Divide(2);
   c1->cd(1);
   tree2->Draw("r1*sin(phi1):r1*cos(phi1)","",opt);
   c1->cd(2);
   tree2->Draw("r2*sin(phi1):r2*cos(phi1)","",opt);
}
