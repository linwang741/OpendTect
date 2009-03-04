/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimsg.cc,v 1.39 2009-03-04 10:45:55 cvsjaap Exp $";


#include "uimsg.h"

#include "mousecursor.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uistatusbar.h"
#include "uiobj.h"
#include "uibody.h"
#include "uiparentbody.h"

#undef Ok
#include <qmessagebox.h>
#include <iostream>

uiMsg* uiMsg::theinst_ = 0;


uiMsg::uiMsg()
	: uimainwin_(0)
{
}

uiMainWin* uiMsg::setMainWin( uiMainWin* m )
{
    uiMainWin* old = uimainwin_;
    uimainwin_ = m;
    return old;
}


uiStatusBar* uiMsg::statusBar()
{
    uiMainWin* mw = uimainwin_;/* ? uimainwin_ 
			       : uiMainWin::gtUiWinIfIsBdy( parent_ );*/

    if ( !mw || !mw->statusBar() )
	mw = uiMainWin::activeWindow();

    if ( !mw || !mw->statusBar() )
	mw = uiMain::theMain().topLevel();

    return mw ? mw->statusBar() : 0;
}


QWidget* uiMsg::popParnt()
{
    uiMainWin* mw = uimainwin_; //Always respect user's setting first.
    if ( !mw ) mw = uiMainWin::activeWindow();
    if ( !mw ) mw = uiMain::theMain().topLevel();

    if ( !mw  )		return 0;
    return mw->body()->qwidget();
}


bool uiMsg::toStatusbar( const char* msg, int fldidx, int msec )
{
    if ( !statusBar() ) return false;

    statusBar()->message( msg, fldidx, msec );
    return true;
}


static BufferString& gtCaptn()
{
    static BufferString* captn = 0;
    if ( !captn )
	captn = new BufferString;
    return *captn;
}

void uiMsg::setNextCaption( const char* s )
{
    gtCaptn() = s;
}


#define mPrepCursor() \
    MouseCursorChanger cc( MouseCursor::Arrow )
#define mPrepTxt() \
    mPrepCursor(); \
    BufferString msg( text ); if ( p2 ) msg += p2; if ( p3 ) msg += p3; \
    if ( msg.isEmpty() ) return
#define mCapt(s) QString( getCaptn(s) )
#define mTxt QString( msg.buf() )

static const char* getCaptn( const char* s )
{
    if ( gtCaptn().isEmpty() )
	return s;

    static BufferString oldcaptn;
    oldcaptn = gtCaptn();
    gtCaptn() = "";

    return oldcaptn.buf();
}


void uiMsg::markCmdRecEvent( int retval, const char* buttxt0,
			     const char* buttxt1, const char* buttxt2 )
{
    uiMainWin* mw = uimainwin_; 
    if ( !mw ) mw = uiMainWin::activeWindow();
    if ( !mw ) mw = uiMain::theMain().topLevel();
    if ( !mw ) return;

    BufferString msg( "QMsgBoxBut " );
    msg += !retval ? buttxt0 : ( retval==1 ? buttxt1 : buttxt2 );

    mw->markCmdRecEvent( false, msg );
}


#define mDeclArgs const char* text, const char* p2, const char* p3

void uiMsg::message( mDeclArgs )
{
    mPrepTxt();
    const char* oktxt = "&Ok";
    QMessageBox::information( popParnt(), mCapt("Information"), mTxt,
	    		      QString(oktxt) );
    markCmdRecEvent( 0, oktxt );
}


void uiMsg::warning( mDeclArgs )
{
    mPrepTxt();
    const char* oktxt = "&Ok";
    QMessageBox::warning( popParnt(), mCapt("Warning"), mTxt, QString(oktxt) );
    markCmdRecEvent( 0, oktxt );
}


void uiMsg::error( mDeclArgs )
{
    mPrepTxt();
    const char* oktxt = "&Ok";
    QMessageBox::critical( popParnt(), mCapt("Error"), mTxt, QString(oktxt) );
    markCmdRecEvent( 0, oktxt );
}


int uiMsg::notSaved( const char* text, bool cancelbutt )
{
    mPrepCursor();
    const char* savetxt = "&Save";
    const char* dontsavetxt = "&Don't save";
    const char* canceltxt = "&Cancel";

#if QT_VERSION < 0x030200
# define mQuestion information
#else
# define mQuestion question
#endif
    const int res = QMessageBox::mQuestion( popParnt(),
				mCapt("Data not saved"), QString(text),
				QString(savetxt),
				QString(dontsavetxt),
				cancelbutt ? QString(canceltxt) : QString::null,
			       	0, 2 );

    markCmdRecEvent( res, savetxt, dontsavetxt, canceltxt );
    return res == 0 ? 1 : (res == 1 ? 0 : -1);
}


void uiMsg::about( const char* text )
{
    mPrepCursor();
    QMessageBox::about( popParnt(), mCapt("About"), QString(text) );
    markCmdRecEvent( 0, "OK" );
}


bool uiMsg::askGoOn( const char* text, bool yn )
{
    mPrepCursor();

    const char* oktxt = yn ? "&Yes" : "&Ok";
    const char* canceltxt = yn ? "&No" : "&Cancel";

    const int res = QMessageBox::warning( popParnt(),
				mCapt("Please specify"), QString(text),
				QString(oktxt),
				QString(canceltxt),
				QString::null, 0, 1 );

    markCmdRecEvent( res, oktxt,canceltxt );
    return !res;
}


int uiMsg::askGoOnAfter( const char* text, const char* cnclmsg ,
			 const char* textyes, const char* textno )
{
    mPrepCursor();
    if ( !cnclmsg || !*cnclmsg )
	cnclmsg = "&Cancel";
    if ( !textyes || !*textyes )
	textyes = "&Yes";
    if ( !textno || !*textno )
	textno = "&No";

    const int res = QMessageBox::warning( popParnt(),
				mCapt("Please specify"), QString(text),
				QString(textyes),
				QString(textno),
				QString(cnclmsg), 0, 2 );

    markCmdRecEvent( res, textyes, textno, cnclmsg );
    return res;
}


bool uiMsg::showMsgNextTime( const char* text, const char* ntmsg )
{
    mPrepCursor();
    const char* oktxt = "&Ok";
    if ( !ntmsg || !*ntmsg )
	ntmsg = "&Don't show this message again";

    const int res = QMessageBox::warning( popParnt(),
				mCapt("Information"), QString(text),
				QString(oktxt),
				QString(ntmsg),
				QString::null, 0, 1 );

    markCmdRecEvent( res, oktxt, ntmsg );
    return !res;
}
