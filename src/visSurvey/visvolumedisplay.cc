/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          August 2002
 RCS:           $Id: visvolumedisplay.cc,v 1.24 2003-01-28 08:27:19 nanne Exp $
________________________________________________________________________

-*/


#include "visvolumedisplay.h"

#include "viscubeview.h"
#include "vistexturerect.h"
#include "vistexture3viewer.h"
#include "visrectangle.h"
#include "cubesampling.h"
#include "attribsel.h"
#include "attribslice.h"
#include "arrayndimpl.h"
#include "survinfo.h"
#include "visselman.h"
#include "visdataman.h"
#include "sorting.h"
#include "iopar.h"
#include "vismaterial.h"

mCreateFactoryEntry( visSurvey::VolumeDisplay );

const char* visSurvey::VolumeDisplay::volumestr = "Cube ID";

visSurvey::VolumeDisplay::VolumeDisplay()
    : VisualObject(true)
    , cube(visBase::CubeView::create())
    , selected_(false)
    , as(*new AttribSelSpec)
    , cache(0)
    , moved(this)
    , manipulated(false)
    , slicemoving(this)
{
    selection()->notify( mCB(this,VolumeDisplay,select));
    deSelection()->notify( mCB(this,VolumeDisplay,deSelect));

    cube->ref();
    cube->getMaterial()->setAmbience( 0.8 );

    cube->getBoxManipEnd()->notify(mCB(this,VolumeDisplay,manipMotionFinishCB));

    CubeSampling cs;
    cs.hrg.start.inl = (5*SI().range().start.inl+3*SI().range().stop.inl)/8;
    cs.hrg.start.crl = (5*SI().range().start.crl+3*SI().range().stop.crl)/8;
    cs.hrg.stop.inl = (3*SI().range().start.inl+5*SI().range().stop.inl)/8;
    cs.hrg.stop.crl = (3*SI().range().start.crl+5*SI().range().stop.crl)/8;
    cs.zrg.start = ( 5*SI().zRange().start + 3*SI().zRange().stop ) / 8;
    cs.zrg.stop = ( 3*SI().zRange().start + 5*SI().zRange().stop ) / 8;
    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
    setCubeSampling( cs );

    inlid = cube->addSlice( 0 );
    initSlice( inlid );
    crlid = cube->addSlice( 1 );
    initSlice( crlid );
    tslid = cube->addSlice( 2 );
    initSlice( tslid );

    visBase::DM().getObj( inlid )->setName("inline");
    visBase::DM().getObj( crlid )->setName("crossline");
    visBase::DM().getObj( tslid )->setName("timeslice");

    const int volrenid = cube->getVolRenId();
    visBase::DM().getObj( volrenid )->setName("Volren");


    setColorTab( getColorTab() );
}


visSurvey::VolumeDisplay::~VolumeDisplay()
{
    selection()->remove( mCB(this,VolumeDisplay,select));
    deSelection()->remove( mCB(this,VolumeDisplay,deSelect));

    cube->getBoxManipEnd()->remove(mCB(this,VolumeDisplay,manipMotionFinishCB));
    cube->unRef();

    delete &as;
}


void visSurvey::VolumeDisplay::setCenter( const Coord3& pos )
{ cube->setCenter( pos ); }


Coord3 visSurvey::VolumeDisplay::center() const
{ return cube->center(); }


Coord3 visSurvey::VolumeDisplay::manipCenter() const
{ return cube->draggerCenter(); }


void visSurvey::VolumeDisplay::setWidth( const Coord3& pos )
{ cube->setWidth( pos ); }


Coord3 visSurvey::VolumeDisplay::width() const
{ return cube->width(); }


Coord3 visSurvey::VolumeDisplay::manipWidth() const
{ return cube->draggerWidth(); }


void visSurvey::VolumeDisplay::showBox( bool yn )
{ cube->showBox( yn ); }


void visSurvey::VolumeDisplay::resetManip()
{
    cube->resetDragger();
}


void visSurvey::VolumeDisplay::getPlaneIds( int& id0, int& id1, int& id2 )
{
    id0 = inlid;
    id1 = crlid;
    id2 = tslid;
}


float visSurvey::VolumeDisplay::getPlanePos( int id__ )
{
    return cube->slicePosition( id__ );
}


bool visSurvey::VolumeDisplay::updateAtNewPos()
{
    succeeded_ = false;
    moved.trigger();
    manipulated = false;
    return succeeded_;
}


AttribSelSpec& visSurvey::VolumeDisplay::getAttribSelSpec()
{ return as; }


const AttribSelSpec& visSurvey::VolumeDisplay::getAttribSelSpec() const
{ return as; }


