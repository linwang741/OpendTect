/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert etc.
 * DATE     : 1996 / Jul 2007 / Mar 2017
-*/


#include "coltabmapper.h"
#include "iopar.h"
#include "keystrs.h"
#include "task.h"

static const char* sKeyStartWidth = "Start_Width";


ColTab::MapperSetup::MapperSetup()
    : isfixed_(false)
    , dohisteq_(defHistEq())
    , range_(RangeType::udf())
    , cliprate_(defClipRate())
    , guesssymmetry_(defAutoSymmetry())
    , symmidval_(defSymMidval())
    , nrsegs_(0)
    , sequsemode_(ColTab::UnflippedSingle)
    , rangeCalculated(this)
{
}


ColTab::MapperSetup::MapperSetup( const RangeType& rg )
    : isfixed_(true)
    , dohisteq_(defHistEq())
    , range_(rg)
    , cliprate_(defClipRate())
    , guesssymmetry_(defAutoSymmetry())
    , symmidval_(defSymMidval())
    , nrsegs_(0)
    , sequsemode_(ColTab::UnflippedSingle)
    , rangeCalculated(this)
{
}


ColTab::MapperSetup::MapperSetup( const MapperSetup& oth )
    : SharedObject(oth)
    , rangeCalculated(this)
{
    copyClassData( oth );
}


ColTab::MapperSetup::~MapperSetup()
{
    sendDelNotif();
    detachAllNotifiers();
}


mImplMonitorableAssignment( ColTab::MapperSetup, SharedObject )


void ColTab::MapperSetup::copyClassData( const ColTab::MapperSetup& oth )
{
    isfixed_ = oth.isfixed_;
    dohisteq_ = oth.dohisteq_;
    range_ = oth.range_;
    cliprate_ = oth.cliprate_;
    guesssymmetry_ = oth.guesssymmetry_;
    symmidval_ = oth.symmidval_;
    nrsegs_ = oth.nrsegs_;
    sequsemode_ = oth.sequsemode_;
}


Monitorable::ChangeType ColTab::MapperSetup::compareClassData(
					const MapperSetup& oth ) const
{
    mStartMonitorableCompare();
    mHandleMonitorableCompare( isfixed_, cIsFixedChange() );
    mHandleMonitorableCompare( dohisteq_, cDoHistEqChange() );
    mHandleMonitorableCompare( nrsegs_, cSegChange() );
    mHandleMonitorableCompare( sequsemode_, cUseModeChange() );
    if ( isfixed_ )
    {
	mHandleMonitorableCompare( range_, cRangeChange() );
    }
    else
    {
	mHandleMonitorableCompare( cliprate_, cAutoScaleChange() );
	mHandleMonitorableCompare( guesssymmetry_, cAutoScaleChange() );
	mHandleMonitorableCompare( symmidval_, cAutoScaleChange() );
    }
    mDeliverMonitorableCompare();
}


void ColTab::MapperSetup::setNotFixed()
{
    mLock4Read();
    if ( !isfixed_ )
	return;
    if ( !mLock2Write() && !isfixed_ )
	return;

    isfixed_ = false;
    range_.setUdf();
    mSendChgNotif( cIsFixedChange(), ChangeData::cUnspecChgID() );
}


void ColTab::MapperSetup::setFixedRange( RangeType newrg )
{
    mLock4Read();
    if ( mIsUdf(newrg.start) )
	newrg.start = range_.start;
    if ( mIsUdf(newrg.stop) )
	newrg.stop = range_.stop;

    if ( isfixed_ && newrg == range_ )
	return;

    mLock2Write();
    const bool isfixedchgd = !isfixed_;
    const bool rgchgd = newrg != range_;
    if ( !isfixedchgd && !rgchgd )
	return;

    range_ = newrg;
    isfixed_ = true;
    if ( isfixedchgd && rgchgd )
	mSendEntireObjChgNotif();
    else if ( isfixedchgd )
	mSendChgNotif( cIsFixedChange(), ChangeData::cUnspecChgID() );
    else
	mSendChgNotif( cRangeChange(), ChangeData::cUnspecChgID() );
}


bool ColTab::MapperSetup::isMappingChange( ChangeType ct ) const
{
    if ( ct == ChangeData::cEntireObjectChgType() )
	return true;

    mLock4Read();
    if ( ct == cIsFixedChange() && isfixed_ )
	return false; // the auto-determined ranges have become fixed

    if ( ct == cDoHistEqChange()
      || ct == cSegChange()
      || ct == cUseModeChange() )
	return true;

    return isfixed_ ? ct == cRangeChange() : true;
}


