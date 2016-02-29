/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "volprocsurfacelimitedfiller.h"

#include "arrayndimpl.h"
#include "binidvalset.h"
#include "emfault3d.h"
#include "emhorizon.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "separstr.h"
#include "ioman.h"
#include "ioobj.h"
#include "executor.h"
#include "mousecursor.h"
#include "seisdatapack.h"
#include "survinfo.h"

namespace VolProc
{

#define mBelow		1
#define mAbove		-1


static const char*	sKeyTopHorID()		{ return "Top horizon"; }
static const char*	sKeyBotHorID()		{ return "Bottom horizon"; }
static const char*	sKeyTopValue()		{ return "Top Value"; }
static const char*	sKeyBotValue()		{ return "Bottom Value"; }
static const char*	sKeyGradient()		{ return "Gradient"; }
static const char*	sKeyUseGradient()	{ return "Use Gradient"; }
static const char*	sKeyHorInterFiller()	{ return "HorInterFiller"; }


SurfaceLimitedFiller::SurfaceLimitedFiller()
    : fixedstartval_( 2000 )
    , fixedgradient_( mCast(float,SI().zDomain().userFactor()) )
    , refz_( 0 )
    , gradhorizon_( 0 )
    , refhorizon_( 0 )
    , starthorizon_( 0 )
    , gradhormid_( 0 )
    , starthormid_( 0 )
    , refhormid_( 0 )
    , usebottomval_( false  )
    , usegradient_( true )
    , usestartval_( true )
    , userefz_( true )
    , gradvertical_( true )
    , startauxdataselidx_( -1 )
    , gradauxdataselidx_( -1 )
    , gradauxidx_( -1 )
    , startauxidx_( -1 )
{
    hors_.allowNull( true );
    faults_.allowNull( true );
}


void SurfaceLimitedFiller::initClass()
{
    SeparString sep( 0, FactoryBase::cSeparator() );
    sep += sFactoryKeyword();
    sep += sKeyHorInterFiller();

    SurfaceLimitedFiller::factory().addCreator( createInstance, sep.buf(),
						sFactoryDisplayName() );
}


SurfaceLimitedFiller::~SurfaceLimitedFiller()
{
    releaseData();
}


void SurfaceLimitedFiller::releaseData()
{
    Step::releaseData();

    deepUnRef( hors_ );
    deepErase( faults_ );

    unRefAndZeroPtr( starthorizon_ );
    unRefAndZeroPtr( refhorizon_ );
    unRefAndZeroPtr( gradhorizon_ );;
}


const MultiID* SurfaceLimitedFiller::getSurfaceID( int idx ) const
{ return surfacelist_.validIdx(idx) ? &surfacelist_[idx] : 0; }


const MultiID* SurfaceLimitedFiller::getStartValueHorizonID() const
{ return starthormid_ && !starthormid_.isEmpty() ? &starthormid_ : 0; }


const MultiID* SurfaceLimitedFiller::getGradientHorizonID() const
{ return gradhormid_ && !gradhormid_.isEmpty() ? &gradhormid_ : 0; }


const MultiID* SurfaceLimitedFiller::getRefHorizonID() const
{ return refhormid_ && !refhormid_.isEmpty() ? &refhormid_ : 0; }


static bool setTargetMultiID( const MultiID* mid, MultiID& targetmid )
{
    if ( !mid || mid->isEmpty() )
	return false;

    targetmid = *mid;
    return true;
}


bool SurfaceLimitedFiller::setStartValueHorizon( const MultiID* mid )
{ return setTargetMultiID( mid, starthormid_ ); }


bool SurfaceLimitedFiller::setGradientHorizon( const MultiID* mid )
{ return setTargetMultiID( mid, gradhormid_ ); }


bool SurfaceLimitedFiller::setRefHorizon( const MultiID* mid )
{ return setTargetMultiID( mid, refhormid_ ); }


bool SurfaceLimitedFiller::setSurfaces( const TypeSet<MultiID>& hids,
				      const TypeSet<char>& geofillside )
{
    deepUnRef( hors_ );
    side_.erase();
    surfacelist_.erase();

    if ( !hids.size() )
	return true;

    for ( int idx=0; idx<hids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( hids[idx] );
	if ( !ioobj )
	{
	    errmsg_ = tr("Object does not exist, "
			 "could not find entry for ID: %1")
		    .arg( toString( hids[idx] ) );
	    return false;
	}

	surfacelist_ += hids[idx];
	side_ += geofillside[idx];
    }

    return side_.size();
}


EM::Horizon* SurfaceLimitedFiller::loadHorizon( const MultiID& mid ) const
{
    RefMan<EM::EMObject> emobj = EM::EMM().loadIfNotFullyLoaded( mid );
    mDynamicCastGet( EM::Horizon*, newhor, emobj.ptr() );
    if ( !newhor ) return 0;

    newhor->ref();
    return newhor;
}


int SurfaceLimitedFiller::setDataHorizon( const MultiID& mid,
				  EM::Horizon3D*& hor3d, int auxdataidx ) const
{
    if ( mid.isEmpty() ) return -1;

    EM::SurfaceIOData surfiod;
    uiString emsg;
    if ( !EM::EMM().getSurfaceData( mid, surfiod, emsg ) ||
	 !surfiod.valnames.validIdx(auxdataidx))
    {
	return -1;
    }

    const BufferString auxdataname = surfiod.valnames.get(auxdataidx);

    EM::Horizon* hor = loadHorizon( mid );
    if ( !hor ) return -1;
    mDynamicCastGet( EM::Horizon3D*, newhor, hor );
    if ( !newhor )
    {
	hor->unRef();
	return -1;
    }

    hor3d = newhor;

    Executor* loader = hor3d->auxdata.auxDataLoader( auxdataidx );
    if ( !loader || !loader->execute() )
    {
	unRefAndZeroPtr( hor3d );
	return -1;
    }

    return hor3d->auxdata.auxDataIndex( auxdataname );
}




bool SurfaceLimitedFiller::prepareComp( int )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );

    if ( !usestartval_ )
    {
	startauxidx_ =
	    setDataHorizon( starthormid_, starthorizon_, startauxdataselidx_ );
    }

    if ( !usegradient_ )
    {
	gradauxidx_ =
	    setDataHorizon( gradhormid_, gradhorizon_, gradauxdataselidx_ );
    }

    if ( !userefz_ )
    {
	if ( refhormid_.isEmpty() )
	    return false;

	EM::Horizon* hor = loadHorizon( refhormid_ );
	if ( !hor ) return false;

	refhorizon_ = hor;
    }

    for ( int idx=0; idx<surfacelist_.size(); idx++ )
    {
	RefMan<EM::EMObject> emobj =
	    EM::EMM().loadIfNotFullyLoaded( surfacelist_[idx] );
	mDynamicCastGet( EM::Horizon*, newhor, emobj.ptr() );
	if ( newhor )
	{
	    newhor->ref();
	    hors_ += newhor;
	    faults_ += 0;
	}
	else
	{
	    mDynamicCastGet( EM::Fault3D*, newft, emobj.ptr() );
	    faults_ += newft ? newft->geometry().sectionGeometry(0) : 0;
	    hors_ += 0;
	}
    }

    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( !output || output->isEmpty() )
	return false;

    return isOK();
}


