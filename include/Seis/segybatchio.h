#ifndef segybatchio_h
#define segybatchio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		18-10-1995
 Contents:	Selectors
 RCS:		$Id: segybatchio.h,v 1.3 2011-03-16 12:10:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"


/*!\brief

Keys that should be used with od_process_segyio.cc

*/

namespace SEGY
{

namespace IO
{
    mGlobal const char* sKeyTask()	{ return "Task"; }
    mGlobal const char* sKeyIndexPS()	{ return "Index Pre-Stack"; }
    mGlobal const char* sKeyIndex3DVol() { return "Index 3D Volume"; }
    mGlobal const char* sKeyIs2D()	{ return "Is 2D"; }

}; //namespace IO

}; //namespce SEGY


#endif
