/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          January 2002
 RCS:           $Id: uibatchlaunch.cc,v 1.14 2002-06-08 10:19:18 bert Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"
#include "uidistriblaunch.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uiseparator.h"
#include "uibutton.h"
#include "uimsg.h"
#include "ioparlist.h"
#include "strmdata.h"
#include "strmprov.h"
#include "lic.h"
#include "hostdata.h"
#include "filegen.h"


uiGenBatchLaunch::uiGenBatchLaunch( uiParent* p, const UserIDSet& nms )
        : uiDialog(p,uiDialog::Setup("Run batch program",
		   "Specify batch parameters","0.1.5"))
        , prognms(nms)
{
    progfld = new uiLabeledComboBox( this, prognms );

    BufferString dir = File_getFullPath( GetDataDir(), "Proc" );
    parfld = new uiFileInput( this, "Parameter file", dir, true );
    parfld->attach( alignedBelow, progfld );
}


bool uiGenBatchLaunch::acceptOK( CallBacker* )
{
    const char* prognm = progfld->box()->text();
    if ( ! *prognm )
        { uiMSG().error("Please specify the batch program"); return NO; }

    const char* fname = parfld->fileName();
    StreamData sd = StreamProvider( fname ).makeIStream();
    if ( !sd.usable() )
    { 
	sd.close();
	uiMSG().error( "Could not open input file" );
        return false; 
    }
	
    parlist = new IOParList( *sd.istrm );
    sd.close();
    if ( parlist->size() < 1 )
    {
        uiMSG().error( "The parameter file is invalid" );
        delete parlist; parlist = 0;
        return false;
    }

    return true;
}

const char* uiGenBatchLaunch::getProg()
{
    return progfld->box()->text();
}


static bool writeProcFile( const IOParList& iopl, const char* basnm,
			   BufferString& tfname )
{
    tfname = File_getFullPath( GetDataDir(), "Proc" );
    tfname = File_getFullPath( tfname, basnm );
    if ( GetSoftwareUser() )
	{ tfname += "_"; tfname += GetSoftwareUser(); }
    tfname += ".par";
    StreamData sd = StreamProvider(tfname).makeOStream();
    bool allok = sd.usable() && iopl.write(*sd.ostrm);
    sd.close();
    if ( !allok )
    {
	BufferString msg = "Cannot write to:\n"; msg += tfname;
	uiMSG().error( msg );
	return false;
    }

    return true;
}


uiBatchLaunch::uiBatchLaunch( uiParent* p, const IOParList& pl,
			      BufferString& hn, const char* pn, bool wp )
        : uiDialog(p,uiDialog::Setup("Batch launch","Specify output mode",
		   "0.1.4"))
	, iopl(pl)
	, hostname(hn)
	, progname(pn)
	, opts("Output to")
{
    finaliseDone.notify( mCB(this,uiBatchLaunch,remSel) );

    remfld = new uiGenInput( this, "Execute",
			     BoolInpSpec( "Local", "Remote (using rsh)" ) );
    remfld->valuechanged.notify( mCB(this,uiBatchLaunch,remSel) );

    opts.add( "Output window" );
    opts.add( "Log file" );
    opts.add( "Standard output" );
    if ( wp )
	opts.add( "Parameter report (no run)" );
    opts.setCurrent( 0 );
    optfld = new uiLabeledComboBox( this, Ptr(opts) );
    optfld->attach( alignedBelow, remfld );
    optfld->box()->selectionChanged.notify( mCB(this,uiBatchLaunch,optSel) );

    StringListInpSpec spec;
    HostDataList hdl;
    for ( int idx=0; idx<hdl.size(); idx++ )
	spec.addString( hdl[idx]->name() );
    remhostfld = new uiGenInput( this, "Hostname", spec );
    remhostfld->attach( alignedBelow, remfld );

    static BufferString fname = "";
    if ( fname == "" )
    {
	fname = "/tmp/out";
	if ( GetSoftwareUser() )
	    { fname += "_"; fname += GetSoftwareUser(); }
	fname += ".log";
    }
    filefld = new uiFileInput( this, "Output file", fname, false );
    filefld->attach( alignedBelow, optfld );
}


uiBatchLaunch::~uiBatchLaunch()
{
    opts.deepErase();
}


bool uiBatchLaunch::execRemote() const
{
    return !remfld->getBoolValue();
}


void uiBatchLaunch::optSel( CallBacker* )
{
    const int sel = selected();
    filefld->display( sel == 1 || sel == 3 );
}


void uiBatchLaunch::remSel( CallBacker* )
{
    bool isrem = execRemote();
    remhostfld->display( isrem );
    optfld->display( !isrem );
    optSel(0);
}


int uiBatchLaunch::selected()
{
    return execRemote() ? 1 : optfld->box()->currentItem();
}


