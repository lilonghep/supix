#include <vector>
#include <cstring>
#include "stdio.h"
#include "fstream"
#include "iostream"
#include "TFile.h"
#include "TF1.h"
#include "TH1F.h"
#include "TH1D.h"
#include "TTree.h"
#include "TH2F.h"
#include "TPad.h"
#include "TH3F.h"
#include "TAttMarker.h"
#include "TLegend.h"
#include "TBranch.h"
#include "TCanvas.h"
#include "TVectorD.h"
#include "TStyle.h"
#include "TGraph.h"
#include "Riostream.h"
#include "stdlib.h"
#include "TMath.h"
#include <string.h>


std::string time_tag();

void supix_noise(){
   const int NROW = 64, NCOL = 16;
   long circle = 0;
   bool m_frame_first = true;
   int pixel_adc_this[NROW][NCOL], pixel_adc_last[NROW][NCOL];
   int waveform[NROW][NCOL], pixeltag[NROW][NCOL];




   ifstream flist("list.txt");
   ifstream fmatrix("matrix_info.txt");
   int* data_raw = new int[8];


   TH1D* h_cds[NROW][NCOL];
   char hname[80], htitle[80];
      for (int i=0; i < NROW; i++) {
         for(int j=0; j<NCOL; j++){
	 sprintf(hname,  "cds_%02d_%d", i, j);
	 sprintf(htitle, "CDS [%02d, %d];CDS", i, j);
	 h_cds[i][j] = new TH1D(hname, htitle, 100, -50, 50);
         }
      }
   cout<<"Create Histogram done!"<<endl;


   while(!flist.eof()){
      std::string name;
      flist>>name;
      cout<<name<<endl;

      FILE* fin= fopen(name.c_str(),"rb");//"raw_191105_102445.data","rb");
      if(fin ==NULL){
         cout<<"failed to open the file!"<<endl;
	 break;
      }
      else
      cout<<"read file ok!!!"<<endl;

   while(!feof(fin)){
      int pixel_adc[NROW][NCOL];
      int pixel_cds[NROW][NCOL];
      for(int i =0; i<NROW; i++){

         if(feof(fin)){
	    cout<<"end of the file!"<<endl;
	    break;
	 }
		 for(int j =0;j<NCOL;j++){

        	fread(&data_raw[0],1,1,fin);
        	fread(&data_raw[1],1,1,fin);
	 		fread(&data_raw[2],1,1,fin);
	 		fread(&data_raw[3],1,1,fin);
			pixel_adc[i][j] = data_raw[1]*256 + data_raw[0];
  			pixel_adc_this[i][j] = pixel_adc[i][j];
	    
	    	if(m_frame_first){
				waveform[i][j] = pixel_adc_this[i][j];
				pixeltag[i][j] = i;
	    	}           
 
            if(!m_frame_first){
               pixel_cds[i][j] = pixel_adc_this[i][j] - pixel_adc_last[i][j];
               
               h_cds[i][j] -> Fill(pixel_cds[i][j]);
               
            }
            pixel_adc_last[i][j] = pixel_adc_this[i][j];


         }   
      
      }


      m_frame_first = false;

   }			
	

   fclose(fin);

}

   flist.close();
   delete[] data_raw;


   string root_filename = "./Histogram/supix_noise_measurement_";
   int matrix_id;
   root_filename += time_tag();
   while(!fmatrix.eof()){
      fmatrix>>matrix_id;
   }
   fmatrix.close();
   cout<<matrix_id<<endl;
   if(matrix_id == 11){
      root_filename += "_A0.root";
   }
   
	else if(matrix_id == 13){
      root_filename += "_A2.root";
   }
   
	else if(matrix_id == 16){
      root_filename += "_A5.root";
   }
   
	else if(matrix_id == 18){
      root_filename += "_A7.root";
   }
   
	else if(matrix_id == 19){
      root_filename += "_A8.root";
   }
	else
 	root_filename+=".root";	

   TFile f (root_filename.c_str(),"RECREATE");
   
   for(int i=0; i<NROW;i++){
      for(int j=0; j<NCOL; j++){

         h_cds[i][j] -> GetYaxis() -> SetTitle("Entries");
         h_cds[i][j] -> GetXaxis() -> SetTitle("ADC");
         h_cds[i][j] -> Write();
      }   

   }
	cout<<"Save the Histogram done!"<<endl;
	f.Close();   
	int m_waveform[NCOL][NROW], m_pixel_tag[16][64];
	for(int j=0; j<NCOL; j++){
		for(int i=0; i<NROW; i++){
			m_waveform[j][i] = waveform[i][j];
			m_pixel_tag[j][i] = pixeltag[i][j];
		}
	}
	TGraph* g_[NCOL];
	for(int i =0; i<NCOL; i++){   
		g_[i] = new TGraph(NROW, m_pixel_tag[i], m_waveform[i]);
	}
	char gname[80];
	TCanvas* c1 = new TCanvas("c1");
	c1 -> Divide(2,2);
	for(int i =0; i<4; i++){
		c1 -> cd(i+1);
		sprintf(gname,"ADC_%02d",i);
		g_[i] -> Draw("APL");
		g_[i] -> SetTitle(gname);
		g_[i] -> GetXaxis() -> SetTitle("Pixel No.");
		g_[i] -> GetYaxis() -> SetTitle("ADC count");
		g_[i] -> SetMarkerStyle(20);
		g_[i] -> SetMarkerSize(1);
		g_[i] -> SetMarkerColor(04);
		g_[i] -> SetLineColor(04);
	}
	TCanvas* c2 = new TCanvas("c2");
	c2 -> Divide(2,2);
	for(int i =4; i<8; i++){
		c2 -> cd(i-3);
		sprintf(gname,"ADC_%02d",i);
		g_[i] -> Draw("APL");
		g_[i] -> SetTitle(gname);
		g_[i] -> GetXaxis() -> SetTitle("Pixel No.");
		g_[i] -> GetYaxis() -> SetTitle("ADC count");
		g_[i] -> SetMarkerStyle(20);
		g_[i] -> SetMarkerSize(1);
		g_[i] -> SetMarkerColor(04);
		g_[i] -> SetLineColor(04);
	}
	TCanvas* c3 = new TCanvas("c3");
	c3 -> Divide(2,2);
	for(int i =8; i<12; i++){
		c3 -> cd(i-7);
		sprintf(gname,"ADC_%02d",i);
		g_[i] -> Draw("APL");
		g_[i] -> SetTitle(gname);
		g_[i] -> GetXaxis() -> SetTitle("Pixel No.");
		g_[i] -> GetYaxis() -> SetTitle("ADC count");
		g_[i] -> SetMarkerStyle(20);
		g_[i] -> SetMarkerSize(1);
		g_[i] -> SetMarkerColor(04);
		g_[i] -> SetLineColor(04);
	}
	TCanvas* c4 = new TCanvas("c4");
	c4 -> Divide(2,2);
	for(int i =12; i<16; i++){
		c4 -> cd(i-11);
		sprintf(gname,"ADC_%02d",i);
		g_[i] -> Draw("APL");
		g_[i] -> SetTitle(gname);
		g_[i] -> GetXaxis() -> SetTitle("Pixel No.");
		g_[i] -> GetYaxis() -> SetTitle("ADC count");
		g_[i] -> SetMarkerStyle(20);
		g_[i] -> SetMarkerSize(1);
		g_[i] -> SetMarkerColor(04);
		g_[i] -> SetLineColor(04);
	}

}

std::string time_tag(){

   static unsigned nn = 0;
   time_t rawtime;
   static time_t lasttime = 0;
   struct tm *timeinfo;
   char fmt[] = "%y%m%d_%H%M%S";
   int size = sizeof(fmt);
   char buffer[14];
   string tag;

   time (&rawtime);
   timeinfo = localtime(&rawtime);

   strftime(buffer, size, fmt, timeinfo);
   tag = buffer;
   return tag;
}
