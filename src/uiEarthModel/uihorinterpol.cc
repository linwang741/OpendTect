/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uihorinterpol.h"

#include "arrayndimpl.h"
#include "array1dinterpol.h"
#include "array2dinterpol.h"
#include "arraynd.h"
#include "ctxtioobj.h"
#include "datainpspec.h"
#include "executor.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "horizongridder.h"
#include "survinfo.h"

#include "uiarray1dinterpol.h"
#include "uiarray2dinterpol.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uihorsavefieldgrp.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

uiHorizonInterpolDlg::uiHorizonInterpolDlg( uiParent* p, EM::Horizon* hor,
					    bool is2d )
    : uiDialog( p, uiDialog::Setup("Horizon Gridding","Gridding parameters",
				   "104.0.16").modal(true) )
    , horizon_( hor )
    , is2d_( is2d )
    , inputhorsel_( 0 )
    , interpol2dsel_( 0 )
    , interpol1dsel_( 0 )
    , savefldgrp_( 0 )
    , finished(this)
{
    setCtrlStyle( DoAndStay );

    if ( horizon_ )
	horizon_->ref();
    else
    {
	IOObjContext ctxt = is2d ? EMHorizon2DTranslatorGroup::ioContext()
	    			 : EMHorizon3DTranslatorGroup::ioContext();
	ctxt.forread = true;
	inputhorsel_ = new uiIOObjSel( this, ctxt );
    }

    if ( !is2d )
    {
	const char* scopes[] = { "Full survey", "Bounding box",
				     "Convex hull", "Only holes", 0 };
	geometrysel_ = new uiGenInput( this, "Scope",
				       StringListInpSpec(scopes) );
	geometrysel_->setText( scopes[2] );

	if ( inputhorsel_ ) geometrysel_->attach( alignedBelow, inputhorsel_ );
	interpol2dsel_ =
	    new uiArray2DInterpolSel( this, false, true, false, 0, true);
	interpol2dsel_->setDistanceUnit( SI().xyInFeet() ? "[ft]" : "[m]" );
	interpol2dsel_->attach( alignedBelow, geometrysel_ );
	mDynamicCastGet(EM::Horizon3D*,hor3d,hor);
	if ( hor3d )
	{
	    RowCol rc = hor3d->geometry().step();
	    BinID step( rc.row(), rc.col() );
	    interpol2dsel_->setStep( step );
	}
    }
    else
    {
	interpol1dsel_ = new uiArray1DInterpolSel( this, false, true );
	interpol1dsel_->setDistanceUnit( SI().xyInFeet() ? "[ft]" : "[m]" );
	if ( inputhorsel_ )
	    interpol1dsel_->attach( alignedBelow, inputhorsel_ );
    }

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );

    savefldgrp_ = new uiHorSaveFieldGrp( this, horizon_, is2d );
    savefldgrp_->setSaveFieldName( "Save gridded horizon" );
    if ( is2d )
    {
	sep->attach( stretchedBelow, interpol1dsel_ );
	savefldgrp_->attach( alignedBelow, interpol1dsel_ );
    }
    else
    {
	sep->attach( stretchedBelow, interpol2dsel_ );
	savefldgrp_->attach( alignedBelow, interpol2dsel_ );
    }
    savefldgrp_->attach( ensureBelow, sep );
}


uiHorizonInterpolDlg::~uiHorizonInterpolDlg()
{
    if ( horizon_ ) horizon_->unRef();
}


const char* uiHorizonInterpolDlg::helpID() const
{
    return uiDialog::helpID();
}


#define mErrRet(msg) { if ( msg ) uiMSG().error( msg ); return false; }

