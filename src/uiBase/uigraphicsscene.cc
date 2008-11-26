/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicsscene.cc,v 1.12 2008-11-26 06:13:35 cvssatyaki Exp $";


#include "uigraphicsscene.h"

#include "draw.h"
#include "odgraphicsitem.h"
#include "uigraphicsitemimpl.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPoint>

#include <math.h>


class ODGraphicsScene : public QGraphicsScene
{
public:
    			ODGraphicsScene( uiGraphicsScene& scene )
			    : uiscene_(scene)
			    , bgopaque_(false)
			    , mousepressedbs_(OD::NoButton)
			    , startpos_( *new QPoint() )	{}

    void		setBackgroundOpaque( bool yn )	{ bgopaque_ = yn; }
protected:
    virtual void	mouseMoveEvent(QGraphicsSceneMouseEvent*);
    virtual void	mousePressEvent(QGraphicsSceneMouseEvent*);
    virtual void	mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    virtual void	mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);

    virtual void	drawBackground(QPainter*,const QRectF&);
    OD::ButtonState	mousepressedbs_;

private:

    bool		bgopaque_;

    QPoint&		startpos_;
    uiGraphicsScene&	uiscene_;
};


void ODGraphicsScene::drawBackground( QPainter* painter, const QRectF& rect )
{
    painter->setBackgroundMode( bgopaque_ ? Qt::OpaqueMode 
	    				  : Qt::TransparentMode );
    QGraphicsScene::drawBackground( painter, rect );
}


void ODGraphicsScene::mouseMoveEvent( QGraphicsSceneMouseEvent* qev )
{
    MouseEvent mev( mousepressedbs_, (int)qev->scenePos().x(),
	    	    (int)qev->scenePos().y() );
    uiscene_.getMouseEventHandler().triggerMovement( mev );
    QGraphicsScene::mouseMoveEvent( qev );
}


void ODGraphicsScene::mousePressEvent( QGraphicsSceneMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    if ( bs == OD::LeftButton )
	startpos_ = QPoint( (int)qev->scenePos().x(),(int)qev->scenePos().y() );
    mousepressedbs_ = bs;
    MouseEvent mev( bs, (int)qev->scenePos().x(), (int)qev->scenePos().y() );
    uiscene_.getMouseEventHandler().triggerButtonPressed( mev );
    QGraphicsScene::mousePressEvent( qev );
}


void ODGraphicsScene::mouseReleaseEvent( QGraphicsSceneMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    mousepressedbs_ = OD::NoButton;
    if ( bs == OD::LeftButton )
    {
	const QPoint& stoppos = QPoint( (int)qev->scenePos().x(),
					(int)qev->scenePos().y() );
	const QRectF selrect( startpos_, stoppos );
	QPainterPath selareapath;
	selareapath.addRect( selrect );
	setSelectionArea( selareapath );
    }
    MouseEvent mev( bs, (int)qev->scenePos().x(), (int)qev->scenePos().y() );
    uiscene_.getMouseEventHandler().triggerButtonReleased( mev );
    QGraphicsScene::mouseReleaseEvent( qev );
}


void ODGraphicsScene::mouseDoubleClickEvent( QGraphicsSceneMouseEvent* qev )
{
    OD::ButtonState bs = OD::ButtonState( qev->modifiers() | qev->button() );
    MouseEvent mev( bs, (int)qev->scenePos().x(), (int)qev->scenePos().y() );
    uiscene_.getMouseEventHandler().triggerDoubleClick( mev );
    QGraphicsScene::mouseDoubleClickEvent( qev );
}



uiGraphicsScene::uiGraphicsScene( const char* nm )
    : NamedObject(nm)
    , mousehandler_(MouseEventHandler())
    , odgraphicsscene_(new ODGraphicsScene(*this))
{
    odgraphicsscene_->setObjectName( nm );
}


uiGraphicsScene::~uiGraphicsScene()
{
    delete odgraphicsscene_;
}


