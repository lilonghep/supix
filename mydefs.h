/*******************************************************************//**
 * $Id: mydefs.h 1212 2020-07-13 06:13:52Z mwang $
 *
 * my defines
 *
 *
 * @createdby:  WANG Meng <mwang@sdu.edu.cn> at 2019-04-19 00:11:11
 * @copyright:  (c)2019 HEPG - Shandong University. All Rights Reserved.
 ***********************************************************************/
#ifndef mydefs_h
#define mydefs_h

#include <cmath>

#include <iostream>
#define LOG std::cout <<__PRETTY_FUNCTION__<<" --- "
//#define CERR std::cerr <<__PRETTY_FUNCTION__<<" --- "

#ifdef DEBUG
#define TRACE	static unsigned long _trace_count=0; LOG << "#" << _trace_count++ << std::endl
#else
#define TRACE
#endif

#define kB	1024
#define MB	1048576		// kB * kB
#define GB	1073741824	// kB * MB
#define kiB	1000
#define MiB	1000000
#define GiB	1000000000

// chip parameters
#define NMATRIX		9		// we have 9 matrixes on chip --Long LI 2020-07-07
#define NROWS		64		// per frame
#define NCOLS		16		// per frame
#define NPIXS		1024		// = NROWS * NCOLS
#define FRAMESIZE	4096		// = 4 * NPIXS, bytes/frame
#define FID_MAX		16		// max frame-id, 4 bits

// #define STRTIME_FMT	"%y%m%d_%H%M%S"

// FPGA data fromat
#define NBITS_ADC	16
#define NBITS_COL	4
#define NBITS_ROW	8
#define NBITS_FID	4

#define MASK_ADC	0xFFFF
#define MASK_COL	0xF
#define MASK_ROW	0x7F	// 0xFF ???
#define MASK_FID	0xF


//
// data type for pixel information
//
typedef unsigned int	pixel_t;	// pixel data type, 32 bits
typedef unsigned short	adc_t;		// ADC, 16 bits
typedef int		cds_t;		// CDS, 32 bits
typedef char		fid_t;		// frame id, 4 bits effective
typedef unsigned char	trig_t;		// trigger,  8 bits


// DAQ mode
enum DAQ_MODE_t { M_NORMAL, M_NOISE, M_CONTINUOUS };

enum trig_types_t
   {
    TRIG_PERIOD	= 0x01,
    TRIG_CDS	= 0x02,
    TRIG_PRE	= 0x80,		// 128, pseudo-trig as a mark
    TRIG_TYPES	= 2		// real trigger Ntypes
   };


//======================================================================
//
// frequently used functions
//
//======================================================================

// convert a flost to per-cent with n digits after period
inline double per_centage(double x, int n=2)
{
   return pow(10, -n) * (int)(x * pow(10, 2+n) + 0.5);
}

#endif //~ mydefs_h
