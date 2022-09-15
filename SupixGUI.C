#include <TApplication.h>
#include <TGClient.h>
#include <TGButton.h>
#include <TRootEmbeddedCanvas.h>
#include <TGListBox.h>
#include <TList.h>
#include <TGFrame.h>
#include <RQ_OBJECT.h>
#include <TObject.h>
#include <TGLayout.h>
#include <TGWindow.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TString.h>
#include <TSystem.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
class SupixAnly;
SupixAnly *analy = 0;

class SupixGUI{
   RQ_OBJECT("SupixGUI")

private:
   TGMainFrame *fMain;
   TGHorizontalFrame *hframe1, *hframe2, *hframe3;
   TGVerticalFrame *vframe1, *vframe2, *vframe3, *vframe4, *vframe5 *vframe6, *vframe7;
   TRootEmbeddedCanvas *fEcanvas;
   TGListBox *fListBox, *fListBox_addr;
   TGCheckButton *fCheckMulti;
   TList *fSelected, *fSelected_addr;
   TGGroupFrame *fGframe1, *fGframe2, *fGframe3;
   TGNumberEntry *fNum1, *fNum2, *fNum3;
   TGLabel *fLabel1, *fLabel2, *fLabel3;
   TGTextButton *start, *reset, *quit, *draw, *exit, *addr_select, *addr_reset;
   
   Int_t id = 1;
   Int_t m_addr = 11;
   Int_t frame_num = 0;
   Int_t trig_num = 0;
   Int_t pipe_len = 1000;

public:
   SupixGUI(const TGWindow *p, UInt_t w, UInt_t h);
   virtual ~SupixGUI();
   void HandleButtons();
   void DoSetLabel(); 
   void DoStart();
   void DoReset();
   void DoQuit();
   void DoDraw();
   void DoExit();
   void DoAddr();
   void DoAddrReset();
//   string time_tag();
};

