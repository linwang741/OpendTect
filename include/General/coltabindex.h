#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2007
________________________________________________________________________

-*/

#include "coltab.h"
#include "typeset.h"
#include "uistring.h"


namespace ColTab
{
class Mapper;
class Sequence;

/*!\brief Looks up color for certain value. Keeps a pre-calc list of colors.

  Note that sequence and (optional) mapper need to stay alive; no copy is made.

 */

mExpClass(General) IndexedLookUpTable
{ mODTextTranslationClass(IndexedLookUpTable);
public:

			IndexedLookUpTable(const Sequence&,int nrcols,
					    const Mapper&);
			IndexedLookUpTable(const Sequence&,int nrcols,
					   SeqUseMode);

    void		update();
			//!< Call when sequence, mapper, or nr cols changed

    inline Color	color( float v ) const
			{ return colorForIndex( indexForValue(v) ); }
    int			indexForValue(float) const;
    Color		colorForIndex(int) const;

    void		setMapper( const Mapper* m )	{ mapper_ = m; }
    void		setNrCols( int n )		{ nrcols_ = n; }
    int			nrCols() const			{ return nrcols_; }
    SeqUseMode		seqUseMode() const		{ return mode_; }

protected:

    const Sequence&	seq_;
    const Mapper*	mapper_;
    int			nrcols_;
    SeqUseMode		mode_;
    TypeSet<Color>	cols_;

    friend class	Indexer;

};

} // namespace ColTab