bool uiHorizonInterpolDlg::interpolate3D()
{
    PtrMan<Array2DInterpol> interpolator = interpol2dsel_->getResult();
    if ( !interpolator )
	return false;

    Array2DInterpol::FillType filltype;
    switch ( geometrysel_->getIntValue() )
    {
	case 0:
	    filltype = Array2DInterpol::Full;
	    savefldgrp_->setFullSurveyArray( true );
	    break;
	case 1:
	    filltype = Array2DInterpol::Full;
	    break;
	case 2:
	    filltype = Array2DInterpol::ConvexHull;
	    break;
	default:
	    filltype = Array2DInterpol::HolesOnly;
    }

    interpolator->setFillType( filltype );

    if ( !savefldgrp_->acceptOK(0) ) 
	return false;

    EM::Horizon* usedhor = savefldgrp_->getNewHorizon() ?
	savefldgrp_->getNewHorizon() : horizon_;

    mDynamicCastGet(EM::Horizon3D*,usedhor3d,usedhor)
    if ( !usedhor3d )
	return false;
    
    mDynamicCastGet(EM::Horizon3D*,hor3d,horizon_)
    if ( !hor3d )
	return false;

    uiTaskRunner tr( this );

    bool success = false;
    for ( int idx=0; idx<hor3d->geometry().nrSections(); idx++ )
    {
	const EM::SectionID sid = hor3d->geometry().sectionID( idx );

	BinID steps = interpol2dsel_->getStep();
	StepInterval<int> rowrg = hor3d->geometry().rowRange( sid );
	rowrg.step = steps.inl();
	StepInterval<int> colrg = hor3d->geometry().colRange();
	colrg.step = steps.crl();
	
	interpolator->setRowStep( SI().inlDistance()*steps.inl() );
	interpolator->setColStep( SI().crlDistance()*steps.crl() );
	
	HorSampling hs( false );
	hs.set( rowrg, colrg );
	interpolator->setOrigin( hs.start );

	Array2DImpl<float>* arr =
	    new Array2DImpl<float>( hs.nrInl(), hs.nrCrl() );
	if ( !arr->isOK() )
	{
	    BufferString msg( "Not enough horizon data for section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

        arr->setAll( mUdf(float) );	

	PtrMan<EM::EMObjectIterator> iterator = hor3d->createIterator( sid );
	if ( !iterator )
	    continue;

	while( true )
	{
	    EM::PosID posid = iterator->next();
	    if ( posid.objectID() == -1 )
		break;
	    BinID bid = posid.getRowCol();
	    if ( hs.includes(bid) )
	    {
		Coord3 pos = hor3d->getPos( posid );
		arr->set( hs.inlIdx(bid.inl()), hs.crlIdx(bid.crl()),
			  (float) pos.z );
	    }
	}

	if ( !interpolator->setArray(*arr,&tr) )
	{
	    BufferString msg( "Cannot setup interpolation on section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	if ( !TaskRunner::execute( &tr, *interpolator ) )
	{
	    BufferString msg( "Cannot interpolate section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	success = true;
	usedhor3d->geometry().sectionGeometry(sid)->setArray(
					    hs.start, hs.step, arr, true );
    }

    return success;
}


bool uiHorizonInterpolDlg::interpolate2D()
{
    if ( !savefldgrp_->acceptOK(0) )
	return false;

    EM::Horizon* usedhor = !savefldgrp_->overwriteHorizon() ?
	savefldgrp_->getNewHorizon() : horizon_;

    mDynamicCastGet(EM::Horizon2D*,usedhor2d,usedhor)
    if ( !usedhor2d )
	return false;
    
    mDynamicCastGet(EM::Horizon2D*,hor2d,horizon_)
    if ( !hor2d )
	return false;

    const EM::Horizon2DGeometry& geom = hor2d->geometry();
	
    uiTaskRunner tr( this );

    for ( int isect=0; isect<geom.nrSections(); isect++ )
    {
	ObjectSet< Array1D<float> > arr1d;
	const EM::SectionID sid = geom.sectionID( isect );
	for ( int lineidx=0; lineidx<geom.nrLines(); lineidx++ )
	    arr1d += hor2d->createArray1D( sid, geom.lineKey(lineidx) );

	interpol1dsel_->setInterpolators( geom.nrLines() );
	interpol1dsel_->setArraySet( arr1d );
	
	ExecutorGroup execgrp( "Interpolator", true );
	for ( int idx=0; idx<arr1d.size(); idx++ )
	    execgrp.add( interpol1dsel_->getResult(idx) );

	if ( !TaskRunner::execute( &tr, execgrp ) )
	{
	    BufferString msg( "Cannot interpolate section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	for ( int idx=0; idx<arr1d.size(); idx++ )
	    usedhor2d->setArray1D( *arr1d[idx], sid,geom.lineKey(idx),false);
    }

    return true;
}


bool uiHorizonInterpolDlg::acceptOK( CallBacker* cb )
{
    const bool isok = is2d_ ? interpol1dsel_->acceptOK()
			    : interpol2dsel_->acceptOK();
    if ( !isok )
	return false;

    if ( inputhorsel_ ) 
    {
	const IOObj* ioobj = inputhorsel_->ioobj();
	if ( !ioobj )
	    return false;

	EM::Horizon* hor = savefldgrp_->readHorizon( ioobj->key() );

	if ( horizon_ ) horizon_->unRef();

	horizon_ = hor;
	horizon_->ref();
    }

    if ( !horizon_ )
	mErrRet( "Missing horizon!" );

    MouseCursorChanger mcc( MouseCursor::Wait );
    
    if ( !is2d_ )
    {
	if ( !interpolate3D() )
	    return false;
    }
    else
    {
	if ( !interpolate2D() )
	    return false;
    }

    const bool res = savefldgrp_->saveHorizon();
    if ( res )
    {
	finished.trigger();
	uiMSG().message( "Horizon successfully gridded/interpolated" );
    }

    return false;
}


mImplFactory1Param(uiHor3DInterpol,uiParent*,uiHor3DInterpol::factory);

uiHor3DInterpolSel::uiHor3DInterpolSel( uiParent* p, bool musthandlefaults )
    : uiGroup(p,"Horizon3D Interpolation")
{
    methodgrps_.allowNull( true );

    const char* scopes[] = { "Full survey", "Bounding box", "Convex hull",
			     "Only holes", 0 };
    filltypefld_ = new uiGenInput( this, "Scope", StringListInpSpec(scopes) );
    filltypefld_->setText( scopes[2] );

    PositionInpSpec::Setup setup;
    PositionInpSpec spec( setup );
    stepfld_ = new uiGenInput( this, "Inl/Crl Step", spec );
    stepfld_->setValue( BinID(SI().inlStep(),SI().crlStep()) );
    stepfld_->attach( alignedBelow, filltypefld_ );

    maxholeszfld_ = new uiGenInput( this, 0, FloatInpSpec() );
    maxholeszfld_->setWithCheck( true );
    maxholeszfld_->attach( alignedBelow, stepfld_ );

    const BufferStringSet& methods = uiHor3DInterpol::factory().getNames(false);
    methodsel_ = new uiGenInput( this, "Algorithm",
	    	StringListInpSpec(uiHor3DInterpol::factory().getNames(true) ) );
    methodsel_->attach( alignedBelow, maxholeszfld_ );
    methodsel_->valuechanged.notify( mCB(this,uiHor3DInterpolSel,methodSelCB) );

    for ( int idx=0; idx<methods.size(); idx++ )
    {
	uiHor3DInterpol* methodgrp = uiHor3DInterpol::factory().create(
					methods[idx]->buf(), this, true );
	if ( methodgrp )
	    methodgrp->attach( alignedBelow, methodsel_ );

	methodgrps_ += methodgrp;
    }

    methodSelCB( 0 );
}


void uiHor3DInterpolSel::methodSelCB( CallBacker* )
{
    const int selidx = methodsel_ ? methodsel_->getIntValue( 0 ) : 0;
    for ( int idx=0; idx<methodgrps_.size(); idx++ )
    {
	if ( methodgrps_[idx] )
	    methodgrps_[idx]->display( idx==selidx );
    }
}


bool uiHor3DInterpolSel::fillPar( IOPar& iopar ) const
{
    return true;
}


bool uiHor3DInterpolSel::usePar( const IOPar& iopar )
{
    return true;
}


uiHor3DInterpol::uiHor3DInterpol( uiParent* p )
    : uiGroup(p,"Horizon Interpolation")
{
}


void uiInvDistHor3DInterpol::initClass()
{
    uiHor3DInterpol::factory().addCreator( create,
	    	uiInvDistHor3DInterpol::sFactoryKeyword() );
}


uiHor3DInterpol* uiInvDistHor3DInterpol::create( uiParent* p )
{
    return new uiInvDistHor3DInterpol( p );
}


uiInvDistHor3DInterpol::uiInvDistHor3DInterpol( uiParent* p )
    : uiHor3DInterpol(p)
{
    fltselfld_ = new uiFaultParSel( this, false );

    radiusfld_ = new  uiGenInput( this, 0, FloatInpSpec() );
    radiusfld_->setWithCheck( true );
    radiusfld_->setChecked( true );
    radiusfld_->checked.notify( mCB(this,uiInvDistHor3DInterpol,useRadiusCB) );
    radiusfld_->attach( alignedBelow, fltselfld_ );

    parbut_ = new uiPushButton( this, "&Parameters", 
		    	mCB(this,uiInvDistHor3DInterpol,doParamDlg),
			false );
    parbut_->attach( rightOf, radiusfld_ );


    setHAlignObj( radiusfld_ );
    useRadiusCB( 0 );
}


void uiInvDistHor3DInterpol::useRadiusCB( CallBacker* )
{
    parbut_->display( radiusfld_->isChecked() );
}


void uiInvDistHor3DInterpol::doParamDlg( CallBacker* )
{
    uiInvDistInterpolPars dlg( this, cornersfirst_, stepsz_, nrsteps_ );
    if ( !dlg.go() ) return;

    cornersfirst_ = dlg.isCornersFirst();
    stepsz_ = dlg.stepSize();
    nrsteps_ = dlg.nrSteps();
}


bool uiInvDistHor3DInterpol::fillPar( IOPar& par ) const
{
    const bool hasradius = radiusfld_->isChecked();
    const float radius = hasradius ? radiusfld_->getfValue(0) : mUdf(float);
    if ( hasradius && (mIsUdf(radius) || radius<=0) )
    {
	uiMSG().error(
		"Please enter a positive value for the search radius\n"
		"(or uncheck the field)" );
	return false;
    }

    const TypeSet<MultiID>& selfaultids = fltselfld_->selFaultIDs();
    par.set( HorizonGridder::sKeyNrFaults(), selfaultids.size() );
    for ( int idx=0; idx<selfaultids.size(); idx++ )
	par.set( IOPar::compKey(HorizonGridder::sKeyFaultID(),idx),
		 selfaultids[idx] );

    par.set( InvDistHor3DGridder::sKeySearchRadius(), radius );
    if ( hasradius )
    {
	par.set( InvDistHor3DGridder::sKeySearchRadius(), radius );
	par.set( InvDistHor3DGridder::sKeyStepSize(), stepsz_ );
	par.set( InvDistHor3DGridder::sKeyNrSteps(), nrsteps_ );
    }

    return true;
}


bool uiInvDistHor3DInterpol::usePar( const IOPar& iopar )
{
    return true;
}


void uiTriangulationHor3DInterpol::initClass()
{
    uiHor3DInterpol::factory().addCreator( create,
	    uiTriangulationHor3DInterpol::sFactoryKeyword() );
}


uiHor3DInterpol* uiTriangulationHor3DInterpol::create( uiParent* p )
{ return new uiTriangulationHor3DInterpol( p ); }


uiTriangulationHor3DInterpol::uiTriangulationHor3DInterpol(uiParent* p)
    : uiHor3DInterpol(p)
{
    fltselfld_ = new uiFaultParSel( this, false );

    useneighborfld_ = new uiCheckBox( this, "Use nearest neighbor" );
    useneighborfld_->setChecked( false );
    useneighborfld_->activated.notify(
	    	mCB(this,uiTriangulationHor3DInterpol,useNeighborCB) );
    useneighborfld_->attach( alignedBelow, fltselfld_ );
    
    maxdistfld_ = new uiGenInput( this, 0, FloatInpSpec() );
    maxdistfld_->setWithCheck( true );
    maxdistfld_->attach( alignedBelow, useneighborfld_ );

    setHAlignObj( useneighborfld_ );
    useNeighborCB( 0 );
}


void uiTriangulationHor3DInterpol::useNeighborCB( CallBacker* )
{
    maxdistfld_->display( !useneighborfld_->isChecked() );
}


bool uiTriangulationHor3DInterpol::fillPar( IOPar& par ) const
{
    bool usemax = !useneighborfld_->isChecked() && maxdistfld_->isChecked();
    const float maxdist = maxdistfld_->getfValue();
    if ( usemax && !mIsUdf(maxdist) && maxdist<0 )
    {
	uiMSG().error( "Maximum distance must be > 0. " );
	return false;
    }

    const TypeSet<MultiID>& selfaultids = fltselfld_->selFaultIDs();
    par.set( HorizonGridder::sKeyNrFaults(), selfaultids.size() );
    for ( int idx=0; idx<selfaultids.size(); idx++ )
	par.set( IOPar::compKey(HorizonGridder::sKeyFaultID(),idx),
		 selfaultids[idx] );

    par.set( TriangulationHor3DGridder::sKeyDoInterpolation(),
	     !useneighborfld_->isChecked() );
    if ( usemax )
	par.set( TriangulationHor3DGridder::sKeyMaxDistance(), maxdist );

    return true;
}


bool uiTriangulationHor3DInterpol::usePar( const IOPar& iopar )
{
    return true;
}


void uiExtensionHor3DInterpol::initClass()
{
    uiHor3DInterpol::factory().addCreator( create,
	    uiExtensionHor3DInterpol::sFactoryKeyword() );
}


uiHor3DInterpol* uiExtensionHor3DInterpol::create( uiParent* p )
{ return new uiExtensionHor3DInterpol( p ); }


uiExtensionHor3DInterpol::uiExtensionHor3DInterpol( uiParent* p )
    : uiHor3DInterpol(p)
{
    nrstepsfld_ = new uiGenInput( this, "Number of steps", IntInpSpec(20) );
    setHAlignObj( nrstepsfld_ );
}


bool uiExtensionHor3DInterpol::fillPar( IOPar& par ) const
{
    if ( nrstepsfld_->getIntValue()<1 )
    {
	uiMSG().error( "Nr steps must be > 0." );	
	return false;
    }

    par.set( ExtensionHor3DGridder::sKeyNrSteps(), nrstepsfld_->getIntValue() );
    return true;
}


bool uiExtensionHor3DInterpol::usePar( const IOPar& iopar )
{
    return true;
}


