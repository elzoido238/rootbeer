///
/// \file Linkdef.h
/// \brief Example user linkdef file
#ifdef __MAKECINT__ 

#pragma link off all globals; 
#pragma link off all classes; 
#pragma link off all functions; 
#pragma link C++ nestedclasses; 
 

#pragma link C++ defined_in Analyzer.hxx;

#endif // #ifdef __CINT__
