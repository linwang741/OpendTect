/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		5-11-2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visflatviewer.cc,v 1.18 2009-01-06 20:37:02 cvsyuancheng Exp $";

#include "visflatviewer.h"
#include "arraynd.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "flatview.h"
#include "vistexturechannels.h"
#include "vissplittexture2rectangle.h"
#include "vistexturechannel2rgba.h"


mCreateFactoryEntry( visBase::FlatViewer );

namespace visBase
{
   
FlatViewer::FlatViewer()
    : VisualObjectImpl( false )
    , dataChange( this )
    , channels_( TextureChannels::create() )
    , channel2rgba_( ColTabTextureChannel2RGBA::create() )
    , rectangle_( SplitTexture2Rectangle::create() )
{
    channel2rgba_->ref();
    channel2rgba_->allowShading( true );

    channels_->ref();
    addChild( channels_->getInventorNode() );
    channels_->setChannels2RGBA( channel2rgba_ );

    if ( channels_->nrChannels()<1 )
    {
    	channels_->addChannel();
    	channel2rgba_->setEnabled( 0, true );	
    }

    rectangle_->ref();
    rectangle_->setMaterial( 0 );
    rectangle_->removeSwitch();
    addChild( rectangle_->getInventorNode() );
}


FlatViewer::~FlatViewer()
{
    channels_->unRef();
    channel2rgba_->unRef();
    rectangle_->unRef();
}


void FlatViewer::handleChange( FlatView::Viewer::DataChangeType dt, bool dofill)
{
    switch ( dt )
    {
	case None:	
		break;
	case WVAData:	
	case WVAPars:	
		pErrMsg( "Not supported" );
		break;
	case All:	
	case Annot:	
		pErrMsg( "Not implemented yet" );
		if ( dt!=All )
		    break;
	case VDData:
	    {
		const FlatDataPack* dp = pack( false );
		if ( !dp )
		    channels_->turnOn( false );
		else
		{
		    const Array2D<float>& dparr = dp->data();
		    channels_->setSize( 1, dparr.info().getSize(0),
					   dparr.info().getSize(1) );
		    if ( !dparr.getData() )
		    {
			const int bufsz = dparr.info().getTotalSz();
			mDeclareAndTryAlloc(float*,ptr, float[bufsz]);
			if ( !ptr )
			    channels_->turnOn( false );
			else
			{
			    dparr.getAll( ptr );
			    channels_->setUnMappedData( 0, 0, ptr,
						    TextureChannels::TakeOver );
			}
		    }
		    else 
		    {
			channels_->setUnMappedData( 0, 0, dparr.getData(),
						    TextureChannels::Cache );
		    }

		    appearance().ddpars_.vd_.ctab_ =
			channel2rgba_->getSequence(0)->name();
		    rectangle_->setOriginalTextureSize( 
				dparr.info().getSize(0),
				dparr.info().getSize(1) );
		    channels_->turnOn( appearance().ddpars_.vd_.show_ );

		    dataChange.trigger();
		    if ( dt!=All )
			break;
		}
	    }
	case VDPars : 	
	    {
	    	const FlatView::DataDispPars::VD& vd = appearance().ddpars_.vd_;
	    	ColTab::MapperSetup mappersetup;
		vd.fill( mappersetup );
		if ( channels_->getColTabMapperSetup( 0,0 )!=mappersetup )
		    channels_->setColTabMapperSetup( 0, mappersetup );

		ColTab::Sequence sequence = *channel2rgba_->getSequence( 0 );
		if ( vd.ctab_!=sequence.name() )
		{
		    if ( ColTab::SM().get( vd.ctab_, sequence ) )
		    {
			channel2rgba_->setSequence( 0, sequence );
		    }
		}
	    }
    }			
}


void FlatViewer::setPosition( const Coord3& c00, const Coord3& c01, 
			      const Coord3& c10, const Coord3& c11 )
{
    rectangle_->setPosition( c00, c01, c10, c11 );
}    


void FlatViewer::allowShading( bool yn )
{
    channel2rgba_->allowShading( yn );
}


void FlatViewer::replaceChannels( TextureChannels* nt )
{
    if ( !nt )
	return;

    if ( channels_ )
    {
	removeChild( channels_->getInventorNode() );
	channels_->unRef();
    }

    channels_ = nt;
    channels_->ref();
}


Interval<float> FlatViewer::getDataRange( bool wva ) const
{
    Interval<float> res( mUdf(float),mUdf(float) );
    if ( wva && !wvapack_ || !wva && !vdpack_ )
	return res;
    
    const Array2D<float>& arr = wva ? wvapack_->data() : vdpack_->data();
    for ( int idx=0; idx<arr.info().getSize(0); idx++ )
    {
	for ( int idy=0; idy<arr.info().getSize(1); idy++ )
	{
	    const float val = arr.get(idx,idy);
	    if ( mIsUdf(val) )
		return Interval<float>( mUdf(float),mUdf(float) );
	    
	    if ( res.isUdf() )
		res.start = res.stop = val;
	    else
		res.include( val );
	}
    }
    
    return res;
}


}; // Namespace
