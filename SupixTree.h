//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Thu Jul  9 15:12:42 2020 by ROOT version 6.18/04
// from TTree supix/test chip
// found on file: data/rawdata.root
//////////////////////////////////////////////////////////

#ifndef SupixTree_h
#define SupixTree_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.

class SupixTree {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
   Int_t           pixel_cds[64][16];
   UShort_t        pixel_adc[64][16];
   UShort_t        pixid[1024];
   ULong64_t       frame;
   UShort_t        npixs;
   UChar_t         trig;
   Char_t          fid;

   // List of branches
   TBranch        *b_pixel_cds;   //!
   TBranch        *b_pixel_adc;   //!
   TBranch        *b_pixid;   //!
   TBranch        *b_frame;   //!
   TBranch        *b_npixs;   //!
   TBranch        *b_trig;   //!
   TBranch        *b_fid;   //!

   SupixTree(TTree *tree=0);
   virtual ~SupixTree();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef SupixTree_cxx
SupixTree::SupixTree(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("data/rawdata.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("data/rawdata.root");
      }
      f->GetObject("supix",tree);

   }
   Init(tree);
}

SupixTree::~SupixTree()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t SupixTree::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t SupixTree::LoadTree(Long64_t entry)
{
// Set the environment to read one entry
   if (!fChain) return -5;
   Long64_t centry = fChain->LoadTree(entry);
   if (centry < 0) return centry;
   if (fChain->GetTreeNumber() != fCurrent) {
      fCurrent = fChain->GetTreeNumber();
      Notify();
   }
   return centry;
}

void SupixTree::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("pixel_cds", pixel_cds, &b_pixel_cds);
   fChain->SetBranchAddress("pixel_adc", pixel_adc, &b_pixel_adc);
   fChain->SetBranchAddress("pixid", pixid, &b_pixid);
   fChain->SetBranchAddress("frame", &frame, &b_frame);
   fChain->SetBranchAddress("npixs", &npixs, &b_npixs);
   fChain->SetBranchAddress("trig", &trig, &b_trig);
   fChain->SetBranchAddress("fid", &fid, &b_fid);
   Notify();
}

Bool_t SupixTree::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void SupixTree::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t SupixTree::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef SupixTree_cxx
