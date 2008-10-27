#ifndef uistatsdisplay_h
#define uistatsdisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra / Bert
 Date:          Aug 2007
 RCS:           $Id: uistatsdisplay.h,v 1.9 2008-10-27 11:13:07 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "datapack.h"
class uiFunctionDisplay;
class uiGenInput;
template <class T> class Array2D;
namespace Stats { template <class T> class RunCalc; }


class uiStatsDisplay : public uiGroup
{
public:

    struct Setup
    {
				Setup()
				    : withplot_(true)
				    , withtext_(true)
				    , vertaxis_(true)
				    , countinplot_(false)	{}

	mDefSetupMemb(bool,withplot)
	mDefSetupMemb(bool,withtext)
	mDefSetupMemb(bool,vertaxis)
	mDefSetupMemb(bool,countinplot)
    };
				uiStatsDisplay(uiParent*,const Setup&);

    bool                        setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const float*,int sz);
    void			setData(const Array2D<float>*);
    void			setData(const Stats::RunCalc<float>&);

    uiFunctionDisplay*		funcDisp()	  { return funcdisp_; }
    void			setMarkValue(float,bool forx);

    void			setHistogram(const TypeSet<float>&,
	    				     Interval<float>,int N=-1);
    				//!< Cannot update the text part if you have it
    				//!< N is the total count of the input data

    int				nrInpVals() const	{ return nrinpvals_; }
    int				nrClasses() const	{ return nrclasses_; }
    void			putN();

protected:

    uiFunctionDisplay*		funcdisp_;
    uiGenInput*			countfld_;
    uiGenInput*			minmaxfld_;
    uiGenInput*			avgstdfld_;
    uiGenInput*			medrmsfld_;

    const Setup			setup_;
    int				nrclasses_;
    int				nrinpvals_;

    void			updateHistogram(const Stats::RunCalc<float>&);
};


#endif
