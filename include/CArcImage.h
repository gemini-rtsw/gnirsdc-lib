#ifndef _ARC_CIMAGE_H_
#define _ARC_CIMAGE_H_

#ifdef WIN32
#pragma warning( disable: 4251 )
#endif

#include <string>
#include <memory>
#include <cmath>
#include <unordered_map>
#include "DllMain.h"


namespace arc
{
	namespace image
	{
		#define T_SIZE( t )	pow( ( double )2, ( double )( sizeof( t ) * 8 ) )

		typedef std::shared_ptr<unsigned char>	tSharedU8Ptr;	// 16 & 32-bit data are stored as bytes
		typedef std::shared_ptr<float>			tSharedFltPtr;
		typedef std::shared_ptr<int>			tSharedS32Ptr;


		class DLLARCIMAGE_API CArcImage
		{
			public:
				//  Constructors/destructors
				// +---------------------------------------------------------------------------------+
				CArcImage();
				virtual ~CArcImage();

				void Free( unsigned char* ptr );
				void Free( float* ptr );
				void Free( int* ptr );


				//  Image data
				// +---------------------------------------------------------------------------------+
				unsigned char* GetRow( void* pBuf, int dRow, int dCol1, int dCol2, int dRows, int dCols, int& rdCount, int dBpp = CArcImage::BPP16 );
				unsigned char* GetCol( void* pBuf, int dCol, int dRow1, int dRow2, int dRows, int dCols, int& rdCount, int dBpp = CArcImage::BPP16 );

				float* GetRowArea( void* pBuf, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int& rdCount, int dBpp = CArcImage::BPP16 );
				float* GetColArea( void* pBuf, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int& rdCount, int dBpp = CArcImage::BPP16 );


				//  Statistics
				// +---------------------------------------------------------------------------------+
				class CImgStats
				{
				public:
					CImgStats()
					{
						gMin = gMax = gMean = gVariance = gStdDev = gTotalPixels = gSaturatedPixCnt = 0;
					}

					double gTotalPixels;
					double gMin;
					double gMax;
					double gMean;
					double gVariance;
					double gStdDev;
					double gSaturatedPixCnt;
				};


				class CImgDifStats
				{
				public:
					CImgStats cImg1Stats;
					CImgStats cImg2Stats;
					CImgStats cImgDiffStats;
				};


				CImgDifStats GetDiffStats( void* pBuf1, void* pBuf2, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int dBpp = CArcImage::BPP16 );
				CImgDifStats GetDiffStats( void* pBuf1, void* pBuf2, int dRows, int dCols, int dBpp = CArcImage::BPP16 );

				CImgStats GetStats( void* pBuf, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int dBpp = CArcImage::BPP16 );
				CImgStats GetStats( void* pBuf, int dRows, int dCols, int dBpp = CArcImage::BPP16 );


				//  Histogram
				// +---------------------------------------------------------------------------------+
				int* Histogram( int& rdCount, void* pBuf, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int dBpp = CArcImage::BPP16 );
				int* Histogram( int& rdCount, void* pBuf, int dRows, int dCols, int dBpp = CArcImage::BPP16 );


				//  Image Buffer Manipulation
				// +---------------------------------------------------------------------------------+
				void Add( unsigned int* pU32Buf, unsigned short* pU16Buf1, unsigned short* pU16Buf2, int dRows, int dCols );
				void Subtract( void* pBuf1, void* pBuf2, int dRows, int dCols, int dBpp = CArcImage::BPP16 );
				void SubtractHalves( void* pBuf, int dRows, int dCols, int dBpp = CArcImage::BPP16 );
				void Divide( void* pBuf1, void* pBuf2, int dRows, int dCols, int dBpp = CArcImage::BPP16 );


				//  Image Copy
				// +---------------------------------------------------------------------------------+
				void Copy( void* pDstBuf, int dDstRows, int dDstCols, void* pSrcBuf, int dSrcRows, int dSrcCols, int dBpp = CArcImage::BPP16 );
				void Copy( void* pDstBuf, int dDstSize, void* pSrcBuf, int dSrcSize );


				//  Image Buffer Fill
				// +---------------------------------------------------------------------------------+
				void Fill( void* pBuf, int dRows, int dCols, int dValue, int dBpp = CArcImage::BPP16 );
				void GradientFill( void* pBuf, int dRows, int dCols, int dBpp = CArcImage::BPP16 );
				void SmileyFill( void* pBuf, int dRows, int dCols, int dBpp = CArcImage::BPP16 );
	//			void LogoFill( void* pBuf, int dRows, int dCols, int dBpp = CArcImage::BPP16 );


				//  Synthetic Image
				// +---------------------------------------------------------------------------------+
				void VerifyAsSynthetic( void* pBuf, int dRows, int dCols, int dBpp = CArcImage::BPP16 );


				//  Constants
				// +---------------------------------------------------------------------------------+
				const static int BPP16 = 16;
				const static int BPP32 = 32;

			private:
				void DrawSemiCircle( int xCenter, int yCenter, int radius, double gStartAngle, double gEndAngle, int dCols, void* pBuf, int dColor = 0, int dBpp = CArcImage::BPP16 );
				void DrawFillCircle( int xCenter, int yCenter, int radius, int dCols, void* pBuf, int dColor = 0, int dBpp = CArcImage::BPP16 );
				void DrawGradientFillCircle( int xCenter, int yCenter, int radius, int dRows, int dCols, void* pBuf, int dBpp = CArcImage::BPP16 );
				void DrawCircle( int xCenter, int yCenter, int radius, int dCols, void* pBuf, int dColor = 0, int dBpp = CArcImage::BPP16 );
				void ThrowException( std::string sMethodName, std::string sMsg );

				//  Smart pointer array deleter
				// +--------------------------------------------------------------------+
				template<typename T> static void ArrayDeleter( T* p );

				//  Stores pointers returned to the user
				// +--------------------------------------------------------------------+
				static std::unordered_map<unsigned long,tSharedU8Ptr> m_u8PtrMap;
				static std::unordered_map<unsigned long,tSharedFltPtr> m_fltPtrMap;
				static std::unordered_map<unsigned long,tSharedS32Ptr> m_s32PtrMap;
		};

	}	// end image namespace
}	// end arc namespace

#endif
