/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Nov 2009
 RCS:           $Id: waveletattrib.cc,v 1.3 2009-11-18 15:39:07 cvsbruno Exp $
________________________________________________________________________

-*/

#include "waveletattrib.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "fft.h"
#include "hilberttransform.h"
#include "wavelet.h"


WaveletAttrib::WaveletAttrib( const Wavelet& wvlt )
    	: wvltsz_(wvlt.size())
	, wvltarr_(new Array1DImpl<float>(wvltsz_))
	, fft_(new FFT(false))
	, hilbert_(new HilbertTransform())
{
    memcpy( wvltarr_->getData(), wvlt.samples(), wvltsz_*sizeof(float) );
}


WaveletAttrib::~WaveletAttrib()
{
    delete wvltarr_;
    delete fft_;
    delete hilbert_;
}


#define mDoTransform( transfm, isforward, inp, outp,sz )\
{\
    transfm->setInputInfo( Array1DInfoImpl(sz) );\
    transfm->setDir( isforward );\
    transfm->init();\
    transfm->transform(inp,outp);\
}


void WaveletAttrib::getHilbert(Array1DImpl<float>& hilb )
{
    hilbert_->setCalcRange( 0, wvltsz_, 0 );
    mDoTransform( hilbert_, true, *wvltarr_, hilb, wvltsz_ );
} 


void WaveletAttrib::getPhase( Array1DImpl<float>& phase )
{
    Array1DImpl<float> hilb( wvltsz_ );
    getHilbert( hilb );
    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	float ph = 0;
	if ( !mIsZero(wvltarr_->get(idx),mDefEps)  )
		ph = atan2( hilb.get(idx), wvltarr_->get(idx) );
	phase.set( idx, ph );
    }
}


void WaveletAttrib::muteZeroFrequency( Array1DImpl<float>& vals )
{
    const int arraysz = vals.info().getSize(0);
    Array1DImpl<float_complex> cvals( arraysz ), tmparr( arraysz );

    for ( int idx=0; idx<arraysz; idx++ )
	cvals.set( idx, vals.get(idx) );

    mDoTransform( fft_, true, cvals, tmparr, arraysz );
    tmparr.set(0, 0 );
    mDoTransform( fft_, false, tmparr, cvals,  arraysz );

    for ( int idx=0; idx<arraysz; idx++ )
	vals.set( idx, cvals.get(idx).real() );
}


void WaveletAttrib::getFrequency( Array1DImpl<float>& padfreq, bool ispadding )
{
    const int padfac = ispadding ? 3 : 1;
    const int zpadsz = padfac*wvltsz_;

    Array1DImpl<float_complex> czeropaddedarr( zpadsz ), cfreqarr( zpadsz );

    for ( int idx=0; idx<zpadsz; idx++ )
	czeropaddedarr.set( idx, 0 );

    for ( int idx=0; idx<wvltsz_; idx++ )
	czeropaddedarr.set( ispadding ? idx+wvltsz_:idx, wvltarr_->get(idx) );

    mDoTransform( fft_, true, czeropaddedarr, cfreqarr, zpadsz );

    for ( int idx=0; idx<zpadsz; idx++ )
    {
	float fq = abs( cfreqarr.get(idx) );
	padfreq.set( zpadsz-idx-1, fq );
    }
}


void WaveletAttrib::getWvltFromFrequency( const Array1DImpl<float>& freqdata,
       					  Array1DImpl<float>& timedata )
{
    Array1DImpl<float_complex> cfreqarr(wvltsz_), ctimearr(wvltsz_);
    for ( int idx=0; idx<wvltsz_; idx++ )
	cfreqarr.set( idx, freqdata.get(idx) );

    mDoTransform( fft_, false, cfreqarr, ctimearr, wvltsz_ );

    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	float val = abs( cfreqarr.get(idx) );
	timedata.set( wvltsz_-idx-1, val );
    }
} 
