#ifndef _CCONIFACE_H_
#define _CCONIFACE_H_

#include "DllMain.h"


namespace arc
{
	namespace device
	{

		class CARCDEVICE_API CConIFace   // Continuous Readout Interface Class
		{
			public:
				virtual ~CConIFace() {}

				virtual void FrameCallback( int   dFramesPerBuffer,		// Frames-per-buffer count
											int   dFrameCount,			// PCI frame count
											int   dRows,				// # of rows in frame
											int   dCols,				// # of cols in frame
											void* pBuffer ) = 0;		// Pointer to frame start in buffer
		};

	}	// end device namespace
}	// end arc namespace


#endif	// _CCONIFACE_H_