bool SurfaceLimitedFiller::computeBinID( const BinID& bid, int )
{
    const RegularSeisDataPack* input = getInput( getInputSlotID(0) );
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    const Array3D<float>* inputarr =
	!input || input->isEmpty() ?  0 : &input->data( 0 );

    const TrcKeySampling& hs = output->sampling().hsamp_;

    const StepInterval<int> outputinlrg( hs.inlRange() );

    if ( !outputinlrg.includes( bid.inl(), false ) ||
         (bid.inl()-outputinlrg.start)%outputinlrg.step )
	return false;

    const StepInterval<int> outputcrlrg( hs.crlRange() );

    if ( !outputcrlrg.includes( bid.crl(), false ) ||
         (bid.crl()-outputcrlrg.start)%outputcrlrg.step )
	return false;

    StepInterval<int> inputinlrg;
    if ( inputarr )
    {
	inputinlrg = input->sampling().hsamp_.inlRange();
	if ( !inputinlrg.includes( bid.inl(), false ) ||
	     (bid.inl()-inputinlrg.start)%inputinlrg.step )
	    inputarr = 0;
    }

    StepInterval<int> inputcrlrg;
    if ( inputarr )
    {
	inputcrlrg = input->sampling().hsamp_.crlRange();
	if ( !inputcrlrg.includes( bid.crl(), false ) ||
	     (bid.crl()-inputcrlrg.start)%inputcrlrg.step )
	    inputarr = 0;
    }

    const od_int64 bidsq = bid.toInt64();
    const double fixedz = userefz_ ? refz_ :
	refhorizon_->getPos( refhorizon_->sectionID(0), bidsq ).z;

    double val0 = fixedstartval_;
    if ( !usestartval_ )
    {
	EM::PosID pid(starthorizon_->id(), starthorizon_->sectionID(0), bidsq);
	val0 = starthorizon_->auxdata.getAuxDataVal( startauxidx_, pid );
    }

    TypeSet<double> horz;
    bool allhordefined = true;
    for ( int idy=0; idy<hors_.size(); idy++ )
    {
	if ( !hors_[idy] ) continue;

	horz += hors_[idy]->getPos(hors_[idy]->sectionID(0),bidsq).z;
	if ( mIsUdf(horz[idy]) )
	{
	    allhordefined = false;
	    break;
	}
    }

    double gradient = fixedgradient_; //gradvertical_==true case done
    if ( !usegradient_ )
    {
	EM::PosID pid( gradhorizon_->id(), gradhorizon_->sectionID(0), bidsq );
	gradient = gradhorizon_->auxdata.getAuxDataVal( gradauxidx_, pid );
    }
    else if ( usebottomval_ )
    {
	const StepInterval<float>& zrg = SI().zRange( true );
	const double topdepth = horz.size() > 0 && !mIsUdf(horz[0]) ?
				horz[0] : zrg.start;
	const double bottomdepth = horz.size() > 1 && !mIsUdf(horz[1]) ?
				   horz[1] : zrg.stop;
	const double depth = bottomdepth - topdepth;
	gradient = valrange_ / depth;
    }

    const int inputinlidx = inputarr ? inputinlrg.nearestIndex(bid.inl()) : -1;
    const int inputcrlidx = inputarr ? inputcrlrg.nearestIndex(bid.crl()) : -1;
    const int outputinlidx = outputinlrg.nearestIndex( bid.inl() );
    const int outputcrlidx = outputcrlrg.nearestIndex( bid.crl() );
    const int outputmaxidx = output->sampling().nrZ() - 1;
    const bool initok = !mIsUdf(val0) && !mIsUdf(gradient) && !mIsUdf(fixedz);

    for ( int idx=outputmaxidx; idx>=0; idx-- )
    {
	const double curz = output->sampling().zsamp_.atIndex( idx );
	bool cancalculate = allhordefined;
	if ( allhordefined )
	{
	    for ( int idy=0; idy<horz.size(); idy++ )
	    {
		if ( (horz[idy]>curz && side_[idy]==mBelow) ||
		     (horz[idy]<curz && side_[idy]==mAbove) )
		{
		    cancalculate = false;
		    break;
		}
	    }
	}

	double value = mUdf(double);
	if ( cancalculate && initok )
	    value = val0 + ( curz - fixedz ) * gradient;
	else if ( inputarr )
	    value = (double)inputarr->get( inputinlidx, inputcrlidx, idx );

	output->data(0).set( outputinlidx, outputcrlidx, idx, (float)value );
    }

    return true;
}