void ColTab::MapperSetup::setCalculatedRange( const RangeType& newrg ) const
{
    mLock4Write();
    MapperSetup& self = *const_cast<MapperSetup*>( this );
    self.range_ = newrg;
    mUnlockAllAccess();
    self.rangeCalculated.trigger();
}


void ColTab::MapperSetup::fillPar( IOPar& par ) const
{
    mLock4Read();
    par.set( sKey::Type(), dohisteq_ ? (isfixed_ ? "HistEq" : "VarHistEq")
				     : (isfixed_ ? "Fixed" : "Auto") );
    par.set( sKeyRange(), range_ );
    par.set( sKeyClipRate(), cliprate_ );
    par.set( sKeySymMidVal(), symmidval_ );
    par.setYN( sKeyAutoSym(), guesssymmetry_ );
    par.setYN( sKeyFlipSeq(), isFlipped(sequsemode_) );
    par.setYN( sKeyCycleSeq(), isCyclic(sequsemode_) );
    par.removeWithKey( sKeyStartWidth );
    par.removeWithKey( "Max Pts" );
}


void ColTab::MapperSetup::usePar( const IOPar& par )
{
    mLock4Write();
    const char* typestr = par.find( sKey::Type() );
    if ( typestr && *typestr )
    {
	if ( *typestr == 'F' || *typestr == 'f' )
	    { isfixed_ = true; dohisteq_ = false; }
	else if ( *typestr == 'A' || *typestr == 'a' )
	    { isfixed_ = false; dohisteq_ = false; }
	else if ( *typestr == 'H' || *typestr == 'h' )
	    { isfixed_ = true; dohisteq_ = true; }
	else if ( *typestr == 'V' || *typestr == 'v' )
	    { isfixed_ = false; dohisteq_ = true; }
    }

    if ( par.find(sKeyRange()) )
	par.get( sKeyRange(), range_ );
    else if ( par.find(sKeyStartWidth) ) // Legacy key
    {
	ValueType start, width;
	par.get( sKeyStartWidth, start, width );
	range_.start = start;
	range_.stop = start + width;
    }
    if ( isfixed_ )
    {
	if ( mIsUdf(range_.start) )
	{
	    if ( mIsUdf(range_.stop) )
		range_.stop = 1.0f;
	    range_.start = range_.stop - 1.0f;
	}
	if ( mIsUdf(range_.stop) )
	    range_.stop = range_.start + 1.0f;
    }

    par.get( sKeyClipRate(), cliprate_ );
    par.get( sKeySymMidVal(), symmidval_ );
    par.getYN( sKeyAutoSym(), guesssymmetry_ );
    if ( !isfixed_ )
    {
	if ( mIsUdf(cliprate_.first) )
	{
	    if ( mIsUdf(cliprate_.second) )
		cliprate_ = defClipRate();
	    else
		cliprate_.first = cliprate_.second;
	}
	if ( mIsUdf(cliprate_.second) )
	    cliprate_.second = cliprate_.first;
    }

    bool flipseq = isFlipped( sequsemode_ );
    bool cycleseq = isCyclic( sequsemode_ );
    par.getYN( sKeyFlipSeq(), flipseq );
    par.getYN( sKeyCycleSeq(), cycleseq );
    sequsemode_ = getSeqUseMode( flipseq, cycleseq );

    mSendEntireObjChgNotif();
}


ColTab::Mapper::Mapper()
    : setup_(new MapperSetup)
    , distrib_(new DistribType)
{
    setNotifs();
}


ColTab::Mapper::Mapper( RangeType rg )
    : setup_(new MapperSetup(rg))
    , distrib_(new DistribType)
{
    setNotifs();
}


ColTab::Mapper::Mapper( const MapperSetup& msu )
    : setup_(msu.clone())
    , distrib_(new DistribType)
{
    setNotifs();
}


ColTab::Mapper::Mapper( const Mapper& oth )
    : SharedObject(oth)
    , setup_(new MapperSetup)
    , distrib_(new DistribType)
{
    copyClassData( oth );
    setNotifs();
}