bool uiBatchLaunch::acceptOK( CallBacker* )
{
    const bool dormt = execRemote();
    if ( execRemote() )
    {
	hostname = remhostfld->text();
	if ( hostname == "" )
	{
	    uiMSG().error( "Please specify the name of the remote host" );
	    return false;
	}
    }

    const int sel = selected();
    BufferString fname = sel == 0 ? "window"
		       : (sel == 2 ? "stdout" : filefld->fileName());
    if ( fname == "" ) fname = "/dev/null";
    IOPar* iop = const_cast<IOPar*>(iopl.size() ? iopl[0] : 0);
    if ( iop )
    {
	iop->set( "Log file", fname );
    	LM().setCert( *iop );
    }

    if ( selected() == 3 )
    {
	if ( iop ) iop->set( "Log file", "stdout" );
        StreamData sd = StreamProvider( fname ).makeOStream();
	if ( !sd.usable() )
        {
	    uiMSG().error( "Cannot open output stream" );
	    sd.close();
	    return false;
	}
	else if ( !iopl.write(*sd.ostrm) )
	{
	    uiMSG().error( "Error during write" );
	    sd.close();
            return false;
	}
	sd.close();
	return true;
    }

    BufferString tfname;
    if ( !writeProcFile(iopl,"batch_processing",tfname) )
	return false;

    BufferString comm( "@" );
    comm += GetSoftwareDir();
    comm = File_getFullPath( comm, "bin" );
    comm = File_getFullPath( comm, "dgb_exec" );
    if ( dormt )
    {
	comm += "_rmt ";
	comm += hostname;
    }
    comm += " "; comm += progname;
    comm += " -bg "; comm += tfname;

    if ( !StreamProvider( comm ).executeCommand(dormt) )
    {
	uiMSG().error( "Cannot start batch program" );
	return false;
    }
    return true;
}


uiFullBatchDialog::uiFullBatchDialog( uiParent* p, const char* ppn,
					const char* t, const char* mpn )
	: uiDialog(p,uiDialog::Setup(t,"X",0)
			.oktext(""))
    	, uppgrp(new uiGroup(this,"Upper group"))
	, procprognm(ppn)
	, multiprognm(mpn)
{
    finaliseStart.notify( mCB(this,uiFullBatchDialog,preFinalise) );
}


static int buttxtpresz = 10;

void uiFullBatchDialog::preFinalise( CallBacker* )
{
    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, uppgrp );

    uiGroup* dogrp = new uiGroup( this, "Start-work buttons" );
    singmachbut = new uiPushButton( dogrp, "Single-machine" );
    singmachbut->activated.notify( mCB(this,uiFullBatchDialog,doButPush) );
    multimachbut = new uiPushButton( dogrp, "Multi-machine" );
    multimachbut->activated.notify( mCB(this,uiFullBatchDialog,doButPush) );
    multimachbut->attach( rightOf, singmachbut );

    uiObject* prev = multimachbut;
    for ( int idx=0; idx<uiDistributedLaunch::nrEnvironments(); idx++ )
    {
	BufferString txt( "Submit to " );
	buttxtpresz = txt.size();
	txt += uiDistributedLaunch::environmentName(idx);
	uiPushButton* but = new uiPushButton( dogrp, txt );
	but->attach( rightOf, prev );
	but->activated.notify( mCB(this,uiFullBatchDialog,doButPush) );
	prev = but;
    }
    dogrp->attach( centeredBelow, sep );
}


void uiFullBatchDialog::doButPush( CallBacker* cb )
{
    if ( !prepareProcessing() ) return;

    IOPar* iopar = new IOPar( "Processing" );
    IOParList iopl; iopl += iopar;
    if ( !fillPar(*iopar) )
	return;

    BufferString tfname;
    if ( cb != singmachbut )
    {
	if ( !writeProcFile(iopl,"cube_processing",tfname) )
	    return;
    }

    bool res = false;
    if ( cb == singmachbut )
	res = singLaunch(iopl);
    else if ( cb == multimachbut )
	res = multiLaunch( tfname );
    else
	res = distrLaunch( cb, tfname );

    if ( res )
	accept(this);
}


bool uiFullBatchDialog::singLaunch( const IOParList& iopl )
{
    BufferString dum;
    uiBatchLaunch dlg( this, iopl, dum, procprognm, false );
    return dlg.go();
}


bool uiFullBatchDialog::multiLaunch( const char* fnm )
{
    BufferString comm( "@" );
    comm += multiprognm;	comm += " ";
    comm += procprognm;		comm += " ";
    comm += fnm;		comm += "&";
    StreamData sd = StreamProvider( comm ).makeOStream();
    if ( !sd.usable() )
	{ uiMSG().error( "Cannot start multi-machine program" ); return false; }

    sd.close();
    return true;
}


bool uiFullBatchDialog::distrLaunch( CallBacker* cb, const char* fnm )
{
    mDynamicCastGet(uiPushButton*,but,cb);
    if ( !but ) { pErrMsg("Huh"); return false; }

    const char* envnm = but->text() + buttxtpresz;
    int envid = 0;
    for ( int idx=0; idx<uiDistributedLaunch::nrEnvironments(); idx++ )
    {
	if ( !strcmp(uiDistributedLaunch::environmentName(idx),envnm) )
	    { envid = idx; break; }
    }

    //TODO set callback and do what's needed
    uiDistributedLaunch dlg( this, procprognm, CallBack(0), envid );
    return dlg.go();
}