void visSurvey::VolumeDisplay::setAttribSelSpec( const AttribSelSpec& as_ )
{
    if ( as==as_ ) return;
    as = as_;
    delete cache;
    cache = 0;
    cube->useTexture( false );
}


CubeSampling& visSurvey::VolumeDisplay::getCubeSampling(bool manippos)
{
    Coord3 center_ = manippos ? cube->draggerCenter() : cube->center();
    Coord3 width_ = manippos ? cube->draggerWidth() : cube->width();

    CubeSampling* cs = new CubeSampling;
    cs->hrg.start = BinID( mNINT( center_.x - width_.x / 2 ),
		  	  mNINT( center_.y - width_.y / 2 ) );

    cs->hrg.stop = BinID( mNINT( center_.x + width_.x / 2 ),
		  	 mNINT( center_.y + width_.y / 2 ) );

    cs->hrg.step = BinID( SI().inlStep(), SI().crlStep() );

    cs->zrg.start = center_.z - width_.z / 2;
    cs->zrg.stop = center_.z + width_.z / 2;

    return *cs;
}


void visSurvey::VolumeDisplay::setCubeSampling( const CubeSampling& cs_ )
{
    Coord3 newwidth( cs_.hrg.stop.inl - cs_.hrg.start.inl,
			 cs_.hrg.stop.crl - cs_.hrg.start.crl, 
			 cs_.zrg.stop - cs_.zrg.start );
    setWidth( newwidth );
    Coord3 newcenter( (cs_.hrg.stop.inl + cs_.hrg.start.inl) / 2,
			  (cs_.hrg.stop.crl + cs_.hrg.start.crl) / 2,
			  (cs_.zrg.stop + cs_.zrg.start) / 2 );
    setCenter( newcenter );
    resetManip();
}


bool visSurvey::VolumeDisplay::putNewData( AttribSliceSet* sliceset )
{
    if ( !sliceset->size() )
    {
	delete sliceset;
	return false;
    }

    if ( sliceset->direction==AttribSlice::Hor )
    {
	const int inlsz = (*sliceset)[0]->info().getSize(0);
	const int crlsz = (*sliceset)[0]->info().getSize(1);
	const int zsz = sliceset->size();
	Array3DImpl<float> datacube( zsz, inlsz, crlsz );

	for ( int zidx=0; zidx<zsz; zidx++ )
	{
	    for ( int inl=0; inl<inlsz; inl++ )
	    {
		for ( int crl=0; crl<crlsz; crl++ )
		{
		    const float val = (*sliceset)[zidx]->get( inl, crl );
		    datacube.set( zidx, inl, crl, val );
		}
	    }
	}

	cube->setData( &datacube );
    }
    else if ( sliceset->direction==AttribSlice::Crl )
    {
	const int inlsz = (*sliceset)[0]->info().getSize(0);
	const int crlsz = sliceset->size();
	const int zsz = (*sliceset)[0]->info().getSize(1);
	Array3DImpl<float> datacube( zsz, inlsz, crlsz );

	for ( int zidx=0; zidx<zsz; zidx++ )
	{
	    for ( int inl=0; inl<inlsz; inl++ )
	    {
		for ( int crl=0; crl<crlsz; crl++ )
		{
		    const float val = (*sliceset)[crl]->get( inl, zidx );
		    datacube.set( zidx, inl, crl, val );
		}
	    }
	}

	cube->setData( &datacube );
    }
    else if ( sliceset->direction==AttribSlice::Inl )
    {
	const int inlsz = sliceset->size();
	const int crlsz = (*sliceset)[0]->info().getSize(0);
	const int zsz = (*sliceset)[0]->info().getSize(1);

	Array3DImpl<float> datacube( zsz, inlsz, crlsz );
	for ( int zidx=0; zidx<zsz; zidx++ )
	{
	    for ( int inl=0; inl<inlsz; inl++ )
	    {
		for ( int crl=0; crl<crlsz; crl++ )
		{
		    const float val = (*sliceset)[inl]->get( crl, zidx );
		    datacube.set( zidx, inl, crl, val );
		}
	    }
	}

	cube->setData( &datacube );
    }

    delete cache;
    cache = sliceset;

    cube->useTexture( true );
    return true;
}


const AttribSliceSet* visSurvey::VolumeDisplay::getPrevData() const
{
    return cache;
}


void visSurvey::VolumeDisplay::turnOn( bool yn ) 
{ cube->turnOn( yn ); }


bool visSurvey::VolumeDisplay::isOn() const 
{ return cube->isOn(); }


void visSurvey::VolumeDisplay::select()
{
    if ( selected_ ) return;
    selected_ = true;
}


