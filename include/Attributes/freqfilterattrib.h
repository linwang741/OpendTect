#ifndef freqfilter_h
#define freqfilter_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          February 2003
 RCS:           $Id: freqfilterattrib.h,v 1.2 2005-07-06 15:02:07 cvshelene Exp $
________________________________________________________________________

-*/


#include "attribprovider.h"
#include "arrayndutils.h"
#include "arrayndimpl.h"
#include "fft.h"
#include <complex>


/*!\brief Frequency filtering Attribute

  FreqFilter type=LowPass,HighPass,BandPass minfreq= maxfreq= nrpoles=
             isfftfilter= window=

Input:                                  ||
0       Real data                       ||0     Real Data
                                        ||1     Imaginary Data
Output:                                 ||
0       Frequency filtered data         ||0     Frequency filtered data
          (Butterworth Filter)          ||           (FFT Filter)

*/

namespace Attrib
{

class FreqFilter: public Provider
{
public:
    static void		initClass();
			FreqFilter(Desc&);

    static const char*  attribName()		{ return "FreqFilter"; }
    static const char*  filtertypeStr()		{ return "type"; }
    static const char*  minfreqStr()    	{ return "minfreq"; }
    static const char*  maxfreqStr()            { return "maxfreq"; }
    static const char*  nrpolesStr()            { return "nrpoles"; }
    static const char*  isfftfilterStr()        { return "isfftfilter"; }
    static const char*  windowStr()             { return "window"; }
    static const char*  filterTypeNamesStr(int);

protected:
    			~FreqFilter();
    static Provider*    createInstance(Desc&);
    static void         updateDesc( Desc& );
    static Provider*    internalCreate(Desc&,ObjectSet<Provider>& existing);

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&, int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples) const;
    void		butterWorthFilter(const DataHolder&, int, int);
    void		fftFilter(const DataHolder&, int, int);

    void 		setSz( int sz );


    int				filtertype;
    float 			minfreq;
    float                       maxfreq;
    int				nrpoles;
    bool			isfftfilter;
    ArrayNDWindow*              window;
    
    ArrayNDWindow::WindowType  	windowtype;
    FFT                         fft;
    FFT                         fftinv;
    int                         fftsz;

    Array1DImpl<float_complex>  signal;
    Array1DImpl<float_complex>  timedomain;
    Array1DImpl<float_complex>  freqdomain;
    Array1DImpl<float_complex>  tmpfreqdomain;
    Array1DImpl<float_complex>  timecplxoutp;
    
    const DataHolder*		redata;
    const DataHolder*		imdata;
};

}; // namespace Attrib

#endif
