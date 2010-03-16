/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2010
-*/

static const char* rcsID = "$Id: seisbayesclass.cc,v 1.8 2010-03-16 13:17:25 cvsbert Exp $";

#include "seisbayesclass.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisbuf.h"
#include "seisbounds.h"
#include "probdenfunc.h"
#include "probdenfunctr.h"
#include "keystrs.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"

const char* SeisBayesClass::sKeyPDFID()		{ return "PDF.ID"; }
const char* SeisBayesClass::sKeyAPProbID()	{ return "PDF.Norm.APProb.ID"; }
const char* SeisBayesClass::sKeyNormPol()	{ return "PDF.Norm.Policy"; }
const char* SeisBayesClass::sKeyPreScale()	{ return "PDF.Norm.PreScale"; }
const char* SeisBayesClass::sKeySeisInpID()	{ return "Seismics.Input.ID"; }
const char* SeisBayesClass::sKeySeisOutID()	{ return "Seismics.Output.ID"; }

DefineEnumNames(SeisBayesClass,NormPol,0,"Normalization Policy")
    { "None", "Per bin", "Joint", "Per PDF", 0 };


SeisBayesClass::SeisBayesClass( const IOPar& iop )
    	: Executor( "Bayesian inversion" )
	, pars_(*new IOPar(iop))
	, needclass_(false)
	, nrdone_(0)
	, totalnr_(-2)
	, inptrcs_(*new SeisTrcBuf(true))
	, aptrcs_(*new SeisTrcBuf(true))
	, outtrcs_(*new SeisTrcBuf(true))
	, initstep_(1)
	, nrdims_(0)
	, normpol_(None)
{
    aprdrs_.allowNull( true );

    const char* res = pars_.find( sKey::Type );
    is2d_ = res && *res == '2';
    if ( is2d_ )
	{ msg_ = "2D not implemented"; return; }

    res = pars_.find( sKeyNormPol() );
    if ( res && *res )
	normpol_ = eEnum(NormPol,res);

    msg_ = "Initializing";
}

#define mAddIdxRank(idx) msg_ += idx+1; msg_ += getRankPostFix(idx+1)


SeisBayesClass::~SeisBayesClass()
{
    cleanUp();

    delete &pars_;
    delete &inptrcs_;
    delete &aptrcs_;
    delete &outtrcs_;
}


void SeisBayesClass::cleanUp()
{
    deepErase(inppdfs_);
    deepErase(pdfs_);
    deepErase(rdrs_);
    deepErase(aprdrs_);
    deepErase(wrrs_);
}


bool SeisBayesClass::getPDFs()
{
    for ( int ipdf=0; ; ipdf++ )
    {
	const char* id = pars_.find( mGetSeisBayesPDFIDKey(ipdf) );
	if ( !id || !*id )
	    break;

	PtrMan<IOObj> ioobj = IOM().get( MultiID(id) );
	if ( !ioobj )
	{
	    msg_ = "Cannot find object for "; mAddIdxRank(ipdf);
	    msg_ += " PDF in data store"; return false;
	}

	ProbDenFunc* pdf = ProbDenFuncTranslator::read( *ioobj, &msg_ );
	if ( !pdf )
	    return false;

	inppdfs_ += pdf;
	pdfnames_.add( ioobj->name() );

	const ProbDenFunc& pdf0 = *inppdfs_[0];
	if ( ipdf == 0 )
	    const_cast<int&>(nrdims_) = pdf->nrDims();
	else if ( pdf->nrDims() != nrdims_ )
	    { msg_ = "PDF's have different dimensions"; return false; }
	else if ( !pdf->isCompatibleWith(pdf0) )
	    { msg_ = "PDF's are not compatible"; return false; }

	TypeSet<int>* idxs = new TypeSet<int>;
	pdf->getIndexTableFor( pdf0, *idxs );
	pdfxtbls_ += idxs;

	const char* res = pars_.find( mGetSeisBayesPreScaleKey(ipdf) );
	float scl = 1;
	if ( res && *res ) scl = atof( res );
	if ( scl < 0 ) scl = -scl;
	if ( scl == 0 ) scl = 1;
	prescales_ += scl;

	aptrcs_.add( new SeisTrc );
	res = pars_.find( mGetSeisBayesAPProbIDKey(ipdf) );
	SeisTrcReader* rdr = 0;
	if ( res && *res )
	{
	    rdr = getReader( res, false, ipdf );
	    if ( !rdr ) return false;
	}
	aprdrs_ += rdr;
    }

    if ( inppdfs_.isEmpty() )
	{ msg_ = "No PDF's in parameters"; return false; }
    else if ( !scalePDFs() )
	return false;

    pdfnames_.add( "Classification" );
    pdfnames_.add( "Confidence" );
    initstep_ = 2;
    return true;
}


bool SeisBayesClass::scalePDFs()
{
    const int nrpdfs = pdfs_.size();

    for ( int ipdf=0; ipdf<nrpdfs; ipdf++ )
    {
	ProbDenFunc* pdf = inppdfs_[ipdf]->clone();
	pdfs_ += pdf;
    }

    return true;
}


