/*******************************************************************//**
 * $Id: common.h 1222 2020-07-19 02:14:48Z mwang $
 *
 * commonly used headers
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-17 18:47:06
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#ifndef common_h
#define common_h


// C headers
#include <errno.h>	// errno
#include <fcntl.h>	// O_WRONLY
#include <limits.h>
#include <float.h>	// Characteristics of floating-point types
#include <pthread.h>
#include <signal.h>	// signal
#include <stdio.h>	// perror
#include <stdlib.h>	// exit()
#include <string.h>	// strerror() on Linux
#include <sys/stat.h>	// mode_t
#include <sys/time.h>	// gettimeofday()
#include <sys/types.h>	// getpid()
#include <time.h>
#include <math.h>
#include <unistd.h>	// write(), open(), read(), ... getpid(), usleep(), getopt()

// C++ headers
#include <sstream>
#include <iostream>
#include <iomanip>

#define MAXLINE		4096

#endif //~ common_h