void visSurvey::VolumeDisplay::deSelect()
{
    if ( !selected_ ) return;
    selected_ = false;

    
    if ( manipulated )
	updateAtNewPos();
}


void visSurvey::VolumeDisplay::manipMotionFinishCB( CallBacker* )
{
    CubeSampling cs = getCubeSampling(true);
    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );

    const Interval<int> inlrange( SI().range(true).start.inl,
				  SI().range(true).stop.inl );
    const Interval<int> crlrange( SI().range(true).start.crl,
				  SI().range(true).stop.crl );

    if ( !inlrange.includes( cs.hrg.start.inl ) ||
	    !inlrange.includes( cs.hrg.stop.inl ) ||
	    !crlrange.includes( cs.hrg.start.crl ) ||
	    !crlrange.includes( cs.hrg.stop.crl ) ||
	    !SI().zRange(true).includes( cs.zrg.start ) ||
	    !SI().zRange(true).includes( cs.zrg.stop ) )
    {
	resetManip();
	return;
    }

    const Coord3 newwidth( cs.hrg.stop.inl - cs.hrg.start.inl,
			 cs.hrg.stop.crl - cs.hrg.start.crl, 
			 cs.zrg.stop - cs.zrg.start );
    cube->setDraggerWidth( newwidth );
    const Coord3 newcenter( (cs.hrg.stop.inl + cs.hrg.start.inl) / 2,
			  (cs.hrg.stop.crl + cs.hrg.start.crl) / 2,
			  (cs.zrg.stop + cs.zrg.start) / 2 );
    cube->setDraggerCenter( newcenter );
}


void visSurvey::VolumeDisplay::initSlice( int sliceid )
{
    DataObject* dobj = visBase::DM().getObj( sliceid );
    mDynamicCastGet(visBase::MovableTextureSlice*,ts,dobj);
    if ( ts )
    {
	ts->setMaterial( cube->getMaterial() );
	ts->motion.notify( mCB(this,VolumeDisplay,sliceMoving) );
    }
}


void visSurvey::VolumeDisplay::sliceMoving( CallBacker* )
{
    slicemoving.trigger();
}


void visSurvey::VolumeDisplay::setColorTab( visBase::VisColorTab& ctab )
{
    cube->setVolRenColorTab( ctab );
    cube->setViewerColorTab( ctab );
}


visBase::VisColorTab& visSurvey::VolumeDisplay::getColorTab()
{ return cube->getViewerColorTab(); }


void visSurvey::VolumeDisplay::setMaterial( visBase::Material* nm)
{ cube->setMaterial(nm); }


visBase::Material* visSurvey::VolumeDisplay::getMaterial()
{ return cube->getMaterial(); }


const visBase::Material* visSurvey::VolumeDisplay::getMaterial() const
{ return cube->getMaterial(); }


int visSurvey::VolumeDisplay::getVolRenId() const
{ return cube->getVolRenId(); }


SoNode* visSurvey::VolumeDisplay::getData() 
{ return cube->getData(); }


bool visSurvey::VolumeDisplay::isVolRenShown() const
{
    return cube->isVolRenShown();
}


void visSurvey::VolumeDisplay::fillPar( IOPar& par, TypeSet<int>& saveids) const
{
    visBase::VisualObject::fillPar( par, saveids );
    int volid = cube->id();
    par.set( volumestr, volid );

    if ( saveids.indexOf( volid )==-1 ) saveids += volid;

    as.fillPar(par);
}


int visSurvey::VolumeDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObject::usePar( par );
    if ( res!=1 ) return res;

    int volid;
    if ( !par.get( volumestr, volid ))
	return -1;

    visBase::DataObject* dataobj = visBase::DM().getObj( volid );
    if ( !dataobj ) return 0;

    mDynamicCastGet(visBase::CubeView*,cv,dataobj);
    if ( !cv ) return -1;

    cube->getBoxManipEnd()->remove(mCB(this,VolumeDisplay,manipMotionFinishCB));
    cube->unRef();
    cube = cv;
    cube->ref();
    cube->getBoxManipEnd()->notify(mCB(this,VolumeDisplay,manipMotionFinishCB));

    inlid = cube->addSlice( 0 );
    visBase::DM().getObj( inlid )->setName("inline");
    crlid = cube->addSlice( 1 );
    visBase::DM().getObj( crlid )->setName("crossline");
    tslid = cube->addSlice( 2 );
    visBase::DM().getObj( tslid )->setName("timeslice");

    const int volrenid = cube->getVolRenId();
    visBase::DM().getObj( volrenid )->setName("Volren");

    if ( !as.usePar( par )) return -1;

    return 1;
}
