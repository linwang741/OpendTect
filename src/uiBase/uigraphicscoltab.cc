/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/


#include "uigraphicscoltab.h"

#include "uigraphicsitemimpl.h"
#include "uipixmap.h"

#include "coltabmapper.h"
#include "coltabsequence.h"

uiColTabItem::uiColTabItem( const uiColTabItem::Setup& su )
    : uiGraphicsItem()
    , setup_(su)
{
    ctseqitm_ = new uiPixmapItem();
    borderitm_ = new uiRectItem();
    minvalitm_ = new uiAdvancedTextItem( toUiString("0") );
    maxvalitm_ = new uiAdvancedTextItem( toUiString("1") );
    addChild( borderitm_ );
    addChild( ctseqitm_ );
    addChild( minvalitm_ );
    addChild( maxvalitm_ );

    setSequence( *ColTab::SeqMGR().getDefault() );
    setMapperSetup( *new ColTab::MapperSetup );

    uiRect boundrec = ctseqitm_->boundingRect();
    ctseqitm_->setPos( 0.f, 0.f );
    borderitm_->setRect( -1, -1, boundrec.width()+1, boundrec.height()+1 );

    adjustLabel();
}


uiColTabItem::~uiColTabItem()
{
    removeAll( true );
}


void uiColTabItem::adjustLabel()
{
    const uiRect rect( 0, 0, setup_.sz_.width(), setup_.sz_.height() );
    OD::Alignment al;

    if ( setup_.hor_ )
    {
	const int starty = setup_.startal_.vPos() == OD::Alignment::VCenter
	    ? rect.centre().y_
	    : (setup_.startal_.vPos() == OD::Alignment::Top
	       ? rect.top()
	       : rect.bottom());

	al = OD::Alignment( setup_.startal_.hPos(),
			OD::Alignment::opposite(setup_.startal_.vPos()) );
	minvalitm_->setAlignment( al );
	minvalitm_->setPos( mCast(float,rect.left()), mCast(float,starty) );

	const int stopy =
	    setup_.stopal_.vPos() == OD::Alignment::VCenter
		? rect.centre().y_
		: (setup_.stopal_.vPos() == OD::Alignment::Top
		   ? rect.top()
		   : rect.bottom());
	al = OD::Alignment( setup_.stopal_.hPos(),
			OD::Alignment::opposite(setup_.stopal_.vPos()) );
	maxvalitm_->setAlignment( al );
	maxvalitm_->setPos( mCast(float,rect.right()), mCast(float,stopy) );
    }
    else
    {
	const int startx = setup_.startal_.hPos() == OD::Alignment::HCenter
	    ? rect.centre().x_
	    : (setup_.startal_.hPos() == OD::Alignment::Left
		? rect.left()
		: rect.right());
	al = OD::Alignment( OD::Alignment::opposite(setup_.startal_.hPos()),
			setup_.startal_.vPos() );
	minvalitm_->setAlignment( al );
	minvalitm_->setPos( mCast(float,startx), mCast(float,rect.top()) );

	const int stopx = setup_.stopal_.hPos() == OD::Alignment::HCenter
	    ? rect.centre().x_
	    : (setup_.stopal_.hPos() == OD::Alignment::Left
		? rect.left()
		: rect.right());
	al = OD::Alignment( OD::Alignment::opposite(setup_.stopal_.hPos()),
			setup_.stopal_.vPos() );
	maxvalitm_->setAlignment( al );
	maxvalitm_->setPos( mCast(float,stopx), mCast(float,rect.bottom()) );
    }
}


void uiColTabItem::setSetup( const Setup& su )
{
    setup_ = su;
    setPixmap();
    adjustLabel();
}


void uiColTabItem::setSeqName( const char* nm )
{
    setSequence( *ColTab::SeqMGR().getAny(nm) );
}


void uiColTabItem::setSequence( const ColTab::Sequence& ctseq )
{
    if ( ctseq_.ptr() == &ctseq )
	return;

    if ( ctseq_ )
	mDetachCB( ctseq_->objectChanged(), uiColTabItem::seqChgCB );
    ctseq_ = &ctseq;

    seqChgCB( 0 );
    mAttachCB( ctseq_->objectChanged(), uiColTabItem::seqChgCB );
}


void uiColTabItem::seqChgCB( CallBacker* )
{
    setPixmap();
}


void uiColTabItem::setPixmap()
{
    uiPixmap pm( setup_.sz_.hNrPics(), setup_.sz_.vNrPics() );
    pm.fill( *ctseq_, setup_.hor_ );
    ctseqitm_->setPixmap( pm );
}


void uiColTabItem::setMapperSetup( const ColTab::MapperSetup& ms )
{
    if ( ctmsu_.ptr() == &ms )
	return;

    if ( ctmsu_ )
	mDetachCB( ctmsu_->objectChanged(), uiColTabItem::mapperChgCB );
    ctmsu_ = &ms;

    mapperChgCB( 0 );
    mAttachCB( ctmsu_->objectChanged(), uiColTabItem::mapperChgCB );
}

void uiColTabItem::mapperChgCB( CallBacker* )
{
    if ( !ctmsu_ )
	return;

    BufferString precision;
    const Interval<float> rg = ctmsu_->range();
    minvalitm_->setPlainText( toUiString(precision.set(rg.start,2)) );
    maxvalitm_->setPlainText( toUiString(precision.set(rg.stop,2)) );

    adjustLabel();
}
