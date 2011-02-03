/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uispinbox.cc,v 1.46 2011-02-03 21:38:59 cvskris Exp $";

#include "uispinbox.h"
#include "uilabel.h"

#include "i_qspinbox.h"
#include "mouseevent.h"
#include "uiobjbody.h"
#include "uivirtualkeyboard.h"

#include <QContextMenuEvent>
#include <QLineEdit>
#include <QValidator>
#include <math.h>

class uiSpinBoxBody : public uiObjBodyImpl<uiSpinBox,QDoubleSpinBox>
{
public:
                        uiSpinBoxBody(uiSpinBox&,uiParent*,const char*);
    virtual		~uiSpinBoxBody();

    virtual int 	nrTxtLines() const	{ return 1; }
    void		setNrDecimals(int);
    void		setAlpha(bool yn);
    bool		isAlpha() const		{ return isalpha_; }

    QValidator::State	validate( QString& input, int& pos ) const
			{
			    const double val = input.toDouble();
			    if ( val > maximum() )
				input.setNum( maximum() );
			    return QDoubleSpinBox::validate( input, pos );
			}

    virtual double	valueFromText(const QString&) const;
    virtual QString	textFromValue(double value) const;

protected:
    virtual void	contextMenuEvent(QContextMenuEvent*);


private:

    i_SpinBoxMessenger& messenger_;

    QDoubleValidator*	dval;

    bool		isalpha_;

};