SupixGUI::SupixGUI(const TGWindow *p, UInt_t w, UInt_t h){
   //Create a main frame
   fMain = new TGMainFrame(p,w,h);



 

   //Create a horizontal frame to contain number entry and executable buttons
   TGHorizontalFrame *hframe3 = new TGHorizontalFrame(fMain,800,100);
   fMain -> AddFrame(hframe3,new TGLayoutHints(kLHintsTop | kLHintsLeft,5,5,5,5 ));
   
   // Create list box of Mode
   vframe5 = new TGVerticalFrame(hframe3,100,50);
   fListBox_addr = new TGListBox(vframe5, 89);
   fSelected_addr = new TList;
   char A0[20],A1[20],A2[20],A3[20],A4[20],A5[20],A6[20],A7[20],A8[20];
   sprintf(A0,"A0[0000]");
   fListBox_addr -> AddEntry(A0,11);
   sprintf(A1,"A1[0001]");
   fListBox_addr -> AddEntry(A1,12);
   sprintf(A2,"A2[0010]");
   fListBox_addr -> AddEntry(A2,13);
   sprintf(A3,"A3[0011]");
   fListBox_addr -> AddEntry(A3,14);
   sprintf(A4,"A4[0100]");
   fListBox_addr -> AddEntry(A4,15);
   sprintf(A5,"A5[0101]");
   fListBox_addr -> AddEntry(A5,16);
   sprintf(A6,"A6[0110]");
   fListBox_addr -> AddEntry(A6,17);
   sprintf(A7,"A7[0111]");
   fListBox_addr -> AddEntry(A7,18);
   sprintf(A8,"A8[1000]");
   fListBox_addr -> AddEntry(A8,19);
   fListBox_addr -> Resize(500,100);
   vframe5 -> AddFrame(fListBox_addr,new TGLayoutHints(kLHintsTop | kLHintsLeft ,5,5,5,5 ));
   hframe3 -> AddFrame(vframe5, new TGLayoutHints(kLHintsTop | kLHintsLeft, 50,50,5,5));
  



   vframe6 = new TGVerticalFrame(hframe3,100,100);
   addr_select = new TGTextButton(vframe6, "    &Address_Select    ");
   addr_select -> Connect("Clicked()", "SupixGUI", this, "DoAddr()");
   vframe6 -> AddFrame(addr_select, new TGLayoutHints(kLHintsCenterX, 50, 50, 10, 10));
   addr_reset = new TGTextButton(vframe6, "   &Address_Reset  ");
   addr_reset -> Connect("Clicked()", "SupixGUI", this, "DoAddrReset()");
   vframe6 -> AddFrame(addr_reset, new TGLayoutHints(kLHintsCenterX, 50, 50, 10, 10));
   hframe3 -> AddFrame(vframe6, new TGLayoutHints(kLHintsTop , 50,50,5,5));
   


   //Create a horizontal frame to contain number entry and executable buttons

   TGHorizontalFrame *hframe1 = new TGHorizontalFrame(fMain,800,100);
   fMain -> AddFrame(hframe1,new TGLayoutHints(kLHintsTop | kLHintsLeft,5,5,5,5 ));
   
   // Create list box of Mode
   vframe1 = new TGVerticalFrame(hframe1,100,50 );
   fListBox = new TGListBox(vframe1, 89);
   fSelected = new TList;
   char test[20],root[20],raw[20];
   sprintf(test,"Test_Mode");
   fListBox -> AddEntry(test,1);
   sprintf(root,"Root_Data");
   fListBox -> AddEntry(root,2);
   sprintf(raw,"Raw_Data");
   fListBox -> AddEntry(raw,3);
   fListBox -> Resize(150,100);


   vframe1 -> AddFrame(fListBox,new TGLayoutHints(kLHintsTop | kLHintsLeft ,5,5,5,5 ));
   fCheckMulti = new TGCheckButton(vframe1,"Multiple selection",10);
   vframe1 -> AddFrame(fCheckMulti, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5,5,5,5));
   fCheckMulti -> Connect("Clicked()", "SupixGUI", this, "HandleButtons()");
   hframe1 -> AddFrame(vframe1, new TGLayoutHints(kLHintsTop | kLHintsCenterX, 50,50,5,5));
   
   //Create the number entry of frame_to_be_record
   vframe2 = new TGVerticalFrame(hframe1,100,100 );
   hframe1 -> AddFrame(vframe2, new TGLayoutHints(kLHintsCenterX, 50,50,5,5));
   fNum1 = new TGNumberEntry(vframe2, 0, 9, 998, TGNumberFormat::kNESInteger,
   						TGNumberFormat::kNEANonNegative,
   						TGNumberFormat::kNELLimitMinMax,
   						0,999999);
   fNum1 -> Connect("ValueSet(Long_t)", "SupixGUI", this, "DoSetLabel()");
   (fNum1 -> GetNumberEntry()) -> Connect("ReturnPressed()", "SupixGUI",this, "DoSetLabel()");
   vframe2 -> AddFrame(fNum1, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5,5,5,5));
   fGframe1 = new TGGroupFrame(vframe2, "Frame Number");
   fLabel1 = new TGLabel(fGframe1,"No inputs");
   fGframe1 -> AddFrame(fLabel1, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5,5,5,5));
   vframe2 -> AddFrame(fGframe1, new TGLayoutHints(kLHintsTop | kLHintsLeft, 2,2,1,1));
   
   
   //Create the number entry of trigger number
   vframe3 = new TGVerticalFrame(hframe1,100,100 );
   hframe1 -> AddFrame(vframe3, new TGLayoutHints(kLHintsCenterX, 50,50,5,5));
   fNum2 = new TGNumberEntry(vframe3, 0, 9, 998, TGNumberFormat::kNESInteger,
   						TGNumberFormat::kNEAAnyNumber,
   						TGNumberFormat::kNELLimitMinMax,
   						-999999,999999);
   fNum2 -> Connect("ValueSet(Long_t)", "SupixGUI", this, "DoSetLabel()");
   (fNum2 -> GetNumberEntry()) -> Connect("ReturnPressed()", "SupixGUI",this, "DoSetLabel()");
   vframe3 -> AddFrame(fNum2, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5,5,5,5));
   fGframe2 = new TGGroupFrame(vframe3, "Trigger Number");
   fLabel2 = new TGLabel(fGframe2,"No inputs");
   fGframe2 -> AddFrame(fLabel2, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5,5,5,5));
   vframe3 -> AddFrame(fGframe2, new TGLayoutHints(kLHintsTop | kLHintsLeft, 2,2,1,1));
  
   //Create the number entry of pipeline length
   vframe7 = new TGVerticalFrame(hframe1,100,100 );
   hframe1 -> AddFrame(vframe7, new TGLayoutHints(kLHintsCenterX, 50,50,5,5));
   fNum3 = new TGNumberEntry(vframe7, 0, 9, 998, TGNumberFormat::kNESInteger,
   						TGNumberFormat::kNEAAnyNumber,
   						TGNumberFormat::kNELLimitMinMax,
   						10,999999);
   fNum3 -> Connect("ValueSet(Long_t)", "SupixGUI", this, "DoSetLabel()");
   (fNum3 -> GetNumberEntry()) -> Connect("ReturnPressed()", "SupixGUI",this, "DoSetLabel()");
   vframe7 -> AddFrame(fNum3, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5,5,5,5));
   fGframe3 = new TGGroupFrame(vframe7, "Pipeline length");
   fLabel3 = new TGLabel(fGframe3,"No inputs");
   fGframe3 -> AddFrame(fLabel3, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5,5,5,5));
   vframe7 -> AddFrame(fGframe3, new TGLayoutHints(kLHintsTop | kLHintsLeft, 2,2,1,1));
  
   //Create vertical frame to contain excutable buttons.

   
   vframe4 = new TGVerticalFrame(hframe1,100,100);
   hframe1 -> AddFrame(vframe4, new TGLayoutHints(kLHintsCenterX,50,50,5,5));
   
   reset = new TGTextButton(vframe4,"&RESET");
   reset -> Connect("Clicked()", "SupixGUI", this, "DoReset()");
   vframe4 -> AddFrame(reset, new TGLayoutHints(kLHintsTop | kLHintsCenterX, 5,5,5,5));
   
   start = new TGTextButton(vframe4,"&START");
   start -> Connect("Clicked()", "SupixGUI", this, "DoStart()");
   vframe4 -> AddFrame(start, new TGLayoutHints(kLHintsTop | kLHintsCenterX, 5,5,5,5));
   

   quit = new TGTextButton(vframe4,"&QUIT");
   quit -> Connect("Clicked()", "SupixGUI", this, "DoQuit()");
   vframe4 -> AddFrame(quit, new TGLayoutHints(kLHintsTop | kLHintsCenterX, 5,5,5,5));


