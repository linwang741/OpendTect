/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: stratsynth.cc,v 1.19 2011-11-03 15:20:37 cvsbruno Exp $";


#include "stratsynth.h"

#include "elasticpropsel.h"
#include "flatposdata.h"
#include "prestackgather.h"
#include "propertyref.h"
#include "survinfo.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "synthseis.h"
#include "velocitycalc.h"
#include "wavelet.h"


StratSynth::StratSynth( const Strat::LayerModel& lm )
    : lm_(lm)
    , wvlt_(0)
{}


StratSynth::~StratSynth()
{
    deepErase( synthetics_ );
    setWavelet( 0 );
}


void StratSynth::setWavelet( const Wavelet* wvlt )
{
    delete wvlt_;
    wvlt_ = wvlt;
}


void StratSynth::clearSynthetics()
{
    deepErase( synthetics_ );
}


#define mErrRet( msg, act )\
{\
    errmsg_ = "Can not generate synthetics:\n";\
    errmsg_ += msg;\
    act;\
}

void StratSynth::addSynthetics( SyntheticData* sd )
{
    synthetics_ += synthetics_.replace( 0, generate() );
}


SyntheticData* StratSynth::getSynthetic( int selid ) 
{
    if ( !selid )
    {
	if ( !synthetics_.isEmpty() )
	    delete synthetics_.remove(0);

	synthetics_.insertAt( generate(), 0 );
    }
    if ( synthetics_.validIdx( selid ) )
	return synthetics_[selid];

    return 0;
}


SyntheticData* StratSynth::generate()
{
    errmsg_.setEmpty(); 

    if ( lm_.isEmpty() ) 
	return false;

    Seis::RaySynthGenerator synthgen;
    synthgen.setWavelet( wvlt_, OD::UsePtr );
    synthgen.usePar( raypars_ );

    const int nraimdls = lm_.size();
    int maxsz = 0;
    for ( int idm=0; idm<nraimdls; idm++ )
    {
	ElasticModel aimod; 
	const Strat::LayerSequence& seq = lm_.sequence( idm ); 
	const int sz = seq.size();
	if ( sz < 1 )
	    continue;

	if ( !fillElasticModel( aimod, seq ) )
	{
	    BufferString msg( errmsg_ );
	    mErrRet( msg.buf(), return false;) 
	}
	maxsz = mMAX( aimod.size(), maxsz );
	synthgen.addModel( aimod );
    }
    if ( maxsz <= 1 )
	mErrRet( "AI model has only one layer, please add an other layer", 
		return false; );

    if ( !synthgen.doWork() )
    {
	const char* errmsg = synthgen.errMsg();
	mErrRet( errmsg ? errmsg : "", return 0 ) ;
    }

    const int crlstep = SI().crlStep();
    const BinID bid0( SI().inlRange(false).stop + SI().inlStep(),
	    	      SI().crlRange(false).stop + crlstep );

    ObjectSet<PreStack::Gather> gatherset;
    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	Seis::RaySynthGenerator::RayModel& rm = synthgen.result( imdl );
	PreStack::Gather* gather = new PreStack::Gather();
	ObjectSet<SeisTrc> trcs; rm.getTraces( trcs, true );
	SeisTrcBuf tbuf( false );
	for ( int idx=0; idx<trcs.size(); idx++ )
	{
	    SeisTrc* trc = trcs[idx];
	    trc->info().binid = BinID( bid0.inl, bid0.crl + imdl * crlstep );
	    trc->info().coord = SI().transform( trc->info().binid );
	    tbuf.add( trc );
	}
	if ( !gather->setFromTrcBuf( tbuf, 0 ) )
	    { delete gather; continue; }

	gatherset += gather;
    }

    PreStack::GatherSetDataPack* gdp = 
		    new PreStack::GatherSetDataPack( 0, gatherset );
    SyntheticData* sd = new SyntheticData( "Synthetic", *gdp );

    ObjectSet<TimeDepthModel> tmpd2ts;
    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	Seis::RaySynthGenerator::RayModel& rm = synthgen.result( imdl );
	rm.getD2T( tmpd2ts, true );
	if ( !tmpd2ts.isEmpty() )
	    sd->d2tmodels_ += tmpd2ts.remove(0);
	deepErase( tmpd2ts );
    }
    return sd;
}


const char* StratSynth::errMsg() const
{
    return errmsg_.isEmpty() ? 0 : errmsg_.buf();
}