void SurfaceLimitedFiller::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );

    pars.set( sKeyNrSurfaces(), side_.size() );
    for ( int idx=0; idx<side_.size(); idx++ )
    {
	BufferString midkey = sKeySurfaceID();
	midkey += idx;
	pars.set( midkey, surfacelist_[idx] );

	BufferString sidekey = sKeySurfaceFillSide();
	sidekey += idx;
	pars.set( sidekey, side_[idx] );
    }

    pars.setYN( sKeyUseStartValue(), usestartval_ );
    if ( usestartval_ )
	pars.set( sKeyStartValue(), fixedstartval_ );
    else
    {
	pars.set( sKeyStartValHorID(), starthormid_ );
	pars.set( sKeyStartAuxDataID(), startauxdataselidx_ );
    }

    pars.setYN( sKeyUseGradValue(), usegradient_ );
    pars.setYN( sKeyGradType(), gradvertical_ );
    if ( usegradient_ )
	pars.set( sKeyGradValue(), fixedgradient_ );
    else
    {
	pars.set( sKeyGradHorID(), gradhormid_ );
	pars.set( sKeyGradAuxDataID(), gradauxdataselidx_ );
    }

    pars.setYN( sKeyUseRefZ(), userefz_ );
    if ( userefz_ )
	pars.set( sKeyRefZ(), refz_ );
    else
	pars.set( sKeyRefHorID(), refhormid_ );
}


