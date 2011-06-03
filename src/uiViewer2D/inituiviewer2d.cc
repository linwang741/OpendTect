/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bruno
 * DATE     : June 2011
-*/

static const char* rcsID = "$Id: inituiviewer2d.cc,v 1.1 2011-06-03 14:10:26 cvsbruno Exp $";


#include "inituiviewer2d.h"

#include "visvw2dfault.h"
#include "visvw2dfaultss2d.h"
#include "visvw2dfaultss3d.h"
#include "visvw2dhorizon3d.h"
#include "visvw2dhorizon2d.h"
#include "visvw2dpickset.h"
#include "visvw2dseismic.h"

void uiViewer2D::initStdClasses()
{
    VW2DFaultSS2D::initClass();
    VW2DFaultSS3D::initClass();
    VW2DFault::initClass();
    Vw2DHorizon2D::initClass();
    Vw2DHorizon3D::initClass();
    VW2DPickSet::initClass();
}
