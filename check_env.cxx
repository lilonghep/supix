/*******************************************************************//**
 * $Id: check_env.cxx 1096 2019-04-26 06:53:57Z mwang $
 *
 * sizeof types
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2019-04-16 11:22:13
 * @copyright:  (c)2019 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
//#include "check_env.h"

#include <unistd.h>     // for getopt()
#include <sys/types.h>	// ushort
#include <stdlib.h>

#include <iostream>
using namespace std;


int main(int argc, char **argv)
{
   cout << "compiler:"
	<< " __cplusplus " << __cplusplus
	<< endl;

   long long xllong;

   cout << "sizeof:"
	<< " char(" << sizeof(char) << ")"
	<< " short(" << sizeof(short) << ")"
	<< " int(" << sizeof(int) << ")"
	<< " long(" << sizeof(long) << ")"
	<< " long long(" << sizeof(xllong) << ")"
	<< " u_char(" << sizeof(u_char) << ")"
	<< " u_short(" << sizeof(u_short) << ")"
	<< " u_int(" << sizeof(u_int) << ")"
	<< " u_long(" << sizeof(u_long) << ")"
	<< " u_int(" << sizeof(u_int) << ")"
	<< " u_int32_t(" << sizeof(u_int32_t) << ")"
	<< " uint(" << sizeof(uint) << ")"
	<< endl;

   short *ptr;
   ptr = (short*)malloc(10 * sizeof(short) );
   cout << "sizeof(ptr) = " << sizeof(ptr)
	<< " sizeof(*ptr) = " << sizeof(*ptr)
	<< endl;

   free(ptr);

   cout << "std::getenv(\"USER\") " << std::getenv("USER")
	<< endl;
}
