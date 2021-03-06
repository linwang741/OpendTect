#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert/Nanne
 Date:          Aug 2007
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "factory.h"
#include "fixedstring.h"
#include "uidlggroup.h"

class ZAxisTransform;
class uiGenInput;
class uiZAxisTransformSel;

/*! Base class for ZAxisTransform ui's*/

mExpClass(uiTools) uiZAxisTransform : public uiDlgGroup
{ mODTextTranslationClass(uiZAxisTransform)
public:
    mDefineFactory3ParamInClass(uiZAxisTransform,uiParent*,
				const char*,const char*,factory)

    virtual void		enableTargetSampling();
    virtual bool		getTargetSampling(StepInterval<float>&) const;

    virtual ZAxisTransform*	getSelection()			= 0;
    virtual FixedString 	toDomain() const		= 0;
    virtual FixedString 	fromDomain() const		= 0;
    virtual bool		canBeField() const		= 0;
				/*!Returns true if it can be in one line,
				   i.e. as a part of a field. If true,
				   object should look at
				   uiZAxisTransformSel::isField() at
				   construction.
				 */

    virtual void		setIs2D(bool);
    virtual bool		is2D() const;

protected:
    static bool 		isField(const uiParent*);
				uiZAxisTransform(uiParent*);
    void			rangeChangedCB(CallBacker*);
    void			finalizeDoneCB(CallBacker*);

    uiGenInput* 		rangefld_;
    bool			rangechanged_;
    bool			is2d_;
};


/*!Selects a ZAxisTransform. */
mExpClass(uiTools) uiZAxisTransformSel : public uiDlgGroup
{ mODTextTranslationClass(uiZAxisTransformSel)
public:
				uiZAxisTransformSel(uiParent*, bool withnone,
						    const char* fromdomain=0,
						    const char* todomain=0,
						    bool withsampling=false,
						    bool asfield=false,
						    bool is2d=false);

    bool			isField() const;
				/*!<If true, the shape will be a one-line
				    group, with label, combobox and transform
				    settings */
    void			setLabel(const uiString&);

    bool			isOK() const { return nrTransforms(); }
    int				nrTransforms() const;

    NotifierAccess*		selectionDone();
    FixedString 		selectedToDomain() const;
				/*<!Always available. */

    bool			acceptOK();
				/*!<Checks that all input is OK. After that
				    the getSelection will return something. */
    ZAxisTransform*		getSelection();
				/*!<Only after successful acceptOK() */

    bool			getTargetSampling(StepInterval<float>&) const;
				/*!<Only after successful acceptOK and only if
				    withsampling was specified in constructor*/

    bool			fillPar(IOPar&);
				/*!<Only after successful acceptOK() */

protected:
    void			selCB(CallBacker*);

    bool			isfield_;
    BufferString		fromdomain_;
    uiGenInput*			selfld_;
    ObjectSet<uiZAxisTransform>	transflds_;
};