int uiGraphicsScene::nrItems() const
{
    return odgraphicsscene_->items().size();
}


void uiGraphicsScene::addItem( uiGraphicsItem* itm )
{
    odgraphicsscene_->addItem( itm->qGraphicsItem() );
    items_ += itm;
}


void uiGraphicsScene::addItemGrp( uiGraphicsItemGroup* itmgrp )
{
    odgraphicsscene_->addItem( itmgrp->qGraphicsItemGroup() );
    items_ += itmgrp;
}


void uiGraphicsScene::removeItem( uiGraphicsItem* itm )
{
    odgraphicsscene_->removeItem( itm->qGraphicsItem() );
    items_ -= itm;
}


uiPixmapItem* uiGraphicsScene::addPixmap( const ioPixmap& pm )
{
    uiPixmapItem* uipixitem = new uiPixmapItem( new ODGraphicsPixmapItem(pm) );
    addItem( uipixitem );
    return uipixitem;
}


uiTextItem* uiGraphicsScene::addText( const char* txt )
{ 
    uiTextItem* uitextitem = new uiTextItem( odgraphicsscene_->addText(txt) );
    items_ += uitextitem;
    return uitextitem;
}

uiTextItem* uiGraphicsScene::addText( int x, int y, const char* text,
       				      const Alignment& al )
{
    uiTextItem* uitextitem = new uiTextItem( odgraphicsscene_->addText(text) );
    uitextitem->setPos( x, y );
    uitextitem->setAlignment( al );
    items_ += uitextitem;
    return uitextitem;
}


uiRectItem* uiGraphicsScene::addRect( float x, float y, float w, float h )
{ 
    uiRectItem* uirectitem =
	new uiRectItem( odgraphicsscene_->addRect(x,y,w,h) );
    uirectitem->moveBy( -w/2, -h/2 );
    items_ += uirectitem;
    return uirectitem;
}


uiEllipseItem* uiGraphicsScene::addEllipse( float x, float y, float w, float h )
{
    uiEllipseItem* uiellipseitem =
	new uiEllipseItem( odgraphicsscene_->addEllipse(x,y,w,h) );
    uiellipseitem->moveBy( -w/2, -h/2 );
    items_ += uiellipseitem;
    return uiellipseitem;
}


uiEllipseItem* uiGraphicsScene::addEllipse( const uiPoint& center,
					    const uiSize& sz )
{
    return addEllipse( center.x, center.y, sz.hNrPics(), sz.vNrPics() );
}


static uiPoint getEndPoint( const uiPoint& pt, double angle, double len )
{
    uiPoint endpt( pt );
    double delta = len * cos( angle );
    endpt.x += mNINT(delta);
    // In UI, Y is positive downward
    delta = -len * sin( angle );
    endpt.y += mNINT(delta);
    return endpt;
}


uiLineItem* uiGraphicsScene::addLine( float x, float y, float w, float h )
{
    uiLineItem* uilineitem =
	new uiLineItem( odgraphicsscene_->addLine(x,y,w,h) );
    items_ += uilineitem;
    return uilineitem;
}    


uiLineItem* uiGraphicsScene::addLine( const uiPoint& p1,
				      const uiPoint& p2 )
{
    return addLine( p1.x, p1.y, p2.x, p2.y );
}


uiLineItem* uiGraphicsScene::addLine( const uiPoint& pt, double angle,
				      double len )
{
    return addLine( pt, getEndPoint(pt,angle,len) );
}


uiPolygonItem* uiGraphicsScene::addPolygon( const TypeSet<uiPoint>& pts,
					    bool fill )
{
    QVector<QPointF> qpts;
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	QPointF pt( QPoint(pts[idx].x,pts[idx].y) );
	qpts += pt;
    }

    uiPolygonItem* uipolyitem = new uiPolygonItem(
	odgraphicsscene_->addPolygon(QPolygonF(qpts)) );
    if ( fill )
	uipolyitem->fill();
    items_ += uipolyitem;
    return uipolyitem;
}

