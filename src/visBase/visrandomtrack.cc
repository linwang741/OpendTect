/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: visrandomtrack.cc,v 1.5 2003-02-12 12:53:04 kristofer Exp $";

#include "visrandomtrack.h"

#include "errh.h"
#include "SoRandomTrackLineDragger.h"
#include "vistexture2.h"
#include "vistristripset.h"
#include "viscoord.h"
#include "vistexturecoords.h"

#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoScale.h"
#include "Inventor/nodes/SoMaterial.h"


mCreateFactoryEntry( visBase::RandomTrack );

visBase::RandomTrack::RandomTrack()
    : dragger( 0 )
    , draggerswitch( 0 )
    , depthrg( 0, 1 )
    , knotmovement( this )
{
    addKnot( Coord( 0, 0 ) );
    addKnot( Coord( 1, 0 ) );
}


visBase::RandomTrack::~RandomTrack()
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	removeChild( sections[idx]->getData() );
	sections[idx]->unRef();
    }
}


void visBase::RandomTrack::showDragger( bool yn )
{
    if ( yn )
    {
	if ( !draggerswitch )
	{
	    createDragger();
	}

	draggerswitch->whichChild = 0;
	if ( dragger->knots.getNum()>knots.size() )
	    dragger->knots.deleteValues(knots.size(),dragger->knots.getNum()-1);

	for ( int idx=0; idx<knots.size(); idx++ )
	{
	    dragger->knots.set1Value( idx, knots[idx].x, knots[idx].y );
	}

	dragger->z0 = depthrg.start;
	dragger->z1 = depthrg.stop;

    }
    else if ( !yn && draggerswitch )
    {
	draggerswitch->whichChild = SO_SWITCH_NONE;
    }
    
}


bool visBase::RandomTrack::isDraggerShown() const
{
    return draggerswitch &&draggerswitch->whichChild.getValue()==SO_SWITCH_NONE;
}


int visBase::RandomTrack::nrKnots() const
{ return knots.size(); }


void visBase::RandomTrack::addKnot( const Coord& np )
{
    knots += np;
    rebuild();
}


void visBase::RandomTrack::insertKnot( int pos, const Coord& np )
{
    knots.insert( pos, np );
    rebuild();
}


Coord visBase::RandomTrack::getKnotPos( int pos ) const
{ return knots[pos]; }


Coord visBase::RandomTrack::getDraggerKnotPos( int pos ) const
{
    if ( !dragger ) return getKnotPos( pos );
    SbVec2f draggerpos = dragger->knots[pos];
    return Coord( draggerpos[0], draggerpos[1] );
}


void visBase::RandomTrack::setKnotPos( int pos, const Coord& np )
{
    knots[pos] = np;
    rebuild();
}


void visBase::RandomTrack::removeKnot( int pos )
{
    if ( knots.size()< 3 )
    {
	pErrMsg("Can't remove knot");
	return;
    }

    knots.remove( pos );
    rebuild();
}


void visBase::RandomTrack::setDepthInterval( const Interval<float>& drg )
{
    depthrg = drg;
    rebuild();
}


const Interval<float> visBase::RandomTrack::getDepthInterval() const
{ return depthrg; }


const Interval<float>
visBase::RandomTrack::getDraggerDepthInterval() const
{
    if ( !dragger ) return getDepthInterval();
    return Interval<float>( dragger->z0.getValue(), dragger->z1.getValue() );
}


#define mSetRange( dim )  \
    createDragger(); \
    SbVec3f xyzstart = dragger->xyzStart.getValue(); \
    SbVec3f xyzstop = dragger->xyzStop.getValue(); \
    SbVec3f xyzstep = dragger->xyzStep.getValue(); \
 \
    xyzstart[dim] = rg.start; \
    xyzstop[dim] = rg.stop; \
    xyzstep[dim] = rg.step; \
 \
    dragger->xyzStart.setValue( xyzstart ); \
    dragger->xyzStop.setValue( xyzstop ); \
    dragger->xyzStep.setValue( xyzstep )

void visBase::RandomTrack::setXrange( const StepInterval<float>& rg )
{
    mSetRange( 0 );
}

void visBase::RandomTrack::setYrange( const StepInterval<float>& rg )
{
    mSetRange( 1 );
}

void visBase::RandomTrack::setZrange( const StepInterval<float>& rg )
{
    mSetRange( 2 );
}


