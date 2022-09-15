/*******************************************************************//**
 * $Id: util.h 1191 2020-07-02 09:45:57Z mwang $
 *
 * utilities
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2020-06-18 16:17:58
 * @copyright:  (c)2020 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#ifndef util_h
#define util_h

#include <iostream>
#include <sstream>

#include <fcntl.h>
#include <sys/types.h>

//
// thread safe I/o
// ref:
//   https://www.techrepublic.com/article/use-stl-streams-for-easy-c-plus-plus-thread-safe-logging/#
//   https://stackoverflow.com/questions/15033827/multiple-threads-writing-to-stdcout-or-stdcerr
//______________________________________________________________________
class AtomicWriter
{
public:
   AtomicWriter(std::ostream& s = std::cout)	: m_stream(s) {}
   // copy-constructor for function's return-by-value
   AtomicWriter(const AtomicWriter& xx)	: m_stream(xx.m_stream) {}
   template <typename T>
   AtomicWriter& operator<<(const T& t) {
      m_oss << t;
      return *this;
   }
   AtomicWriter& operator<<( std::ostream& (*f) (std::ostream&) ) {
      m_oss << f;
      return *this;
   }
   ~AtomicWriter() { m_stream << m_oss.str(); }
private:
   std::ostringstream m_oss;
   std::ostream & m_stream;
};

// ensure temporary obj.
inline AtomicWriter __wm_log(std::ostream& s = std::cout) { static AtomicWriter log(s); return log; }
#define LOG	__wm_log() << __PRETTY_FUNCTION__ << " --- "
#define COUT	__wm_log(std::cout) //<< __PRETTY_FUNCTION__ << " --- "
#define CERR	__wm_log(std::cerr) << __PRETTY_FUNCTION__ << " --- "
#ifdef DEBUG
#define TRACE	static unsigned long _trace_count=0; LOG << "#" << _trace_count++ << std::endl;
#else
#define TRACE
#endif



// interface to open(2)
int open_fd(const char *path, int oflag = O_WRONLY | O_CREAT | O_TRUNC );
/***
	O_RDONLY        open for reading only
	O_WRONLY        open for writing only
	O_RDWR          open for reading and writing

	O_APPEND        append on each write
	O_CREAT         create file if it does not exist
	O_TRUNC         truncate size to 0
*/


// interface to close(2)
int close_fd(int fd, const char *msg="");

/*
  interface to read(2)

  Plain read() may not read all bytes requested in the buffer, so
  read_all() loops until all data was indeed read, or exits in
  case of failure, except for EINTR. The way the EINTR condition is
  handled is the standard way of making sure the process can be suspended
  with CTRL-Z and then continue running properly.

  The function has no return value, because it always succeeds (or exits
  instead of returning).

  The function doesn't expect to reach EOF either.
*/
ssize_t read_all(int fd, unsigned char *buf, size_t nbytes);


// write raw date into files
ssize_t write_all(int fd, unsigned char *buf, size_t nbytes);

// print thread information
void printids(const char *s);


//======================================================================
//
// math
//
//======================================================================

//
// propagation of errors
//

// y = (x1 + x2)/(x0 + x2)
void variance_ratio3(double x0, double V0, double x1, double V1, double x2, double V2, double& y, double& Vy);

// y = x / (1 + eps*(alpha - 1))
//   (x, eps, alpha) = (x0, x1, x2)
void variance_xea(double x0, double V0, double x1, double V1, double x2, double V2, double& y, double& Vy);



#endif //~ util_h
