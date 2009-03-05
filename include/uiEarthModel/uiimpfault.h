#ifndef uiimpfault_h
#define uiimpfault_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimpfault.h,v 1.9 2009-03-05 06:37:21 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiTableImpDataSel;

namespace EM { class Fault3D;class Fault; }
namespace Table { class FormatDesc; }

/*! \brief Dialog for fault import */

mClass uiImportFault : public uiDialog
{
public:
			~uiImportFault();

protected:
			uiImportFault(uiParent*,const char*);

    void		createUI();
    void		typeSel(CallBacker*);
    bool		checkInpFlds();
    bool		handleAscii();
    bool		handleLMKAscii();
    virtual bool	acceptOK(CallBacker*) { return false; }
    virtual bool	getFromAscIO(std::istream&,EM::Fault&);
    EM::Fault*		createFault() const;

    uiFileInput*	infld_;
    uiFileInput*	formatfld_;
    uiGenInput*		typefld_;
    uiIOObjSel*		outfld_;
    CtxtIOObj&		ctio_;
    Table::FormatDesc*	fd_;
    uiTableImpDataSel*	dataselfld_;
    bool		isfss_;
    const char*		type_;
};


mClass uiImportFault3D : public uiImportFault
{
public:
    			uiImportFault3D(uiParent*,const char* type);
protected:
    bool		acceptOK(CallBacker*);			
};

#endif