void visBase::RandomTrack::setDraggerSize( const Coord3& nz )
{
    createDragger();
    SoScale* size =
	dynamic_cast<SoScale*>(dragger->getPart("subDraggerScale", true ));
    size->scaleFactor.setValue( nz.x, nz.y, nz.z );
}


void visBase::RandomTrack::setClipRate( float nc )
{
    const int nrsections = sections.size();
    for ( int idx=0; idx<nrsections; idx++ )
    {
	Texture2* texture = sections[idx]->getTexture2();
	texture->setClipRate( nc );
    }
}


float visBase::RandomTrack::clipRate() const
{ return sections[0]->getTexture2()->clipRate(); }


void visBase::RandomTrack::setAutoScale( bool nc )
{
    const int nrsections = sections.size();
    for ( int idx=0; idx<nrsections; idx++ )
    {
	Texture2* texture = sections[idx]->getTexture2();
	texture->setAutoScale( nc );
    }
}


bool visBase::RandomTrack::autoScale() const
{ return sections[0]->getTexture2()->autoScale(); }


void visBase::RandomTrack::setColorTab( VisColorTab& nc )
{
    const int nrsections = sections.size();
    for ( int idx=0; idx<nrsections; idx++ )
    {
	Texture2* texture = sections[idx]->getTexture2();
	texture->setColorTab( nc );
    }
}


visBase::VisColorTab& visBase::RandomTrack::getColorTab()
{ return sections[0]->getTexture2()->getColorTab(); }


void visBase::RandomTrack::setData( int section, const Array2D<float>& data )
{
    Texture2* texture = sections[section]->getTexture2();
    texture->setData( &data );
}


void visBase::RandomTrack::rebuild()
{
    while ( sections.size()<knots.size()-1 )
    {
	TriangleStripSet* strip = TriangleStripSet::create();
	strip->setTexture2( Texture2::create() );
	strip->ref();
	if ( sections.size() )
	    strip->setCoordinates( sections[0]->getCoordinates() );

	TextureCoords* texturecoords = TextureCoords::create();
	strip->setTextureCoords( texturecoords );
	texturecoords->addCoord( Coord( 0, 0 ) );
	texturecoords->addCoord( Coord( 0, 1 ) );
	texturecoords->addCoord( Coord( 1, 0 ) );
	texturecoords->addCoord( Coord( 1, 1 ) );

	strip->setTextureCoordIndex( 0, 0 );
	strip->setTextureCoordIndex( 1, 1 );
	strip->setTextureCoordIndex( 2, 2 );
	strip->setTextureCoordIndex( 3, 3 );

	sections += strip;
	addChild( strip->getData() );
    }

    while ( sections.size()>knots.size()-1 )
    {
	const int idx = sections.size()-1;
	TriangleStripSet* strip = sections[idx];
	removeChild( strip->getData() );
	strip->unRef();
	sections.remove( idx );
    }

    if ( !sections.size() ) return;

    Coordinates* coords = sections[0]->getCoordinates();

    for ( int idx=0; idx<knots.size(); idx++ )
    {
	coords->setPos( idx*2,
		Coord3( knots[idx].x, knots[idx].y, depthrg.start ));
	coords->setPos( idx*2+1,
		Coord3( knots[idx].x, knots[idx].y, depthrg.stop ));

	if ( !idx ) continue;

	sections[idx-1]->setCoordIndex( 0, (idx-1)*2 );
	sections[idx-1]->setCoordIndex( 1, (idx-1)*2+1 );
	sections[idx-1]->setCoordIndex( 2, idx*2 );
	sections[idx-1]->setCoordIndex( 3, idx*2+1 );
	sections[idx-1]->setCoordIndex( 4, -1 );
    }
}


void visBase::RandomTrack::createDragger()
{
    if ( draggerswitch ) return;

    dragger = new SoRandomTrackLineDragger;
    draggerswitch = new SoSwitch;
    insertChild( 0, draggerswitch );
    draggerswitch->addChild( dragger );

    SoMaterial* material = new SoMaterial;

    material->transparency.setValue( 0.9 );
    dragger->setPart( "feedbackMaterial", material );
}


void visBase::RandomTrack::motionCB(void* data,
				    SoRandomTrackLineDragger* dragger)
{
    visBase::RandomTrack* myself = (visBase::RandomTrack*) data;
    CBCapsule<int> caps( dragger->getMovingKnot(), myself );
    myself->knotmovement.trigger( &caps );
}
