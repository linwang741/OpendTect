/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/05/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uilineedit.cc,v 1.39 2010-11-18 17:20:11 cvsjaap Exp $";

#include "uilineedit.h"
#include "i_qlineedit.h"

#include "datainpspec.h"
#include "mouseevent.h"
#include "uibody.h"
#include "uiobjbody.h"
#include "uivirtualkeyboard.h"

#include <QSize> 
#include <QCompleter>
#include <QContextMenuEvent>
#include <QIntValidator>
#include <QDoubleValidator>

class uiLineEditBody : public uiObjBodyImpl<uiLineEdit,QLineEdit>
{
public:

                        uiLineEditBody( uiLineEdit& handle,
				   uiParent*, const char* nm="Line Edit body");

    virtual		~uiLineEditBody()		{ delete &messenger_; }

    virtual int 	nrTxtLines() const		{ return 1; }

protected:

    virtual void	contextMenuEvent(QContextMenuEvent*);

private:

    i_lineEditMessenger& messenger_;

};


uiLineEditBody::uiLineEditBody( uiLineEdit& handle,uiParent* parnt, 
				const char* nm )
    : uiObjBodyImpl<uiLineEdit,QLineEdit>(handle,parnt,nm)
    , messenger_ ( *new i_lineEditMessenger(this,&handle) )
{ 
    setStretch( 1, 0 ); 
    setHSzPol( uiObject::Medium );
}


void uiLineEditBody::contextMenuEvent( QContextMenuEvent* ev )
{ handle().popupVirtualKeyboard( ev->globalX(), ev->globalY() ); }


//------------------------------------------------------------------------------


uiLineEdit::uiLineEdit( uiParent* parnt, const DataInpSpec& spec,
			const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , editingFinished(this), returnPressed(this)
    , selectionChanged(this), textChanged(this)
    , UserInputObjImpl<const char*>()
{
    setText( spec.text() );
}


uiLineEdit::uiLineEdit( uiParent* parnt, const char* nm ) 
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , editingFinished(this), returnPressed(this)
    , selectionChanged(this), textChanged(this)
    , UserInputObjImpl<const char*>()
{
    setText( "" );
}


uiLineEditBody& uiLineEdit::mkbody( uiParent* parnt, const char* nm )
{ 
    body_ = new uiLineEditBody(*this,parnt,nm);
    return *body_; 
}


const char* uiLineEdit::getvalue_() const
{
    BufferString res( mQStringToConstChar( body_->text() ) );
    char* ptr = res.buf();
    mTrimBlanks(ptr);
    result_ = ptr;
    return result_.buf();
}


void uiLineEdit::setvalue_( const char* t )
{
    body_->setText( mIsUdf(t) ? QString() : QString(t) );
    body_->setCursorPosition( 0 );
    setEdited( false );
}


void uiLineEdit::setPasswordMode()
{
    body_->setEchoMode( QLineEdit::Password );
}


void uiLineEdit::setValidator( const uiIntValidator& val )
{
    body_->setValidator( new QIntValidator(val.bottom_,val.top_,body_) );
}


void uiLineEdit::setValidator( const uiFloatValidator& val )
{
    QDoubleValidator* qdval =
	new QDoubleValidator( val.bottom_, val.top_, val.nrdecimals_, body_ );
    if ( !val.scnotation_ )
	qdval->setNotation( QDoubleValidator::StandardNotation );
    body_->setValidator( qdval );
}


void uiLineEdit::setMaxLength( int maxtxtlength )
{ body_->setMaxLength( maxtxtlength ); }

int uiLineEdit::maxLength() const
{ return body_->maxLength(); }

void uiLineEdit::setEdited( bool yn )
{ body_->setModified( yn ); }

bool uiLineEdit::isEdited() const
{ return body_->isModified(); }

void uiLineEdit::setCompleter( const BufferStringSet& bs, bool cs )
{
    QStringList qsl;
    for ( int idx=0; idx<bs.size(); idx++ )
	qsl << QString( bs.get(idx) );

    QCompleter* qc = new QCompleter( qsl, 0 );
    qc->setCaseSensitivity( cs ? Qt::CaseSensitive : Qt::CaseInsensitive );
    body_->setCompleter( qc );
}


void uiLineEdit::setReadOnly( bool yn )
{ body_->setReadOnly( yn ); }

bool uiLineEdit::isReadOnly() const
{ return body_->isReadOnly(); }

bool uiLineEdit::update_( const DataInpSpec& spec )
{ setText( spec.text() ); return true; }

void uiLineEdit::home()
{ body_->home( false ); }

void uiLineEdit::end()
{ body_->end( false ); }

void uiLineEdit::backspace()
{ body_->backspace(); }

void uiLineEdit::del()
{ body_->del(); }

void uiLineEdit::cursorBackward( bool mark, int steps )
{ body_->cursorBackward( mark, steps ); }

void uiLineEdit::cursorForward( bool mark, int steps )
{ body_->cursorForward( mark, steps ); }

int uiLineEdit::cursorPosition() const
{ return body_->cursorPosition(); }

void uiLineEdit::insert( const char* text )
{ body_->insert( text ); }

int uiLineEdit::selectionStart() const
{ return body_->selectionStart(); }

void uiLineEdit::setSelection( int start, int length )
{ body_->setSelection( start, length ); }


const char* uiLineEdit::selectedText() const
{
    result_ = mQStringToConstChar( body_->selectedText() );
    return result_.buf();
}


bool uiLineEdit::handleLongTabletPress()
{
    const Geom::Point2D<int> pos = TabletInfo::currentState()->globalpos_;
    popupVirtualKeyboard( pos.x, pos.y );
    return true;
}


void uiLineEdit::popupVirtualKeyboard( int globalx, int globaly )
{
    mDynamicCastGet( uiVirtualKeyboard*, virkeyboardparent, parent() );

    if ( virkeyboardparent || isReadOnly() || !hasFocus() )
	return;

    uiVirtualKeyboard virkeyboard( *this, globalx, globaly );
    virkeyboard.show();

    if ( virkeyboard.enterPressed() )
	returnPressed.trigger();

    editingFinished.trigger();
}