uiPolyLineItem* uiGraphicsScene::addPolyLine( const TypeSet<uiPoint>& ptlist )
{
    uiPolyLineItem* polylineitem = new uiPolyLineItem();
    polylineitem->setPolyLine( ptlist );
    addItem( polylineitem );
    return polylineitem;
}


uiPointItem* uiGraphicsScene::addPoint( bool hl )
{
    uiPointItem* uiptitem = new uiPointItem();
    uiptitem->qPointItem()->setHighLight( hl );
    addItem( uiptitem );
    return uiptitem;
}


uiMarkerItem* uiGraphicsScene::addMarker( const MarkerStyle2D& mstyl, int side )
{
    uiMarkerItem* markeritem = new uiMarkerItem( mstyl );
    markeritem->qMarkerItem()->setSideLength( side );
    addItem( markeritem );
    return markeritem;
}


uiArrowItem* uiGraphicsScene::addArrow( const uiPoint& tail,
					const uiPoint& head,
					const ArrowStyle& arrstyle )
{
    uiArrowItem* arritem = new uiArrowItem();
    arritem->setArrowStyle( arrstyle );
    const int arrsz = (int)sqrt( (head.x - tail.x)*(head.x - tail.x) +
		      	         (tail.y - head.y)*(tail.y - head.y) );
    arritem->setArrowSize( arrsz );
    arritem->setPos( head.x, head.y );
    const uiPoint relvec( head.x - tail.x, tail.y - head.y );
    const double ang = atan2(relvec.y,relvec.x) *180/M_PI;
    arritem->rotate( ang );
    addItem( arritem );
    return arritem;
}


void uiGraphicsScene::setBackGroundColor( const Color& color )
{
    QBrush brush( QColor(color.r(),color.g(),color.b()) );
    odgraphicsscene_->setBackgroundBrush( brush );
}


const Color uiGraphicsScene::backGroundColor() const
{
    QColor color( odgraphicsscene_->backgroundBrush().color() ); 
    return Color( color.red() , color.green(), color.blue() );
}


void uiGraphicsScene::removeAllItems()
{
    QList<QGraphicsItem *> items = odgraphicsscene_->items();
    for ( int idx=0; idx<items.size(); idx++ )
	odgraphicsscene_->removeItem( items[idx] );
    deepErase( items_ );
}


int uiGraphicsScene::getSelItemSize() const
{
    QList<QGraphicsItem*> items = odgraphicsscene_->selectedItems();
    return items.size();
}


uiRect uiGraphicsScene::getSelectedArea() const
{
    QRectF selarea( odgraphicsscene_->selectionArea().boundingRect().toRect() );
    return uiRect( (int)selarea.topLeft().x(), (int)selarea.topLeft().y(),
	   	   (int)selarea.bottomRight().x(),
		   (int)selarea.bottomRight().y() );
}


void uiGraphicsScene::setSelectionArea( const uiRect& rect )
{
    const QRectF selrect( rect.topLeft().x, rect.topLeft().y, rect.width(),
	    		  rect.height() );
    QPainterPath selareapath;
    selareapath.addRect( selrect );
    odgraphicsscene_->setSelectionArea( selareapath );
}


void uiGraphicsScene::useBackgroundPattern( bool usebgpattern )
{
    odgraphicsscene_->setBackgroundOpaque( usebgpattern );

    if ( usebgpattern )
    {
	QBrush brush;
	brush.setColor( QColor(0,0,0) );
	brush.setStyle( Qt::DiagCrossPattern );
	odgraphicsscene_->setBackgroundBrush( brush );
    }
}


double uiGraphicsScene::width() const
{ return odgraphicsscene_->width(); }

double uiGraphicsScene::height() const
{ return odgraphicsscene_->height(); }

void uiGraphicsScene::setSceneRect( float x, float y, float w, float h )
{ odgraphicsscene_->setSceneRect( x, y, w, h ); }
