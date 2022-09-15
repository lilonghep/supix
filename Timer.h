/*******************************************************************//**
 * $Id: Timer.h 1208 2020-07-11 15:32:21Z mwang $
 *
 * Timer
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-21 01:09:59
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#ifndef Timer_h
#define Timer_h

#include <cstdlib>	// linux: NULL
#include <cmath>
#include <sys/time.h>


//
// use recursive relations to calculate cumulative mean and variance.
//______________________________________________________________________
class RecurStats
{
public:
   RecurStats()
   {
      m_n = 0;
      m_mean = 0;
      m_variance = 0;
   }
   
   void add(double, int mm=1);		// add samples, mm: repeated number
   double get_mean()		{ return m_mean; }
   double get_variance()	{ return m_variance; }
   void get_results(unsigned long &nn, double &mean, double &sigma)
   { nn = m_n; mean = m_mean; sigma = sqrt(m_variance); }
   void get_results(double &mean, double &sigma)
   { mean = m_mean; sigma = sqrt(m_variance); }
   
private:
   unsigned long m_n;
   double m_mean;
   double m_variance;		// sum(x_i - mean)^2 / (n-1)
};



//
// time difference in usec.
// usage:
//   Timer timer("name");
//   timer.start();
//   ...
//   timer.stop();
//   ...
//   timer.print();
//______________________________________________________________________
class Timer
{
public:
   // constructor(s)
   Timer(const char *s="Timer")
      : name(s)
   {}

   // destructor
   virtual ~Timer();

   void start()
   {  gettimeofday(&m_tstart, NULL);  }

   void stop(int mm=1)		// mm: number of repeated samples
   {
      gettimeofday(&m_tstop, NULL);
      double xm = (double)get_dusec() / mm;
      m_stats.add(xm, mm);
   }

   double get_mean()		{ return m_stats.get_mean(); }
   double get_variance()	{ return m_stats.get_variance(); }
   
   // time difference in usec
   unsigned long get_dusec()
   { return (m_tstop.tv_sec - m_tstart.tv_sec)*1000000 + m_tstop.tv_usec - m_tstart.tv_usec; }

   // unit = 0 usec
   //        1 sec
   //        2 min
   //        3 hour
   void print(int unit=0);	// print mean and standard deviation
   //void print(const char *tag);
   //const char * sprint(const char *tag);

private:
   const char *name;
   struct timeval m_tstart, m_tstop;
   RecurStats m_stats;
   
};

#endif //~ Timer_h
