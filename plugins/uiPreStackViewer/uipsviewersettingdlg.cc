/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          5-11-2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipsviewersettingdlg.h"

#include "uibutton.h"
#include "uipsviewerscalingtab.h"
#include "uipsviewerappearancetab.h"
#include "uipsviewershapetab.h"
#include "uipsviewerpreproctab.h"
#include "visflatviewer.h"
#include "visprestackdisplay.h"
#include "prestackprocessor.h"
#include "od_helpids.h"

namespace PreStackView
{


uiViewer3DSettingDlg::uiViewer3DSettingDlg( uiParent* p, 
	visSurvey::PreStackDisplay& viewer, uiViewer3DMgr& mgr )
    : uiTabStackDlg( p, uiDialog::Setup("Prestack display properties", 
			          viewer.getObjectName(), 
                                  mODHelpKey(mPSViewerSettingDlgHelpID) 
                                  ).modal(false) ) 
    , preproctab_(0)
{
    shapetab_ = new uiViewer3DShapeTab( tabParent(), viewer, mgr );
    addGroup( shapetab_ );

    apptab_ = new uiViewer3DAppearanceTab( tabParent(), viewer, mgr );
    addGroup( apptab_ );

    scaletab_ = new uiViewer3DScalingTab( tabParent(), viewer, mgr );
    addGroup( scaletab_ );

    preproctab_ = new uiViewer3DPreProcTab( tabParent(), viewer, mgr );
    addGroup( preproctab_ );
   
    applytoallfld_ =
	new uiCheckBox( this,"&Apply to all viewers",
			mCB(this,uiViewer3DSettingDlg,applyCheckedCB) );
    applytoallfld_->attach( centeredBelow, tabObject() );
    
    enableSaveButton( sSaveAsDefault() );
}


void uiViewer3DSettingDlg::applyCheckedCB( CallBacker* cb )
{
   const bool ischecked = applytoallfld_->isChecked();
   apptab_->applyToAll( ischecked );
   scaletab_->applyToAll( ischecked );
   if ( preproctab_ )
       preproctab_->applyToAll( ischecked );
   shapetab_->applyToAll( ischecked );
}


bool uiViewer3DSettingDlg::acceptOK( CallBacker* cb )
{
   if ( saveButtonChecked() )
   {
       apptab_->saveAsDefault( true );
       scaletab_->saveAsDefault( true );
       shapetab_->saveAsDefault( true );
   }

   return uiTabStackDlg::acceptOK(cb);
}


}; //namespace

