/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
 RCS:		$Id: visvw2dpickset.cc,v 1.7 2011-06-03 14:10:26 cvsbruno Exp $
________________________________________________________________________

-*/

#include "visvw2dpickset.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "cubesampling.h"
#include "flatposdata.h"
#include "indexinfo.h"
#include "ioobj.h"
#include "ioman.h"
#include "pickset.h"
#include "picksettr.h"
#include "separstr.h"
#include "survinfo.h"
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"
#include "uiworld2ui.h"


mCreateVw2DFactoryEntry( VW2DPickSet );

VW2DPickSet::VW2DPickSet( const EM::ObjectID& picksetidx, uiFlatViewWin* win,
			  const ObjectSet<uiFlatViewAuxDataEditor>& editors )
    : Vw2DDataObject()
    , pickset_(0) 					  
    , picks_(new FlatView::Annotation::AuxData( "Picks" ))
    , editor_(const_cast<uiFlatViewAuxDataEditor*>(editors[0]))
    , viewer_(editor_->getFlatViewer())
    , deselected_(this)
    , isownremove_(false)
{
    if ( picksetidx > 0 && Pick::Mgr().size() < picksetidx )
	pickset_ = &Pick::Mgr().get( picksetidx );

    viewer_.appearance().annot_.auxdata_ += picks_;
    viewer_.appearance().annot_.editable_ = false; 
    viewer_.dataChanged.notify( mCB(this,VW2DPickSet,dataChangedCB) );
    viewer_.viewChanged.notify( mCB(this,VW2DPickSet,dataChangedCB) );

    auxid_ = editor_->addAuxData( picks_, true );
    editor_->enableEdit( auxid_, true, true, true );
    editor_->enablePolySel( auxid_, true );
    editor_->removeSelected.notify( mCB(this,VW2DPickSet,pickRemoveCB) );
    editor_->movementFinished.notify( mCB(this,VW2DPickSet,pickAddChgCB) );
    
    Pick::Mgr().setChanged.notify( mCB(this,VW2DPickSet,dataChangedCB) );
    Pick::Mgr().locationChanged.notify( mCB(this,VW2DPickSet,dataChangedCB) );
}


VW2DPickSet::~VW2DPickSet()
{
    viewer_.appearance().annot_.auxdata_ -= picks_;
    editor_->removeAuxData( auxid_ );
    delete picks_;

    viewer_.dataChanged.remove( mCB(this,VW2DPickSet,dataChangedCB) );
    viewer_.viewChanged.remove( mCB(this,VW2DPickSet,dataChangedCB) );
    editor_->removeSelected.remove( mCB(this,VW2DPickSet,pickRemoveCB) );
    editor_->movementFinished.remove( mCB(this,VW2DPickSet,pickAddChgCB) );
    Pick::Mgr().setChanged.remove( mCB(this,VW2DPickSet,dataChangedCB) );
    Pick::Mgr().locationChanged.remove( mCB(this,VW2DPickSet,dataChangedCB) );
}


void VW2DPickSet::pickAddChgCB( CallBacker* cb )
{
    if ( !isselected_ || editor_->getSelPtIdx().size() 
	 || editor_->isSelActive() )
	return;

    FlatView::Point newpt = editor_->getSelPtPos();
    const Coord3 crd = getCoord( newpt );
    if ( !crd.isDefined() ) 
	return;
    // Add
    if ( !pickset_ ) 
	return;
    (*pickset_) += Pick::Location( crd );
    const int locidx = pickset_->size()-1;
    Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::Added,
				 pickset_, locidx );
    Pick::Mgr().reportChange( 0, cd );
}


void VW2DPickSet::pickRemoveCB( CallBacker* cb )
{
    if ( !pickset_ ) return;
    const TypeSet<int>&	selpts = editor_->getSelPtIdx();
    const int selsize = selpts.size();
    isownremove_ = selsize == 1;
    for ( int idx=0; idx<selsize; idx++ )
    {
	const int locidx = selpts[idx];
	if ( !picks_->poly_.validIdx(locidx) )
	    continue;
        
	const int pickidx = picksetidxs_[locidx];
	picksetidxs_.remove( locidx );
	Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::ToBeRemoved,
				 pickset_, pickidx );
	pickset_->remove( pickidx );
	Pick::Mgr().reportChange( 0, cd );
    }
    
    isownremove_ = false;
}


MarkerStyle2D VW2DPickSet::get2DMarkers( const Pick::Set& ps ) const
{
    MarkerStyle2D style( MarkerStyle2D::Square, ps.disp_.pixsize_, 
			 ps.disp_.color_ );
    switch( ps.disp_.markertype_ )
    {
	case MarkerStyle3D::Plane:
	    style.type_ = MarkerStyle2D::Plane;
	    break;
	case MarkerStyle3D::Sphere:
	    style.type_ = MarkerStyle2D::Circle;
	    break;
	case MarkerStyle3D::Cube:
	    style.type_ = MarkerStyle2D::Square;
	    break;
	default:
    	    style.type_ = MarkerStyle2D::Circle;
    }

    return style;
}