bool SurfaceLimitedFiller::useHorInterFillerPar( const IOPar& pars )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    if ( !Step::usePar( pars ) )
	return false;

    usestartval_ = true; usegradient_ = true;
    userefz_ = true; refz_ = 0;

    float topvalue = mUdf(float);
    if ( pars.get(sKeyTopValue(),topvalue) && !mIsUdf(topvalue) )
	fixedstartval_ = topvalue;

    bool usegradient = true; float gradient = mUdf(float);
    pars.getYN( sKeyUseGradient(), usegradient );
    if (usegradient && pars.get(sKeyGradient(),gradient) && !mIsUdf(gradient))
	fixedgradient_ = gradient;

    TypeSet<MultiID> horids;
    MultiID tophorid;
    if ( pars.get(sKeyTopHorID(), tophorid) )
    {
	horids += tophorid;
	userefz_ = false;
	if ( !setRefHorizon( &tophorid ) )
	    return false;
    }

    MultiID bottomhorid;
    if ( pars.get(sKeyBotHorID(), bottomhorid) )
	horids += bottomhorid;

    float bottomvalue = mUdf(float);
    if ( !usegradient && pars.get(sKeyBotValue(),bottomvalue) &&
	 !mIsUdf(bottomvalue) )
    {
	valrange_ = bottomvalue - topvalue;
	const StepInterval<float>& zrange = SI().zRange(true);
	fixedgradient_ = mCast(float,valrange_/(zrange.stop-zrange.start));
	if ( horids.size() > 0 )
	    usebottomval_ = true;
    }

    TypeSet<char> sides;
    for ( int idx=0; idx<horids.size(); idx++ )
	sides += mCast(char,idx?mAbove:mBelow);

    if ( !setSurfaces(horids, sides) )
	return false;

    return true;
}


bool SurfaceLimitedFiller::usePar( const IOPar& pars )
{
    BufferString type;
    pars.get( sKey::Type(), type );
    if ( type == sKeyHorInterFiller() )
	return useHorInterFillerPar( pars );

    MouseCursorChanger cursorlock( MouseCursor::Wait );
    if ( !Step::usePar( pars ) )
	return false;

    int nrhors = 0;
    pars.get( sKeyNrSurfaces(), nrhors );
    TypeSet<MultiID> horbds;
    TypeSet<char> sides;
    for ( int idx=0; idx<nrhors; idx++ )
    {
	BufferString midkey = sKeySurfaceID();
	midkey += idx;

	MultiID mid;
	pars.get( midkey, mid );
	horbds += mid;

	BufferString sidekey = sKeySurfaceFillSide();
	sidekey += idx;

	int side;
	pars.get( sidekey, side );
	sides += mCast( char , side );
    }

    if ( !setSurfaces(horbds, sides) )
	return false;

    pars.getYN( sKeyUseStartValue(), usestartval_ );
    if ( usestartval_ )
	pars.get( sKeyStartValue(), fixedstartval_ );
    else
    {
	MultiID mid;
	if ( !pars.get(sKeyStartValHorID(),mid) )
	    return false;

	if ( !setStartValueHorizon( &mid ) )
	    return false;

	if ( !pars.get(sKeyStartAuxDataID(),startauxdataselidx_) )
	    return false;
    }

    pars.getYN( sKeyGradType(), gradvertical_ );
    pars.getYN( sKeyUseGradValue(), usegradient_ );
    if ( usegradient_ )
	pars.get( sKeyGradValue(), fixedgradient_ );
    else
    {
	MultiID mid;
	if ( !pars.get(sKeyGradHorID(),mid) )
	    return false;

	if ( !setGradientHorizon( &mid ) )
	    return false;

	if ( !pars.get(sKeyGradAuxDataID(),gradauxdataselidx_) )
	    return false;
    }

    pars.getYN( sKeyUseRefZ(), userefz_ );
    if ( userefz_ )
	pars.get( sKeyRefZ(), refz_ );
    else
    {
	MultiID mid;
	if ( !pars.get(sKeyRefHorID(),mid) )
	    return false;

	if ( !setRefHorizon( &mid ) )
	    return false;
    }

    return true;
}


bool SurfaceLimitedFiller::isOK() const
{
    if ( usegradient_ && mIsUdf(fixedgradient_) )
	return false;

    if ( !usegradient_ && !gradhorizon_->auxdata.nrAuxData() )
	return false;

    if ( usestartval_ && mIsUdf(fixedstartval_) )
	return false;

    if ( !usestartval_ && !starthorizon_->auxdata.nrAuxData() )
	return false;

    if ( userefz_ && mIsUdf(refz_) )
	return false;

    if ( !userefz_ && !refhorizon_ )
	return false;

    return true;
}


od_int64 SurfaceLimitedFiller::extraMemoryUsage( OutputSlotID,
	const TrcKeySampling& hsamp, const StepInterval<int>& zsamp ) const
{ return  getBaseMemoryUsage( hsamp, zsamp ); }


} // namespace VolProc
