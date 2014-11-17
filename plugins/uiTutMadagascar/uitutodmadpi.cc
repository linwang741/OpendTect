/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : H. Huck
 * DATE     : Jun 2009
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uitutodmad.h"
#include "odplugin.h"

#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"


mDefODPluginInfo(uiTutMadagascar)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Madagascar Tutorial plugin",
	"OpendTect",
	"dGB (Helene)",
	"=od",
	"User Interface for Madagascar tutorial plugin."));
    return &retpi;
}


// We need an object to receive the CallBacks, we will thus create a manager
// inheriting from CallBacker.

class uiMadTutMgr :  public CallBacker
{ mODTextTranslationClass(uiMadTutMgr);
    public:

			uiMadTutMgr(uiODMain&);

    uiODMain&           mainappl;
    void                dispDlg(CallBacker*);
};


uiMadTutMgr::uiMadTutMgr( uiODMain& a )
    : mainappl( a )
{
    uiAction* newitem = new uiAction( tr("Display Madagascar data ..."),
					  mCB(this,uiMadTutMgr,dispDlg) );
    mainappl.menuMgr().utilMnu()->insertItem( newitem );
}


void uiMadTutMgr::dispDlg( CallBacker* )
{
    uiTutODMad maddispdlg( &mainappl );
    maddispdlg.go();
}


mDefODInitPlugin(uiTutMadagascar)
{
    (void)new uiMadTutMgr( *ODMainWin() );
    return 0; // All OK - no error messages
}