/*
   //Create canvas widget
   fECanvas = new TRootEmbeddedCanvas("ECanvas",fMain,800,600);
   fMain -> AddFrame(fECanvas,new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,10,10,10,10));
*/
  

   

   


   //Create horizontal widget with Draw & Exit buttons
   hframe2 = new TGHorizontalFrame(fMain,800,50);
   fMain -> AddFrame(hframe2, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 10, 10, 10, 10));
   draw = new TGTextButton(hframe2, "                    &DRAW                    ");
   draw -> Connect("Clicked()", "SupixGUI", this, "DoDraw()");
   hframe2 -> AddFrame(draw, new TGLayoutHints(kLHintsCenterX, 50, 50, 10, 10));
   exit = new TGTextButton(hframe2, "                    &EXIT                    ");
   exit -> Connect("Clicked()", "SupixGUI", this, "DoExit()");
   hframe2 -> AddFrame(exit, new TGLayoutHints(kLHintsCenterX, 50, 50, 10, 10));
   



   // Set a name to the main frame
   fMain -> SetWindowName("SUPIX GUI");
   
   // Map all windows of main frame
   fMain -> MapSubwindows();
 
   //Initial the ladyout algorithm
   fMain -> Resize(fMain -> GetDefaultSize());

   //Map main frame
   fMain -> MapWindow();
   fListBox -> Select(1);
   fListBox_addr -> Select(11); 

}

void SupixGUI::HandleButtons(){
   TGButton *btn = (TGButton*)gTQSender;
   Int_t check_id =0;
   check_id = btn -> WidgetId();
   Printf("HandleButton: id = %d\n",check_id);
   
   if(check_id == 10){
      fListBox ->SetMultipleSelections(fCheckMulti -> GetState());
   }
}

void SupixGUI::DoSetLabel(){
   
   fLabel1 -> SetText(Form("%d",fNum1 -> GetNumberEntry() -> GetIntNumber()));
   fGframe1 -> Layout();
   frame_num = fNum1 -> GetNumberEntry() -> GetIntNumber();

   fLabel2 -> SetText(Form("%d",fNum2 -> GetNumberEntry() -> GetIntNumber()));
   fGframe2 -> Layout();
   trig_num = fNum2 -> GetNumberEntry() -> GetIntNumber();
   
   fLabel3 -> SetText(Form("%d",fNum3 -> GetNumberEntry() -> GetIntNumber()));
   fGframe3 -> Layout();
   pipe_len = fNum3 -> GetNumberEntry() -> GetIntNumber();
}

