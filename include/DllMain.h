#ifndef _CARCDEVICE_DLLMAIN_H_
#define _CARCDEVICE_DLLMAIN_H_


// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CARCDEVICE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CARCDEVICE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WIN32
	#ifdef CARCDEVICE_EXPORTS
		#define CARCDEVICE_API __declspec( dllexport )
		#define CARCDEVICE_STL_EXPORT
	#else
		#define CARCDEVICE_API __declspec( dllimport )
		#define CARCDEVICE_STL_EXPORT extern
	#endif
#else
	#define CARCDEVICE_API __attribute__((visibility("default")))
#endif


#endif		// _CARCDEVICE_DLLMAIN_H_
