// $Id: libsupix_LinkDef.h 1153 2020-06-29 05:03:31Z mwang $
// trailing modifier:
//   -	not generate the Streamer() method
//   !	not generate the function operator>>(TBuffer &b, MyClass *&obj)
//   +	generate an automatic Streamer() for the class containing ClassDef;
//	otherwise, generate a ShowMember function and a Shadow Class.
//	mutually exclusive with `-'.
//
////////////////////////////////////////////////////////////////////////

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

// `+' for TObject based classes

//#pragma link C++ class SupixFPGA;
#pragma link C++ class RunInfo+;
#pragma link C++ class SupixTree;
#pragma link C++ class SupixAnly;
#endif
