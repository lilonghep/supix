/*******************************************************************//**
 * $Id: Timer.cxx 1208 2020-07-11 15:32:21Z mwang $
 *
 * Timer
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-21 01:10:39
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "Timer.h"

#include <limits.h>
#include <stdio.h>

#include <iomanip>
#include <sstream>

// recursive statistics
// - a single measurement with <mm> counting
//______________________________________________________________________
void RecurStats::add(double xm, int mm)
{
   if (m_n >= ULONG_MAX) {	// check limits
      printf("%s exceeding ULONG_MAX=%lu\n", __PRETTY_FUNCTION__, ULONG_MAX);
      return;
   }
   
   // variance first
   if (m_n + mm - 1 > 0) {
      m_variance = m_variance * (m_n - 1) / (m_n + mm - 1)
	 + (xm - m_mean)*(xm - m_mean) * mm * m_n / (m_n+mm-1) / (m_n+mm) ;
   }
   
   // ... then mean
   m_mean = (m_n * m_mean + mm*xm) / (m_n + mm);

   // last
   m_n += mm;
// #ifdef DEBUG
//    std::ostringstream oss;
//    oss << __PRETTY_FUNCTION__
//        << " " << m_n << " " << mm << " " << xm
//        << " " << m_mean << " " << sqrt(m_variance)
//        << std::endl;
//    fputs(oss.str().c_str(), stdout);
// #endif

}

//======================================================================

//
// constructor(s)
//______________________________________________________________________
// Timer::Timer()
// {

// }


//
// destructor
//______________________________________________________________________
Timer::~Timer()
{

}

void Timer::print(int unit)
{
   char str[1000];
   unsigned long nn;
   double mean, sigma;
   m_stats.get_results(nn, mean, sigma);
   const char* sunit = "usec";
   if (unit == 1) {
      mean  /= 1e6;
      sigma /= 1e6;
      sunit  = "sec";
   }
   else if (unit == 2) {
      mean  /= 6e7;
      sigma /= 6e7;
      sunit  = "min";
   }
   else if (unit == 3) {
      mean  /= 3.6e9;
      sigma /= 3.6e9;
      sunit  = "hour";
   }
   sprintf(str, "\t%16s : %10.3f +- %10.3f %s \tx %lu\n", name, mean, sigma, sunit, nn);
   // std::ostringstream oss;
   // oss << tag << std::setprecision(3)
   //     << ": " << m_mean << " +- " << sqrt(m_variance) << " usec"
   //     << std::endl
   //    ;
   // fputs(oss.str().c_str(), stdout);
   fputs(str, stdout);
   //fputc('\n', stdout);
}

// const char * Timer::sprint(const char *tag)
// {
//    std::ostringstream oss;
//    oss << tag << std::setprecision(3)
//        << ": " << m_mean << " +- " << sqrt(m_variance) << " usec"
//        << std::endl
//       ;
//    return oss.str().c_str();
// }