void ColTab::Mapper::setNotifs()
{
    mAttachCB( setup_->objectChanged(), Mapper::setupChgCB );
    mAttachCB( distrib_->objectChanged(), Mapper::distribChgCB );
}


ColTab::Mapper::~Mapper()
{
    sendDelNotif();
    detachAllNotifiers();
}


mImplMonitorableAssignment( ColTab::Mapper, SharedObject )


void ColTab::Mapper::copyClassData( const Mapper& oth )
{
    setup_ = oth.setup_->clone();
    distrib_ = oth.distrib_->clone();
}


Monitorable::ChangeType ColTab::Mapper::compareClassData(
						const Mapper& oth ) const
{
    mDeliverYesNoMonitorableCompare(
	    *setup_ == *oth.setup_ && *distrib_ == *oth.distrib_ );
}


ColTab::PosType ColTab::Mapper::getLinRelPos( const RangeType& rg,
						      ValueType val )
{
    const PosType width = rg.width( false );
    return width == 0.f ? 0.5f : (val-rg.start) / width;
}


ColTab::PosType ColTab::Mapper::getHistEqRelPos( const RangeType& rg,
							 ValueType val ) const
{
    mLock4Read();
    MonitorLock ml( *distrib_ );
    const ValueType cumval = distrib_->valueAt( val, true );
    const float relposinrg = cumval / distrib_->sumOfValues();
    return rg.start + relposinrg * rg.width();
}


void ColTab::Mapper::determineRange() const
{
    mLock4Read();
    RangeType rg;
    if ( distrib_->isEmpty() )
	{ rg.start = 0.f; rg.stop = 1.f; }
    else
    {
	const ClipRatePair clips( setup_->clipRate() );
	const float sumvals = distrib_->sumOfValues();
	const ClipRatePair distrvals( clips.first * sumvals,
			       (1.0f-clips.second) * sumvals );
	rg.start = distrib_->positionForCumulative( distrvals.first );
	rg.stop = distrib_->positionForCumulative( distrvals.second );
    }

    setup_->setCalculatedRange( rg );
}


ColTab::Mapper::RangeType ColTab::Mapper::getRange() const
{
    mLock4Read();
    const Interval<float> rg = setup_->range();
    if ( !mIsUdf(rg.start) && !mIsUdf(rg.stop) )
	return rg;

    determineRange();
    return setup_->range();
}


ColTab::PosType ColTab::Mapper::relPosition( ValueType val ) const
{
    if ( mIsUdf(val) )
	return 0.5f;

    mLock4Read();
    Interval<float> rg = setup_->range();
    if ( mIsUdf(rg.start) || mIsUdf(rg.stop) )
    {
	determineRange();
	rg = setup_->range();
    }

    const bool dohisteq = !distrib_->isEmpty() && setup_->doHistEq();
    float relpos = dohisteq ? getHistEqRelPos(rg,val) : getLinRelPos(rg,val);
    const int nrsegs = setup_->nrSegs();

    if ( nrsegs > 0 )
    {
	relpos *= nrsegs;
	relpos = (0.5f + ((int)relpos)) / nrsegs;
    }

    if ( relpos > 1.f )
	relpos = 1.f;
    else if ( relpos < 0.f )
	relpos = 0.f;

    return relpos;
}


ColTab::PosType ColTab::Mapper::seqPosition( ValueType val ) const
{
    PosType seqpos = relPosition( val );

    mLock4Read();
    const SeqUseMode sequsemode = setup_->seqUseMode();
    if ( isCyclic(sequsemode) )
    {
	seqpos *= 2;
	if ( seqpos > 1.0f )
	    seqpos = 2.0f - seqpos;
    }
    if ( isFlipped(sequsemode) )
	seqpos = 1.0f - seqpos;

    return seqpos;
}


int ColTab::Mapper::colIndex( ValueType val, int nrcols ) const
{
    int ret = (int)(nrcols * seqPosition(val));
    return ret < 0 ? 0 : (ret >=nrcols ? nrcols-1 : ret);
}


void ColTab::Mapper::setupChgCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );
    mLock4Read();

    if ( setup_->isMappingChange(chgdata.changeType()) )
	mSendChgNotif( cMappingChange(), ChangeData::cUnspecChgID() );
}


void ColTab::Mapper::distribChgCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );
    mLock4Read();

    if ( !setup_->isFixed() )
	determineRange();

    mSendChgNotif( cMappingChange(), ChangeData::cUnspecChgID() );
}