void SupixGUI::DoStart(){
   string log_inform;
   char cmd [100] = {0};
   fSelected -> Clear();
   if(fListBox -> GetMultipleSelections())
   id = 10;
   else
   id = fListBox -> GetSelected();
   cout << "Mode id: "<<id <<endl;
   if(id == 1){
      log_inform = "nohup ./daq.exe -TWR -a %d -n %d -t %d -L %d &"; 
      sprintf(cmd,log_inform.c_str(), m_addr -11, frame_num,trig_num, pipe_len);
      gSystem -> TSystem::Exec(cmd);
   }
   if(id == 2){
      log_inform = "nohup ./daq.exe -R -a %d -n %d -t %d -L %d &"; 
      sprintf(cmd,log_inform.c_str(), m_addr -11, frame_num,trig_num, pipe_len);
      gSystem -> TSystem::Exec(cmd);
   }
   if(id == 3){
      log_inform = "nohup ./daq.exe -W -a %d -n %d -t %d -L %d &"; 
      sprintf(cmd,log_inform.c_str(), m_addr -11, frame_num,trig_num, pipe_len);
      gSystem -> TSystem::Exec(cmd);
   }
   if(id == 10){
      log_inform = "nohup ./daq.exe -WR -a %d -n %d -t %d -L %d &"; 
      sprintf(cmd,log_inform.c_str(), m_addr -11, frame_num,trig_num, pipe_len);
      gSystem -> TSystem::Exec(cmd);
   }
}

void SupixGUI::DoReset(){
   id = 1;
   fListBox -> SetMultipleSelections(kFALSE);
   fListBox -> Select(1);
   cout<< "Mode id change to "<<id<<endl;
   frame_num = fNum1 -> TGNumberEntry::SetIntNumber(0);
   fGframe1 -> Layout();
   trig_num = fNum2 -> TGNumberEntry::SetIntNumber(0);
   fGframe2 -> Layout();
  
}

void SupixGUI::DoQuit(){
   gSystem -> TSystem::Exec("pkill daq.exe");
  // gSystem -> TSystem::Exec("root -l read_filename.C &");
   std::cout<<"********** Program was terminated! **********"<<std::endl;
}

void SupixGUI::DoDraw(){
   if (! analy) {
      gROOT->ProcessLine(".L SupixAnly.cxx+");
      gROOT->ProcessLine(".L analyser.C+");
      weave = analyser("./data/raw_*.root");
      draw_all();
      
   }
   cout<<"Done!"<<endl;
}

void SupixGUI::DoExit(){
   gApplication -> Terminate(0);
}


SupixGUI::~SupixGUI(){
   fMain -> Cleanup();
   delete fMain;
}
void SupixGUI(){
   new SupixGUI(gClient -> GetRoot(), 800,600);
}

void SupixGUI::DoAddr(){
	m_addr = fListBox_addr ->GetSelected();
	if(m_addr == 11){
		gSystem -> TSystem::Exec("../memwrite /dev/xillybus_mem_8 0 248");
		cout<<"Address set to A0"<<endl;
	}
	else if(m_addr == 13){
		gSystem -> TSystem::Exec("../memwrite /dev/xillybus_mem_8 0 249");
		cout<<"Address set to A2"<<endl;
	}
	else if(m_addr == 16){
		gSystem -> TSystem::Exec("../memwrite /dev/xillybus_mem_8 0 250");
		cout<<"Address set to A5"<<endl;
	}
	else if(m_addr == 18){
		gSystem -> TSystem::Exec("../memwrite /dev/xillybus_mem_8 0 251");
		cout<<"Address set to A7"<<endl;
	}
	else if(m_addr == 19){
		gSystem -> TSystem::Exec("../memwrite /dev/xillybus_mem_8 0 252");
		cout<<"Address set to A8"<<endl;
	}
	
	ofstream fout("matrix_info.txt");
	fout<<m_addr<<endl;
	fout.close();

}

void SupixGUI::DoAddrReset(){
	m_addr = 11;
	gSystem -> TSystem::Exec("../memwrite /dev/xillybus_mem_8 0 248");
	cout<<"Address set to A0"<<endl;
}
/*
string SupixGUI::time_tag(){

   static unsigned nn = 0;
   time_t rawtime;
   static time_t lasttime = 0;
   struct tm *timeinfo;
   char fmt[] = "%y%m%d_%H%M%S";
   int size = sizeof(fmt);
   char buffer[size];
   string tag;

   time (&rawtime);
   timeinfo = localtime(&rawtime);

   strftime(buffer, size, fmt, timeinfo);
   tag = buffer;
   return tag;
}
*/
