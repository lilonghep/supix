/*******************************************************************//**
 * $Id: test_hybrid.C 1096 2019-04-26 06:53:57Z mwang $
 *
 * @brief:      
 *
 * Description:
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2016-05-03 10:11:41
 * @copyright:  (c)2016 HEPG - Shandong University. All Rights Reserved.
 *
 ***********************************************************************/
#include "SupixFPGA.h"

/// system header files
#include <iostream>
using namespace std;


//======================================================================


void test_hybrid() {
   cout << "This message will appear BOTH in root and on command line." << endl;

#ifdef __CINT__
   cout << "__CINT__ defined!" << endl;
#endif

#ifdef __MAKECINT__
   cout << "__MAKECINT__ defined!" << endl;
#endif

   SupixFPGA *testClass = new SupixFPGA();
   delete testClass;
}


#ifndef __CINT__
int main()
{
   cout << "This message will appear ONLY on command line." << endl;

   test_hybrid();

   char buf[10];
   cout << "char buf[10]: buf= " << sizeof(buf) << " *buf= " << sizeof(*buf) << endl;

   cout << sizeof(STRTIME_FMT) << endl;

   return 0;
}
#endif