uiSpinBoxBody::uiSpinBoxBody( uiSpinBox& handle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiSpinBox,QDoubleSpinBox>(handle,p,nm)
    , messenger_(*new i_SpinBoxMessenger(this,&handle))
    , dval(new QDoubleValidator(this))
    , isalpha_(false)
{
    setHSzPol( uiObject::Small );
    setCorrectionMode( QAbstractSpinBox::CorrectToNearestValue );
}


uiSpinBoxBody::~uiSpinBoxBody()
{
    delete &messenger_;
    delete dval;
}


static const char* letters[] =
{ "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
  "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", 0 };

void uiSpinBoxBody::setAlpha( bool yn )
{
    isalpha_ = yn;
    setMinimum( 0 );
    setMaximum( specialValueText().isEmpty() ? 25 : 26 );
    setSingleStep( 1 );
}


double uiSpinBoxBody::valueFromText( const QString& text ) const
{
    if ( !specialValueText().isEmpty() && text==specialValueText() )
	return handle_.minFValue();

    QString mytxt = text;
    if ( isalpha_ )
    {
	for ( int idx=0; idx<26; idx++ )
	{
	    if ( text == letters[idx] )
		return (double)idx;
	}

	return 0;
    }

    return (double)(mytxt.toFloat() * handle_.factor_);
}


QString uiSpinBoxBody::textFromValue( double val ) const
{
    if ( !specialValueText().isEmpty() && val==handle_.minFValue() )
	return specialValueText();

    QString str;
    if ( isalpha_ )
    {
	QString svtxt = specialValueText();
	int intval = mNINT(val);
       	if ( !svtxt.isEmpty() )
	    intval--;

	if ( intval < 0 )
	    str = "a";
	else if ( intval > 25 )
	    str = "z";
	else
	    str = letters[intval];
    }
    else
	str.setNum( (float)val / handle_.factor_ );

    return str;
}


void uiSpinBoxBody::setNrDecimals( int dec )
{ dval->setDecimals( dec ); }


void uiSpinBoxBody::contextMenuEvent( QContextMenuEvent* ev )
{ handle().popupVirtualKeyboard( ev->globalX(), ev->globalY() ); }


//------------------------------------------------------------------------------

uiSpinBox::uiSpinBox( uiParent* p, int dec, const char* nm )
    : uiObject(p,nm,mkbody(p,nm))
    , valueChanged(this)
    , valueChanging(this)
    , dosnap_(false)
    , factor_(1)
{
    setNrDecimals( dec );
    setKeyboardTracking( false );
    valueChanged.notify( mCB(this,uiSpinBox,snapToStep) );
    oldvalue_ = getFValue();
}


uiSpinBox::~uiSpinBox()
{
    valueChanged.remove( mCB(this,uiSpinBox,snapToStep) );
}


uiSpinBoxBody& uiSpinBox::mkbody(uiParent* parnt, const char* nm )
{ 
    body_= new uiSpinBoxBody(*this,parnt, nm);
    return *body_; 
}

void uiSpinBox::setSpecialValueText( const char* txt )
{
    body_->setSpecialValueText( txt );
    if ( isAlpha() ) setMaxValue( 26 );
}


void uiSpinBox::setAlpha( bool yn )
{ body_->setAlpha( yn ); }

bool uiSpinBox::isAlpha() const
{ return body_->isAlpha(); }

void uiSpinBox::setNrDecimals( int dec )
{ body_->setDecimals( dec ); }


void uiSpinBox::snapToStep( CallBacker* )
{
    if ( !dosnap_ ) return;

    const double diff = body_->value() - body_->minimum();
    const double step = body_->singleStep();
    const int ratio =  mNINT( diff / step );
    const float newval = minFValue() + ratio * step;
    setValue( newval );
}


void uiSpinBox::setInterval( const StepInterval<int>& intv )
{
    setMinValue( intv.start );
    setMaxValue( intv.stop );
    setStep( intv.step ? intv.step : 1 );
}


void uiSpinBox::setInterval( const StepInterval<float>& intv )
{
    setMinValue( intv.start );
    setMaxValue( intv.stop );
    setStep( intv.step ? intv.step : 1 );
}


StepInterval<int> uiSpinBox::getInterval() const
{
    return StepInterval<int>(minValue(),maxValue(),step());
}


StepInterval<float> uiSpinBox::getFInterval() const
{
    return StepInterval<float>( minFValue(), maxFValue(), fstep() );
}


int uiSpinBox::getValue() const
{ return mNINT(body_->value()); }

float uiSpinBox::getFValue() const	
{ return (float)body_->value(); }


const char* uiSpinBox::text() const
{
    static BufferString res;
    res = mQStringToConstChar( body_->textFromValue(getFValue()) );
    return res;
}


void uiSpinBox::setValue( int val )
{
    if ( mIsUdf(val) )
	val = maxValue();
    body_->setValue( val );
}

void uiSpinBox::setValue( float val )
{
    if ( mIsUdf(val) )
	val = maxFValue();
    body_->setValue( val );
}

void uiSpinBox::setValue( const char* txt )
{ body_->setValue( body_->valueFromText(txt) ); }

void uiSpinBox::setMinValue( int val )
{ body_->setMinimum( val ); }

void uiSpinBox::setMinValue( float val )
{ body_->setMinimum( val ); }

int uiSpinBox::minValue() const
{ return mNINT(body_->minimum()); }

float uiSpinBox::minFValue() const
{ return (float)body_->minimum(); }

void uiSpinBox::setMaxValue( int val )
{ body_->setMaximum( val ); }

void uiSpinBox::setMaxValue( float val )
{ body_->setMaximum( val ); }

int uiSpinBox::maxValue() const
{ return mNINT(body_->maximum()); }

float uiSpinBox::maxFValue() const
{ return (float)body_->maximum(); }

int uiSpinBox::step() const
{ return mNINT(body_->singleStep()); }

float uiSpinBox::fstep() const
{ return (float)body_->singleStep(); }

void uiSpinBox::stepBy( int nrsteps )
{ body_->stepBy( nrsteps ); }

void uiSpinBox::setStep( int step_, bool snapcur )		
{ setStep( (double)step_, snapcur ); }

void uiSpinBox::setStep( float step_, bool snapcur )
{
    if ( !step_ ) step_ = 1;
    body_->setSingleStep( step_ );
    dosnap_ = snapcur;
    snapToStep(0);
}


void uiSpinBox::setPrefix( const char* suffix )
{ body_->setPrefix( suffix ); }


const char* uiSpinBox::prefix() const
{
    static BufferString res;
    res = mQStringToConstChar( body_->prefix() );
    return res;
}


void uiSpinBox::setSuffix( const char* suffix )
{ body_->setSuffix( suffix ); }


const char* uiSpinBox::suffix() const
{
    static BufferString res;
    res = mQStringToConstChar( body_->suffix() );
    return res;
}


void uiSpinBox::setKeyboardTracking( bool yn )
{
#if QT_VERSION >= 0x040300
    body_->setKeyboardTracking( yn );
#endif
}


bool uiSpinBox::keyboardTracking() const
{
#if QT_VERSION < 0x040300
    return false;
#else
    return body_->keyboardTracking();
#endif
}


void uiSpinBox::notifyHandler( bool editingfinished )
{
    BufferString msg = toString( oldvalue_ );
    oldvalue_ = getFValue();
    msg += editingfinished ? " valueChanged" : " valueChanging";
    const int refnr = beginCmdRecEvent( msg );

    if ( editingfinished )
	valueChanged.trigger( this );
    else
	valueChanging.trigger( this );

    endCmdRecEvent( refnr, msg );
}


bool uiSpinBox::handleLongTabletPress()
{
    const Geom::Point2D<int> pos = TabletInfo::currentState()->globalpos_;
    popupVirtualKeyboard( pos.x, pos.y );
    return true;
}


void uiSpinBox::popupVirtualKeyboard( int globalx, int globaly )
{
    if ( !hasFocus() )
	return; 

    uiVirtualKeyboard virkeyboard( *this, globalx, globaly );
    virkeyboard.show();

    if ( virkeyboard.enterPressed() )
	valueChanged.trigger();
}


//------------------------------------------------------------------------------

uiLabeledSpinBox::uiLabeledSpinBox( uiParent* p, const char* txt, int dec,
				    const char* nm )
	: uiGroup(p,"LabeledSpinBox")
{
    sb = new uiSpinBox( this, dec, nm && *nm ? nm : txt );
    BufferString sblbl;
    lbl = new uiLabel( this, txt, sb );
    lbl->setAlignment( Alignment::Right );
    setHAlignObj( sb );
}