SeisTrcReader* SeisBayesClass::getReader( const char* id, bool isdim, int idx )
{
    PtrMan<IOObj> ioobj = IOM().get( MultiID(id) );
    if ( !ioobj )
    {
	msg_.setEmpty(); const ProbDenFunc& pdf0 = *inppdfs_[0];
	if ( isdim )
	    msg_.add( "Cannot find input cube for " )
		.add( inppdfs_[0]->dimName(idx) );
	else
	    msg_.add( "Cannot find a priori scaling cube for " )
		.add( inppdfs_[idx]->name() );
	msg_.add( "\nID found is " ).add( id );
	return 0;
    }

    SeisTrcReader* rdr = new SeisTrcReader( ioobj );
    rdr->usePar( pars_ );
    if ( !rdr->prepareWork() )
    {
	msg_.setEmpty();
	msg_.add( "For " ).add( ioobj->name() ).add(":\n").add( rdr->errMsg() );
	delete rdr; return 0;
    }

    return rdr;
}


bool SeisBayesClass::getReaders()
{
    if ( inppdfs_.isEmpty() ) return false;
    const ProbDenFunc& pdf0 = *inppdfs_[0];
    for ( int idim=0; idim<nrdims_; idim++ )
    {
	inptrcs_.add( new SeisTrc );

	const char* id = pars_.find( mGetSeisBayesSeisInpIDKey(idim) );
	if ( !id || !*id )
	{
	    msg_ = "Cannot find "; mAddIdxRank(idim);
	    msg_ += " input cube (for "; msg_ += pdf0.dimName(idim);
	    msg_ += ") in parameters";
	    return false;
	}

	SeisTrcReader* rdr = getReader( id, true, idim );
	if ( !rdr ) return false;
	rdrs_ += rdr;
    }

    initstep_ = 3;
    return true;
}


bool SeisBayesClass::getWriters()
{
    const int nrpdfs = pdfs_.size();
    if ( nrpdfs < 1 ) return false;

    wrrs_.allowNull( true ); bool haveoutput = false;
    for ( int ipdf=0; ipdf<nrpdfs+2; ipdf++ )
    {
	outtrcs_.add( new SeisTrc );

	const char* id = pars_.find( mGetSeisBayesSeisOutIDKey(ipdf) );
	if ( !id || !*id )
	    { wrrs_ += 0; continue; }
	else
	    haveoutput = true;

	PtrMan<IOObj> ioobj = IOM().get( MultiID(id) );
	if ( !ioobj )
	{
	    msg_ = "Cannot find output cube for ";
	    msg_ += pdfnames_.get( ipdf );
	    msg_ += "\nID found is "; msg_ += id; return false;
	}

	wrrs_ += new SeisTrcWriter( ioobj );
    }

    if ( !haveoutput )
	{ msg_ = "No output specified in parameters"; return false; }

    needclass_ = wrrs_[nrpdfs] || wrrs_[nrpdfs+1];
    initstep_ = 0;
    msg_ = "Processing";
    return true;
}


const char* SeisBayesClass::message() const
{
    return msg_;
}


const char* SeisBayesClass::nrDoneText() const
{
    return initstep_ ? "Step" : "Positions handled";
}


od_int64 SeisBayesClass::nrDone() const
{
    return initstep_ ? initstep_ : nrdone_;
}


od_int64 SeisBayesClass::totalNr() const
{
    if ( initstep_ )
	return 4;

    if ( totalnr_ == -2 && !rdrs_.isEmpty() )
    {
	Seis::Bounds* sb = rdrs_[0]->getBounds();
	SeisBayesClass& self = *(const_cast<SeisBayesClass*>(this));
	self.totalnr_ = sb->expectedNrTraces();
	delete sb;
    }
    return totalnr_ < -1 ? -1 : totalnr_;
}


#include "timefun.h"

int SeisBayesClass::nextStep()
{
    if ( initstep_ )
	return (initstep_ == 1 ? getPDFs()
	     : (initstep_ == 2 ? getReaders()
		 	       : getWriters()))
	     ? MoreToDo() : ErrorOccurred();

    int ret = readInpTrcs();
    if ( ret != 1 )
	return ret > 1	? MoreToDo()
	    : (ret == 0	? closeDown() : ErrorOccurred());

    return createOutput() ? MoreToDo() : ErrorOccurred();
}


int SeisBayesClass::closeDown()
{
    cleanUp();
    return Finished();
}


int SeisBayesClass::readInpTrcs()
{
    SeisTrcInfo& ti0 = inptrcs_.get(0)->info();
    int ret = rdrs_[0]->get( ti0 );
    if ( ret != 1 )
    {
	if ( ret < 0 )
	    msg_ = rdrs_[0]->errMsg();
	return ret;
    }

    for ( int idx=0; idx<rdrs_.size(); idx++ )
    {
	if ( idx && !rdrs_[idx]->seisTranslator()->goTo( ti0.binid ) )
	    return 2;
	if ( !rdrs_[idx]->get(*inptrcs_.get(idx)) )
	{
	    msg_ = rdrs_[idx]->errMsg();
	    return Executor::ErrorOccurred();
	}
    }

    for ( int idx=0; idx<aprdrs_.size(); idx++ )
    {
	SeisTrcReader* rdr = aprdrs_[idx];
	if ( !rdr ) continue;

	if ( !rdr->seisTranslator()->goTo( ti0.binid ) )
	    return 2;
	if ( !rdr->get(*aptrcs_.get(idx)) )
	{
	    msg_ = rdr->errMsg();
	    return Executor::ErrorOccurred();
	}
    }

    return 1;
}


