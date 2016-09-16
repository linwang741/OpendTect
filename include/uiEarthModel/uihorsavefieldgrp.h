#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Y. Liu
 Date:          Nov 2009
________________________________________________________________________

-*/


#include "uiearthmodelmod.h"
#include "uigroup.h"

namespace EM { class Horizon; }

class uiCheckBox;
class uiGenInput;
class uiIOObjSel;

/*!\brief save or overwrite horizon field set up. It will create new horizon
    based on given horizon, if the old horizon is not given, you can read it
    from memory. You can also call saveHorizon() to save horizon based on your
    choice of as new or overwrite. */

mExpClass(uiEarthModel) uiHorSaveFieldGrp : public uiGroup
{ mODTextTranslationClass(uiHorSaveFieldGrp);
public:
				uiHorSaveFieldGrp(uiParent*,EM::Horizon*,
						  bool is2d=false);
				~uiHorSaveFieldGrp();

    void			setSaveFieldName(const char*);
    bool			displayNewHorizon() const;
    bool			overwriteHorizon() const;
    void			allowOverWrite(bool);
    EM::Horizon*		getNewHorizon() const	{ return newhorizon_; }

    EM::Horizon*		readHorizon(const DBKey&);
    bool			saveHorizon();

    void			setHorRange(const Interval<int>& newinlrg,
					    const Interval<int>& newcrlrg);
    void			setFullSurveyArray(bool yn);
    bool			needsFullSurveyArray() const;
    bool			acceptOK();

protected:

    uiGenInput*			savefld_;
    uiCheckBox*			addnewfld_;
    uiIOObjSel*			outputfld_;

    EM::Horizon*		horizon_;
    EM::Horizon*		newhorizon_;
    bool			usefullsurvey_;
    bool			is2d_;

    bool			createNewHorizon();
    void			saveCB(CallBacker*);
    void			expandToFullSurveyArray();
};