Coord3 VW2DPickSet::getCoord( const FlatView::Point& pt ) const
{
    const FlatDataPack* fdp = viewer_.pack( true );
    if ( !fdp )	fdp = viewer_.pack( false );

    mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp);
    if ( dp3d )
    {
	const CubeSampling cs = dp3d->cube().cubeSampling();
	BinID bid; float z;
	if ( dp3d->dataDir() == CubeSampling::Inl )
	{
	    bid = BinID( cs.hrg.start.inl, (int)pt.x );
	    z = pt.y;
	}
	else if ( dp3d->dataDir() == CubeSampling::Crl )
	{
	    bid = BinID( (int)pt.x, cs.hrg.start.crl );
	    z = pt.y;
	}
	else
	{
	    bid = BinID( (int)pt.x, (int)pt.y );
	    z = cs.zrg.start;
	}

	return ( cs.hrg.includes(bid) && cs.zrg.includes(z) ) ? 
	    Coord3( SI().transform(bid), z ) : Coord3::udf();
    }

    return Coord3::udf();
}


void VW2DPickSet::updateSetIdx( const CubeSampling& cs )
{
    if ( !pickset_ ) return;
    picksetidxs_.erase();
    for ( int idx=0; idx<pickset_->size(); idx++ )
    {
	const Coord3& pos = (*pickset_)[idx].pos;
	const BinID bid = SI().transform(pos);
	if ( cs.hrg.includes(bid) )
	    picksetidxs_ += idx;
    }
}


void VW2DPickSet::drawAll()
{
    const FlatDataPack* fdp = viewer_.pack( true );
    if ( !fdp )	fdp = viewer_.pack( false );

    mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp);
    const bool oninl = dp3d->dataDir() == CubeSampling::Inl;
    const CubeSampling& cs = dp3d->cube().cubeSampling();

    updateSetIdx( cs );

    if ( isownremove_ ) return;

    const uiWorldRect& curvw = viewer_.curView();
    const float zdiff = curvw.height();
    const float nrzpixels = viewer_.rgbCanvas().arrArea().vNrPics();
    const float zfac = nrzpixels / zdiff;
    const float xdiff = curvw.width() *
	( oninl ? SI().crlDistance() : SI().inlDistance() );
    const float nrxpixels = viewer_.rgbCanvas().arrArea().hNrPics();
    const float xfac = nrxpixels / xdiff;

    picks_->poly_.erase();
    picks_->markerstyles_.erase();
    if ( !pickset_ ) return;
    MarkerStyle2D markerstyle = get2DMarkers( *pickset_ );
    const int nrpicks = picksetidxs_.size();
    for ( int idx=0; idx<nrpicks; idx++ )
    {
	const int pickidx = picksetidxs_[idx];
	const Coord3& pos = (*pickset_)[pickidx].pos;
	const BinID bid = SI().transform(pos);
	FlatView::Point point( oninl ? bid.crl : bid.inl, pos.z );
	picks_->poly_ += point;

	BufferString dipval;
	(*pickset_)[pickidx].getText( "Dip" , dipval );
	SeparString dipstr( dipval );
	const float dip = oninl ? dipstr.getFValue( 1 ) : dipstr.getFValue( 0 );
	const float depth = (dip/1000000) * zfac;
	markerstyle.rotation_ =
	    mIsUdf(dip) ? 0 : ( atan2(2*depth,xfac) * (180/M_PI) );
	picks_->markerstyles_ += markerstyle;
    }
    
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void VW2DPickSet::clearPicks()
{
    if ( !pickset_ ) return;
    pickset_->erase();
    drawAll();
}


void VW2DPickSet::enablePainting( bool yn )
{
    picks_->enabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void VW2DPickSet::dataChangedCB( CallBacker* )
{
    drawAll();
}


void VW2DPickSet::selected()
{
   isselected_ = true;
}


void VW2DPickSet::triggerDeSel()
{
    isselected_ = false;
    deselected_.trigger();
}


void VW2DPickSet::fillPar( IOPar& iop ) const
{
    Vw2DDataObject::fillPar( iop );
    iop.set( sKeyMID(), pickSetID() ); 
}


void VW2DPickSet::usePar( const IOPar& iop )
{
    Vw2DDataObject::usePar( iop );
    MultiID mid;
    iop.get( sKeyMID(), mid );

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( Pick::Mgr().indexOf(ioobj->key()) >= 0 )
	return;
    Pick::Set* newps = new Pick::Set; BufferString bs;
    if ( PickSetTranslator::retrieve(*newps,ioobj,true, bs) )
    {
	Pick::Mgr().set( ioobj->key(), newps );
	pickset_ = newps;
    }
}


const MultiID VW2DPickSet::pickSetID() const
{
    return pickset_ ? Pick::Mgr().get( *pickset_ ) : -1;
}