#define mWrTrc(nr) \
    	wrr = wrrs_[nr]; \
	if ( wrr && !wrr->put(*outtrcs_.get(nr)) ) \
	    { msg_ = wrr->errMsg(); return ErrorOccurred(); }

int SeisBayesClass::createOutput()
{
    const int nrpdfs = pdfs_.size();
    for ( int ipdf=0; ipdf<nrpdfs; ipdf++ )
    {
	if ( needclass_ || wrrs_[ipdf] )
	    calcProbs( ipdf );
	SeisTrcWriter* mWrTrc(ipdf)
    }

    if ( needclass_ )
    {
	calcClass();
	SeisTrcWriter* mWrTrc(nrpdfs)
	mWrTrc(nrpdfs+1)
    }

    nrdone_++;
    return MoreToDo();
}


void SeisBayesClass::prepOutTrc( SeisTrc& trc, bool isch ) const
{
    const SeisTrc& inptrc = *inptrcs_.get( 0 );
    if ( trc.isEmpty() )
    {
	const DataCharacteristics dc( isch ? DataCharacteristics::UI8
					   : DataCharacteristics::F32 );
	trc.data().setComponent( dc, 0 );
	for ( int icomp=0; icomp<inptrc.nrComponents(); icomp++ )
	{
	    if ( icomp < trc.nrComponents() )
		trc.data().setComponent( dc, icomp );
	    else
		trc.data().addComponent( inptrc.size(), dc );
	    trc.reSize( inptrc.size(), false );
	}
    }

    trc.info() = inptrc.info();
}


void SeisBayesClass::calcProbs( int ipdf )
{
    SeisTrc& trc = *outtrcs_.get( ipdf ); prepOutTrc( trc, false );
    const ProbDenFunc& pdf = *pdfs_[ipdf];

    TypeSet<float> inpvals( nrdims_, 0 );
    const float eps = trc.info().sampling.step * 0.0001;
    for ( int icomp=0; icomp<trc.nrComponents(); icomp++ )
    {
	for ( int isamp=0; isamp<trc.size(); isamp++ )
	{
	    for ( int idim0=0; idim0<nrdims_; idim0++ )
	    {
		const int idim = (*pdfxtbls_[ipdf])[idim0];
		const SeisTrc& inptrc = *inptrcs_.get( idim );
		const float z = trc.samplePos( isamp );
		if ( z < inptrc.samplePos(0) - eps )
		    inpvals[idim] = inptrc.get( 0, icomp );
		else if ( z > inptrc.samplePos(inptrc.size()-1) + eps )
		    inpvals[idim] = inptrc.get( inptrc.size()-1, icomp );
		else
		    inpvals[idim] = inptrc.getValue( z, icomp );
	    }
	    trc.set( isamp, pdf.value(inpvals), icomp );
	}
    }
}


void SeisBayesClass::calcClass()
{
    const int nrpdfs = pdfs_.size();
    SeisTrc& clsstrc = *outtrcs_.get( nrpdfs ); prepOutTrc( clsstrc, true );
    SeisTrc& conftrc = *outtrcs_.get( nrpdfs+1 ); prepOutTrc( conftrc, false );

    TypeSet<float> probs( nrpdfs, 0 ); int winner; float conf;
    for ( int icomp=0; icomp<clsstrc.nrComponents(); icomp++ )
    {
	for ( int isamp=0; isamp<clsstrc.size(); isamp++ )
	{
	    for ( int iprob=0; iprob<nrpdfs; iprob++ )
		probs[iprob] = outtrcs_.get(iprob)->get( isamp, icomp );
	    getClass( probs, winner, conf );
	    clsstrc.set( isamp, winner, icomp );
	    conftrc.set( isamp, conf, icomp );
	}
    }
}


void SeisBayesClass::getClass( const TypeSet<float>& probs, int& winner,
				float& conf ) const
{
    // users don't like class '0', so we add '1' to winner
    if ( probs.size() < 2 )
	{ conf = winner = 1; return; }

    winner = 0; float winnerval = probs[0];
    for ( int idx=1; idx<probs.size(); idx++ )
    {
	if ( probs[idx] > winnerval )
	    { winner = idx; winnerval = probs[idx]; }
    }
    if ( winnerval < mDefEps )
	{ conf = 0; winner++; return; }

    float runnerupval = probs[winner ? 0 : 1];
    for ( int idx=0; idx<probs.size(); idx++ )
    {
	if ( idx == winner ) continue;
	if ( probs[idx] > runnerupval )
	    runnerupval = probs[idx];
    }

    winner++;
    conf = (winnerval - runnerupval) / winnerval;
}
