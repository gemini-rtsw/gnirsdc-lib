/*
 *  CArcDeinterlace.h
 *  CArcDeinterlace
 *
 *  Created by Scott Streit on 2/9/11.
 *  Copyright 2011 Astronomical Research Cameras, Inc. All rights reserved.
 *
 */
#ifndef _ARC_CDEINTERLACE_H_
#define _ARC_CDEINTERLACE_H_

#ifdef WIN32
#pragma warning( disable: 4251 )
#endif

#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include "DllMain.h"

#ifdef WIN32
	#include <windows.h>
#endif


namespace arc
{
	namespace deinterlace
	{
		// +------------------------------------------------------------+
		// | Define OS dependent custom library handle                  |
		// +------------------------------------------------------------+
		#ifdef WIN32
			#define ArcCustomLib	HINSTANCE
		#else
			#define ArcCustomLib	void*
		#endif


		// +------------------------------------------------------------+
		// | Define custom function pointers                            |
		// +------------------------------------------------------------+
		typedef int  ( *RunCustomAlgFUNC )( void *, int, int, int, int );
		typedef void ( *GetCustomNameFUNC )( int, char*, int );
		typedef void ( *GetErrorMsgFUNC )( char*, int );
		typedef int  ( *GetCustomCountFUNC )( void );


		// +------------------------------------------------------------+
		// | CArcDeinterlace class definition                           |
		// +------------------------------------------------------------+
		class CDEINTERLACE_API CArcDeinterlace
		{
			public:
				CArcDeinterlace();
				virtual ~CArcDeinterlace();

				//  Deinterlace the data with the specified algorithm
				// +--------------------------------------------------------------------+
				void RunAlg( void *pData, int dRows, int dCols, int dAlgorithm, int dArg = -1 );


				//  Custom dAlgorithm interface
				// +--------------------------------------------------------------------+
				bool FindCustomLibrary( const std::string sLibPath );
				void RunCustomAlg( void *pData, int dRows, int dCols, int dAlgorithm, int dArg = -1 );
				void GetCustomInfo( int index, int& dAlgorithm, std::string& name );
				int  GetCustomCount();


				//  Algorithm definitions
				// +--------------------------------------------------------------------+
				const static int DEINTERLACE_NONE        = 0;
				const static int DEINTERLACE_PARALLEL    = 1;
				const static int DEINTERLACE_SERIAL      = 2;
				const static int DEINTERLACE_CCD_QUAD    = 3;
				const static int DEINTERLACE_IR_QUAD     = 4;
				const static int DEINTERLACE_CDS_IR_QUAD = 5;
				const static int DEINTERLACE_HAWAII_RG   = 6;
				const static int DEINTERLACE_STA1600	 = 7;
				const static int DEINTERLACE_CUSTOM      = 8;

			private:
				std::shared_ptr<unsigned short> m_pNewData;

				ArcCustomLib					m_hCustomLib;
				RunCustomAlgFUNC				m_fnRunCustomAlg;
				GetCustomNameFUNC				m_fnGetCustomName;
				GetCustomCountFUNC				m_fnGetCustomCount;
				GetErrorMsgFUNC					m_fnGetErrorMsg;
				int								m_dCustomCount;

				//  Smart pointer deleter
				// +-----------------------------------------------------------------+
				template<typename T> static void ArrayDeleter( T* p );

				void Parallel( unsigned short *pData, int dRows, int dCols );
				void Serial( unsigned short *pData, int dRows, int dCols );
				void CCDQuad( unsigned short *pData, int dRows, int dCols );
				void IRQuad( unsigned short *pData, int dRows, int dCols );
				void IRQuadCDS( unsigned short *pData, int dRows, int dCols );
				void HawaiiRG( unsigned short *pData, int dRows, int dCols, int dArg );
				void STA1600( unsigned short *pData, int dRows, int dCols );
				void ThrowException( std::string sMethodName, std::string sMsg );

				void GetDirList( const std::string& sPath, std::vector<std::string>& vDirs );
				void LoadCustomLibrary( const std::string sLibPath, const std::string sLibName );
				bool IsCustomLibrary( ArcCustomLib hCustomLib );
				std::string GetCustomErrorMsg();
		};

	}	// end deinterlace namespace
}	// end arc namespace


#endif
