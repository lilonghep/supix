/*******************************************************************//**
 * $Id: test_thread.cxx 1193 2020-07-04 15:03:06Z mwang $
 *
 * test primitives of pthread
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-18 17:33:49
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#include "util.h"
#include "Timer.h"
#include "common.h"
using namespace std;

enum unit_tests { TIMING, ATOMIC_IO, RET_STR };
int  g_unit = 0;
int  g_maxloops = 10;
int  g_maxchilds = 0;

int  g_nchilds = 0;

class FooClass;
FooClass* g_foo = NULL;


// test class in threads
//______________________________________________________________________
class FooClass
{
public:
   int x;

   FooClass();
   ~FooClass();
   void step();
   void step_lock();
   inline void step_inline();
   inline void step_inline_lock();
   int get();
   int get_inline() { return x; }
   void print();
   
private:
   pthread_rwlock_t rwlock;
};


FooClass::FooClass()
{
   x = 0;
   int err = pthread_rwlock_init(&rwlock, NULL);
   if (err != 0)
	 exit(err);
   print();
}

FooClass::~FooClass()
{
   int err = pthread_rwlock_destroy(&rwlock);
   if (err != 0)
	 exit(err);
   print();
}

void FooClass::step()
{  x++; }

void FooClass::step_lock()
{
   pthread_rwlock_wrlock(&rwlock);
   x++;
   pthread_rwlock_unlock(&rwlock);
}

inline void FooClass::step_inline()
{  x++; }

inline void FooClass::step_inline_lock()
{
   pthread_rwlock_wrlock(&rwlock);
   x++;
   pthread_rwlock_unlock(&rwlock);
}

int FooClass::get() { return x; }

void FooClass::print()
{
   LOG << "x=" << x << endl;
}


// new thread
//______________________________________________________________________

void test_atomic_io()
{
   Timer timer;
   pthread_t tid = pthread_self();
   for (int i=0; i < g_maxloops; i++) {
      timer.start();
      CERR << tid << " " << i << " " << endl;
      timer.stop();
   }
   timer.print();
}

void test_timing()
{
   Timer timer_dir("direct");
   Timer timer_func("func");
   Timer timer_lock("lock");
   Timer timer_inline_func("inline func");
   Timer timer_inline_lock("inline lock");
   for (int i=0; i < g_maxloops; i++) {
      timer_dir.start();
      g_foo->x++;
      timer_dir.stop();

      timer_func.start();
      g_foo->step();
      timer_func.stop();

      timer_lock.start();
      g_foo->step_lock();
      timer_lock.stop();

      timer_inline_func.start();
      g_foo->step_inline();
      timer_inline_func.stop();

      timer_inline_lock.start();
      g_foo->step_inline_lock();
      timer_inline_lock.stop();
   }

   pthread_t tid = pthread_self();
   LOG << (void*)tid << " childs=" << g_maxchilds << ": maxloops=" << g_maxloops << endl;
   timer_dir.print();
   timer_func.print();
   timer_inline_func.print();
   timer_lock.print();
   timer_inline_lock.print();
}



//const char*
string
test_string()
{
   static int n=0;
   ostringstream oss;
   oss.str("");
   oss << "test_string: " << 1 + 2*n++ ;
   return oss.str();
}


void test_ret_str()
{
   pthread_t tid = pthread_self();
   for (int i=0; i < g_maxloops; i++) {
      CERR << tid << " #" << i
	   << " " << test_string()
	   << endl;
   }
}


// test atomic cout
void thr_func()
{

   switch (g_unit) {
   case TIMING:
      test_timing();
      break;
   case ATOMIC_IO:
      test_atomic_io();
      break;
   case RET_STR:
      test_ret_str();
      break;
   default:
      break;
   }
      
}

// // test ++
// void thr_func()
// {
//    pthread_t tid = pthread_self();
//    int nmix = 0;
//    for (int i=0; i < g_maxloops; i++) {
//       int xold = foo.get();
//       foo.step();
//       int xnew = foo.get();
//       if ( (xnew - xold) != 1) {
// 	 nmix++;
// 	 printf("WRONG 0x%lx: %d %d -> %d\n", (unsigned long)tid, xnew-xold, xold, xnew);
// 	 fflush(NULL);
//       }
//       usleep( random() % 1000 );
//    }
//    printf("sum 0x%lx: %d/%d = %.2f %%\n", (unsigned long)tid, nmix, g_maxloops, (float)nmix/g_maxloops*100);
//    fflush(NULL);
// }


void * new_thread(void *arg)
{
   (void)(arg);
   g_nchilds++;
   printids("child thread start:");
   thr_func();
   printids("child thread return:");
   g_nchilds--;
   return ((void*)0);
}


////////////////////////////////////////////////////////////////////////

void
usage(char **argv)
{
   cout << "Usage: " << argv[0] << " [options]" << endl
	<< "\t\t -n NUM		# loops" << endl
	<< "\t\t -t NUM		# child threas" << endl
	<< "\t\t -u ID		# test ID"
	<< ": " << TIMING << "=TIMING"
	<< ", " << ATOMIC_IO << "=ATOMIC_IO"
	<< ", " << RET_STR << "=RET_STR"
	<< endl
	<< endl;
   exit(0);
}

int
main(int argc, char **argv)
{
#ifdef _POSIX_THREADS
   printf("_POSIX_THREADS yes\n");
#else
   printf("_POSIX_THREADS no\n");
   return -1;
#endif

   // command line options and arguments
   int copt;
   // int xint;
   // unsigned long xulong;
   while ((copt = getopt(argc, argv, "n:t:u:")) != -1) {
      switch (copt) {
      case 'n':		// number of loops
	 g_maxloops = atoi(optarg);
         break;
      case 't':		// number of child threads
	 g_maxchilds = atoi(optarg);
         break;
      case 'u':		// unit test id
	 g_unit = atoi(optarg);
         break;
      case '?':
      default:
         usage(argv);
      }
   }
   // check non-option arguments
   if (argc == 1 || optind < argc) {
      usage(argv);
   }

   g_foo = new FooClass();
   
   for (int i=0; i < g_maxchilds; i++) {
      pthread_t ntid;
      int err = pthread_create(&ntid, NULL, new_thread, NULL);
      if (err != 0) {
	 cout << "cant't create thread: " << strerror(err) << endl;
	 exit(err);
      }
      cout << __func__ << "#" << i << ": nchilds=" << g_nchilds << endl;
   }

   printids("main thread:");
   //sleep(1);	// wait for thread start
   cout << __func__ << ": nchilds=" << g_nchilds << endl;
   thr_func();

   // finalize
   //
   while(g_nchilds > 0) {
      sleep(1);
   }
   cout << __func__ << ": nchilds=" << g_nchilds << endl;

   delete g_foo;
   
   return 0;
}