bool StratSynth::fillElasticModel( ElasticModel& aimodel, 
				const Strat::LayerSequence& seq )
{
    const ElasticPropSelection& eps = lm_.elasticPropSel();
    const PropertyRefSelection& props = lm_.propertyRefs();
    if ( !eps.isValidInput( &errmsg_ ) )
	return false; 

    ElasticPropGen elpgen( eps, props );
    const ElasticPropertyRef& denref = eps.getPropertyRef(ElasticFormula::Den); 
    const ElasticPropertyRef& pvref = eps.getPropertyRef(ElasticFormula::PVel); 
    const ElasticPropertyRef& svref = eps.getPropertyRef(ElasticFormula::SVel); 

    const Strat::Layer* lay = 0;
    for ( int idx=0; idx<seq.size(); idx++ )
    {
	lay = seq.layers()[idx];
	const float dval  = elpgen.getVal(denref,lay->values(),props.size());
	const float pval = elpgen.getVal(pvref,lay->values(),props.size());
	const float sval = elpgen.getVal(svref,lay->values(),props.size());

	ElasticLayer ail ( lay->thickness(), pval, sval, dval );
	aimodel += ail;
    }
    return true;
}




SyntheticData::SyntheticData( const char* nm, 
			      PreStack::GatherSetDataPack& p )
    : NamedObject(nm)
    , prestackpack_(p)
    , poststackpack_(0)		      
    , poststackpackid_(DataPack::cNoID())    
    , prestackpackid_(DataPack::cNoID())    
{
    p.setName( name() );
    setPack( true, &p );
    setPostStack( 0 );
}


SyntheticData::~SyntheticData()
{
    deepErase( d2tmodels_ );
    removePack( false );
    removePack( true ); 
}


void SyntheticData::setName( const char* nm )
{
    NamedObject::setName( nm );
    prestackpack_.setName( nm );
    if ( poststackpack_ ) 
	poststackpack_->setName( nm );
}


const DataPack* SyntheticData::getPack( bool isps ) const
{
    if ( isps ) 
	return &prestackpack_;
    else
	return poststackpack_;
}


void SyntheticData::setPostStack( float offset, const Interval<float>* stackrg )
{
    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    Interval<float> offrg = stackrg ? *stackrg : Interval<float>(offset,offset);
    prestackpack_.fill( *tbuf, offrg );
    if ( tbuf->isEmpty() )
	return;

    SeisTrcBufDataPack* tdp = new SeisTrcBufDataPack( 
			tbuf, Seis::Line, SeisTrcInfo::TrcNr, "Seismic" ) ;

    const SeisTrc& trc0 = *tbuf->get(0);
    StepInterval<double> zrg( trc0.info().sampling.start,
			      trc0.info().sampling.atIndex(trc0.size()-1),
			      trc0.info().sampling.step );
    tdp->posData().setRange( true, StepInterval<double>(1,tbuf->size(),1) );
    tdp->posData().setRange( false, zrg );
    tdp->setName( name() ); 

    setPack( false, tdp );
    poststackpack_ = tdp; 
}


void SyntheticData::setPack( bool isps, DataPack* dp )
{
    removePack( isps );
    if ( !dp ) return;
    DataPackMgr::ID pmid = isps ? DataPackMgr::CubeID() : DataPackMgr::FlatID();
    DPM( pmid ).add( dp );
    DataPack::FullID& dpid = isps ? prestackpackid_ : poststackpackid_;
    dpid = DataPack::FullID( pmid, dp->id());
}


void SyntheticData::removePack( bool isps )
{
    const DataPack::FullID dpid = isps ? prestackpackid_ : poststackpackid_;
    DataPackMgr::ID packmgrid = DataPackMgr::getID( dpid );
    const DataPack* dp = DPM(packmgrid).obtain( DataPack::getID(dpid) );
    if ( dp )
	DPM(packmgrid).release( dp->id() );
}


const Interval<float> SyntheticData::offsetRange() const
{
    Interval<float> offrg( 0, 0 );
    const ObjectSet<PreStack::Gather>& gathers = prestackpack_.getGathers();
    if ( gathers.isEmpty() ) return offrg;
    const PreStack::Gather& gather = *gathers[0];
    offrg.set( gather.getOffset(0), gather.getOffset( gather.size(true)-1) );
    return offrg;
}
