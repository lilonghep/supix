/*******************************************************************//**
 * $Id: show_limits.cxx 1212 2020-07-13 06:13:52Z mwang $
 *
 * show runtime limits
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-21 16:26:56
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "common.h"
using namespace std;



int main(void)
{
   cout << "<limits:h>" << endl
	<< "\tlong\tsizeof=" << sizeof(long) << " LONG_MIN=" << LONG_MIN << " LONG_MAX=" << LONG_MAX << endl
	<< "\tint\tsizeof=" << sizeof(int) << " INT_MIN=" << INT_MIN << " INT_MAX=" << INT_MAX << endl
	<< "\tshort\tsizeof=" << sizeof(short) << " SHRT_MIN=" << SHRT_MIN << " SHRT_MAX=" << SHRT_MAX << endl
	<< "\tchar\tsizeof=" << sizeof(char) << " CHAR_MIN=" << CHAR_MIN << " CHAR_MAX=" << CHAR_MAX << endl
	<< "\tunsigned\tsizeof=" << sizeof(unsigned) << " UINT_MAX=" << UINT_MAX << endl
	<< "\tunsigned long\tsizeof=" << sizeof(unsigned long) << " ULONG_MAX=" << ULONG_MAX << endl
	<< "\tsize_t\tsizeof=" << sizeof(size_t) << endl
	<< "\tssize_t\tsizeof=" << sizeof(ssize_t) << endl
	<< "\ttime_t\tsizeof=" << sizeof(time_t) << endl
#ifdef _POSIX_THREADS
	<< "\tpthread_t\tsizeof=" << sizeof(pthread_t) << endl
#endif
	<< "<float.h> EPSILON: difference between 1 and the representable least value greater than 1." << endl
	<< "\tfloat\tsizeof=" << sizeof(float) << " FLT_MIN=" << FLT_MIN << " FLT_MAX=" << FLT_MAX << " FLT_EPSILON=" << FLT_EPSILON << endl
	<< "\tdouble\tsizeof=" << sizeof(double) << " DBL_MIN=" << DBL_MIN << " DBL_MAX=" << DBL_MAX << " DBL_EPSILON=" << DBL_EPSILON << endl
	<< "\tlong double\tsizeof=" << sizeof(long double) << " LDBL_MIN=" << LDBL_MIN << " LDBL_MAX=" << LDBL_MAX << " LDBL_EPSILON=" << LDBL_EPSILON << endl
	// << " ULONG_MAX " << ULONG_MAX
	// << " ULLONG_MAX " << ULLONG_MAX
      //<< " SIZE_MAX " << SIZE_MAX
	<< endl;

   cout << "rand()";
   for (int i=0; i < 10; i++)
      cout << " " << rand();
   cout << endl;

   cout << "random()";
   for (int i=0; i < 10; i++)
      cout << " " << random();
   cout << endl;
   
   return 0;
}
