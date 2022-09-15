/*******************************************************************//**
 * $Id: test_main.cxx 1222 2020-07-19 02:14:48Z mwang $
 *
 * test C/C++ functionality
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-21 16:26:56
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "common.h"
using namespace std;
#define LOG std::cout <<__PRETTY_FUNCTION__<<" --- "

enum { HELLO };
enum { WORLD };

// return char*
const char *test_str(const char *ss)
{
   static char str[100];
   sprintf(str, "%s: '%s'", __func__, ss);
   return str;
}

class MyClass
{
public:
   MyClass() {
      i++;
      LOG << i << endl;
   }
   ~MyClass() {
      LOG << i << endl;
      i--;
   }
   
private:
   static int i;
};

int MyClass::i = 0;

char const* test_str()
{
   static int nn = 0;
   static string str;
   ostringstream oss;
   oss << "test_str() " << nn++;
   str = oss.str();
   return str.c_str();
}

// ptr to a const char*
void const_str()
{
   const char* str = "hello";
   cout << __func__ << " test ptr to a const char*: " << str << endl;
   str = "world!";
   cout << __func__ << " test ptr to a const char*: " << str << endl;
}

const char* test_string()
{
   static int n=0;
   ostringstream oss;
   oss << __func__ << " " << n++;
   return oss.str().c_str();
}

void test_parray()
{
   double * const parr = new double[10];
   LOG << parr << " " << sizeof(parr) << " " << *parr << endl;
   for (int i=0; i < 10; i++) {
      *(parr + i) = sin(i);
      LOG << i << " " << *(parr+i) << endl;
   }
   delete []parr;
   LOG << parr << " " << sizeof(parr) << " " << *parr << endl;
}

void test_pptr()
{
   MyClass * pptr = new MyClass[3];
   delete []pptr;
}


int
main(int argc, char **argv)
//main(void)
{
   int xint = -1;
   if (argc > 1)	xint = atoi(argv[1]);
   
   // conditianal output
   cout << "conditional output: xint=" << xint
	<< " output " << ( xint < 0 ? "no" : to_string(xint) )
	<< endl;

   cout << "0 return const char*(): " << test_str() << endl;
   cout << "1 return const char*(): " << test_str() << endl;

   // pointer casting
   unsigned char* ptr = (unsigned char*)malloc(MAXLINE);
   cout << (void*)ptr
	<< " char* = " << (void*)ptr
	<< " short* = " << (short*)ptr
	<< " int* = " << (int*)ptr
	<< endl;
   delete ptr;

   const_str();

   for (int i=0; i < 3; i++)
      cout << test_string() << endl;

   // test bits
   unsigned short pixid;
   for (int ir=0; ir<4; ir++) {
      for (int ic=0; ic<3; ic++) {
	 pixid = (ir << 4) + ic;
	 cout << "pixid=" << pixid << "/0x" << hex << pixid << dec
	      << " ir=" << ir << " ic=" << ic
	      << endl;
      }
   }

   unsigned short foo = -1, bar = 61300;
   int result;
   result = foo - bar;
   cout << foo << " - " << bar << " = " << result << endl;

   cout << "enum:"
	<< " HELLO=" << HELLO
	<< " WORLD=" << WORLD
	<< endl;

   test_parray();
   test_pptr();
   
   return 0;
}
