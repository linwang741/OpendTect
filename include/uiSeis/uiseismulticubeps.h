#ifndef uiseismulticubeps_h
#define uiseismulticubeps_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uiseismulticubeps.h,v 1.2 2008-09-04 14:06:42 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiGenInput;
class uiListBox;
class uiListBox;
class uiIOObjSel;
class IOObj;
class CtxtIOObj;
class uiSeisMultiCubePSEntry;


class uiSeisMultiCubePS : public uiDialog
{

public:
                        uiSeisMultiCubePS(uiParent*);
                        ~uiSeisMultiCubePS();

    const IOObj*	createdIOObj() const;

protected:

    CtxtIOObj&		ctio_;
    ObjectSet<uiSeisMultiCubePSEntry>	entries_;
    ObjectSet<uiSeisMultiCubePSEntry>	selentries_;
    int			curselidx_;

    uiListBox*		cubefld_;
    uiListBox*		selfld_;
    uiGenInput*		offsfld_;
    uiIOObjSel*		outfld_;

    void		fillEntries();
    void		fillBox(uiListBox*);
    void		recordEntryOffs();
    void		fullUpdate();

    void		selChg(CallBacker*);
    void		addCube(CallBacker*);
    void		rmCube(CallBacker*);
    bool		acceptOK(CallBacker*);
};

#endif
