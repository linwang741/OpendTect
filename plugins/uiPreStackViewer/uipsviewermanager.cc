/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		5-11-2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewermanager.cc,v 1.28 2008-12-18 11:04:10 cvsbert Exp $";

#include "uipsviewermanager.h"

#include "bufstringset.h"
#include "ioman.h"
#include "ioobj.h"
#include "prestackgather.h"
#include "prestackprocessor.h"
#include "settings.h"
#include "survinfo.h"
#include "uidlggroup.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewmainwin.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uipsviewershapetab.h"
#include "uipsviewerposdlg.h"
#include "uipsviewersettingdlg.h"
#include "uiseispartserv.h"
#include "uisoviewer.h"
#include "uivispartserv.h"
#include "visflatviewer.h"
#include "visplanedatadisplay.h"
#include "visprestackviewer.h"
#include "visseis2ddisplay.h"
#include "vistransform.h"
#include "uipsviewercoltab.h"
#include "uiamplspectrum.h"


namespace PreStackView
{

uiPSViewerMgr::uiPSViewerMgr()
    : selectpsdatamenuitem_( "D&isplay PS Gather" )
    , positionmenuitem_( "P&osition ..." )  
    , proptymenuitem_( "&Properties ..." )				 
    , viewermenuitem_( "View in &2D Panel" )
    , amplspectrumitem_( "&Amplitude spectrum" )
    , removemenuitem_( "&Remove" ) 
    , visserv_( ODMainWin()->applMgr().visServer() )
    , preprocmgr_( new PreStack::ProcessManager )
{
    visserv_->removeAllNotifier().notify( mCB(this,uiPSViewerMgr,removeAllCB) );
    RefMan<MenuHandler> menuhandler = visserv_->getMenuHandler();

    IOM().surveyToBeChanged.notify(mCB(this,uiPSViewerMgr,surveyToBeChangedCB));
    ODMainWin()->sessionSave.notify( mCB(this,uiPSViewerMgr,sessionSaveCB) );
    ODMainWin()->sessionRestore.notify(
	    mCB(this,uiPSViewerMgr,sessionRestoreCB) );
       
    menuhandler->createnotifier.notify( mCB(this,uiPSViewerMgr,createMenuCB) );
    menuhandler->handlenotifier.notify( mCB(this,uiPSViewerMgr,handleMenuCB) );
}


uiPSViewerMgr::~uiPSViewerMgr()
{
    visserv_->removeAllNotifier().remove( mCB(this,uiPSViewerMgr,removeAllCB) );
    RefMan<MenuHandler> menuhandler = visserv_->getMenuHandler(); 

    IOM().surveyToBeChanged.remove(mCB(this,uiPSViewerMgr,surveyToBeChangedCB));
    ODMainWin()->sessionSave.remove( mCB(this,uiPSViewerMgr,sessionSaveCB) );
    ODMainWin()->sessionRestore.remove( 
	    mCB(this,uiPSViewerMgr,sessionRestoreCB) );
    menuhandler->createnotifier.remove( mCB(this,uiPSViewerMgr,createMenuCB) );
    menuhandler->handlenotifier.remove( mCB(this,uiPSViewerMgr,handleMenuCB) );

    delete visserv_;
    removeAllCB( 0 );
    delete preprocmgr_;
}    


void uiPSViewerMgr::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet( uiMenuHandler*, menu, cb );
    
    RefMan<visBase::DataObject> dataobj = visserv_->getObject( menu->menuID() );

    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, dataobj.ptr() );
    mDynamicCastGet( visSurvey::Seis2DDisplay*, s2d, dataobj.ptr() );
    if ( (pdd && pdd->getOrientation()!=visSurvey::PlaneDataDisplay::Timeslice) 
	   || s2d ) 
    {
	uiSeisPartServer* seisserv = ODMainWin()->applMgr().seisServer();

	BufferStringSet gnms; seisserv->getStoredGathersList(!s2d,gnms);
	if ( gnms.size() )
	{
	    selectpsdatamenuitem_.removeItems();
	    selectpsdatamenuitem_.createItems(gnms);
	    
	    mAddMenuItem( menu, &selectpsdatamenuitem_, true, false );
	}
	else
	    mResetMenuItem( &selectpsdatamenuitem_ );
    }
    else
	mResetMenuItem( &selectpsdatamenuitem_ );

    mDynamicCastGet( PreStackView::PreStackViewer*, psv, dataobj.ptr() );
    if ( psv )
    {
    	mAddMenuItem( menu, &positionmenuitem_, true, false );
	mAddMenuItem( menu, &proptymenuitem_, true, false );
    	mAddMenuItem( menu, &viewermenuitem_, true, false ); 
    	mAddMenuItem( menu, &amplspectrumitem_, true, false ); 
    	mAddMenuItem( menu, &removemenuitem_, true, false ); 
    }
    else
    {
	mResetMenuItem( &positionmenuitem_ );
	mResetMenuItem( &proptymenuitem_ );
	mResetMenuItem( &viewermenuitem_ );
	mResetMenuItem( &amplspectrumitem_ );
	mResetMenuItem( &removemenuitem_ );
    }
}


void uiPSViewerMgr::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);

    if ( menu->isHandled() )
	return;

    int sceneid = getSceneID( menu->menuID() );
    if ( sceneid==-1 )
	return;

    const int mnuidx = selectpsdatamenuitem_.itemIndex( mnuid );

    RefMan<visBase::DataObject> dataobj = visserv_->getObject( menu->menuID() );
    mDynamicCastGet(PreStackView::PreStackViewer*,psv,dataobj.ptr())
    if ( mnuidx < 0 && !psv )
	return;

    if ( mnuidx>=0 )
    {
	menu->setIsHandled( true );
	if ( !addNewPSViewer( menu, sceneid, mnuidx ) )
	    return;
    }
    else if ( mnuid==removemenuitem_.id )
    {
	menu->setIsHandled( true );
	visserv_->removeObject( psv, sceneid );
	viewers_ -= psv;
	psv->unRef();
    }
    else if ( mnuid==proptymenuitem_.id )
    {
	menu->setIsHandled( true );
	uiPSViewerSettingDlg dlg(menu->getParent(), *psv, *this, *preprocmgr_);
	dlg.go();
    }
    else if ( mnuid==positionmenuitem_.id )
    {
	menu->setIsHandled( true );
	uiPSViewerPositionDlg dlg( menu->getParent(), *psv );
	dlg.go();
    }
    else if ( mnuid==viewermenuitem_.id )
    {
	menu->setIsHandled( true );
	PtrMan<IOObj> ioobj = IOM().get( psv->getMultiID() );
	if ( !ioobj )
	   return;

	BufferString title = psv->is3DSeis() ?
	    getSeis3DTitle( psv->getBinID(), ioobj->name() ) :
	    getSeis2DTitle( psv->traceNr(), psv->lineName() );	
	uiFlatViewWin* viewwin = create2DViewer( title, psv->getDataPackID() );

	if ( viewwin )
	{
    	    viewwindows_ += viewwin;
    	    viewwin->start();
	}
    }
    else if ( mnuid==amplspectrumitem_.id )
    {
	menu->setIsHandled( true );
	uiAmplSpectrum* asd = new uiAmplSpectrum( menu->getParent() );
	asd->setDeleteOnClose( true );
	asd->setDataPackID( psv->getDataPackID(), DataPackMgr::FlatID );
	BufferString capt( "Amplitude spectrum for " );
	capt += psv->getObjectName();
	capt += " at ";
	if ( psv->is3DSeis() )
	    { capt += psv->getPosition().inl; capt += "/"; }
	capt += psv->getPosition().crl;
	asd->setCaption( capt );
	asd->show();
    }
}


int uiPSViewerMgr::getSceneID( int mnid )
{
    int sceneid = -1;
    TypeSet<int> sceneids;
    visserv_->getChildIds( -1, sceneids );
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	TypeSet<int> scenechildren;
	visserv_->getChildIds( sceneids[idx], scenechildren );
	if ( scenechildren.indexOf( mnid ) )
	{
	    sceneid = sceneids[idx];
	    break;
	}
    }
    
    return sceneid;
}


#define mErrReturn(msg) { uiMSG().error(msg); return false; }

bool uiPSViewerMgr::addNewPSViewer( const uiMenuHandler* menu, 
				    int sceneid, int mnuidx )
{
    if ( !menu )
	return false;

    PtrMan<IOObj> ioobj = IOM().getLocal(
	    selectpsdatamenuitem_.getItem(mnuidx)->text );
    if ( !ioobj )
	mErrReturn( "No object selected" )

    RefMan<visBase::DataObject> dataobj =
	visserv_->getObject( menu->menuID() );
		
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, dataobj.ptr() );
    mDynamicCastGet( visSurvey::Seis2DDisplay*, s2d, dataobj.ptr() );
    if ( !pdd && !s2d )
	mErrReturn( "Display panel is not set." )

    Coord3 pickedpos = menu->getPickedPos();

    PreStackViewer* viewer = PreStackViewer::create();
    viewer->ref();
    viewer->setMultiID( ioobj->key() );
    visserv_->addObject( viewer, sceneid, false );
    viewers_ += viewer;

    const uiSoViewer*  sovwr = ODMainWin()->sceneMgr().getSoViewer( sceneid );
    const Coord3 campos = sovwr->getCameraPosition();
    const Coord3 displaycampos = 
	viewer->getScene()->getUTM2DisplayTransform()->transformBack( campos );
    const BinID dir0 = SI().transform(displaycampos)-SI().transform(pickedpos);
    const Coord dir( dir0.inl, dir0.crl );

    if ( pdd )
    {
	viewer->setSectionDisplay( pdd ); 
	BinID bid;
	if (  menu->getMenuType() != uiMenuHandler::fromScene ) 
	{
	    HorSampling hrg = pdd->getCubeSampling().hrg;
	    bid = SI().transform((SI().transform(hrg.start)
				 +SI().transform(hrg.stop))/2);
	}
	else bid = SI().transform( pickedpos );
    	
	viewer->displaysOnPositiveSide( viewer->getBaseDirection().dot(dir)>0 );
	if ( !viewer->setPosition( bid ) )
	    mErrReturn( "No prestack data at this position" )
    } 
    else if ( s2d )
    {
	int trcnr;
	if ( menu->getMenuType() != uiMenuHandler::fromScene )
	    trcnr = s2d->getTraceNrRange().center();
	else
	    trcnr = s2d->getNearestTraceNr( pickedpos );

	viewer->setSeis2DDisplay( s2d, trcnr );
	viewer->displaysOnPositiveSide( viewer->getBaseDirection().dot(dir)>0 );
	if ( !viewer->setSeis2DData( ioobj ) )
	    mErrReturn( "No prestack data at this position" )
    }
    
    bool autoview;
    if ( Settings::common().getYN(PreStackViewer::sKeyAutoWidth(), autoview) ||
	 Settings::common().getYN("AutoWidth", autoview) )
	viewer->displaysAutoWidth( autoview );

    float factor;
    if ( Settings::common().get( PreStackViewer::sKeyFactor(), factor ) ||
	 Settings::common().get( "Factor", factor ) )
	viewer->setFactor( factor );
   
    float width; 
    if ( Settings::common().get( PreStackViewer::sKeyWidth(), width ) ||
	 Settings::common().get( "Width", width ) )
	viewer->setWidth( width );
    
    IOPar* par = Settings::common().subselect( 
	    PreStackView::uiPSViewerColTab::sKeyDataPars() );
    if ( par )
	viewer->flatViewer()->appearance().ddpars_.usePar( *par );
    BufferString coltab;
    if ( Settings::common().get( FlatView::DataDispPars::sKeyColTab, coltab ) )
	viewer->flatViewer()->appearance().ddpars_.vd_.ctab_ =  coltab;
	
    viewer->flatViewer()->handleChange( FlatView::Viewer::VDPars );

    if ( viewer->getScene() )
	viewer->getScene()->change.notifyIfNotNotified( mCB( this, 
		    uiPSViewerMgr,sceneChangeCB ) );
    
    return true;
}


#define mErrRes(msg) { uiMSG().error(msg); return 0; }

uiFlatViewWin* uiPSViewerMgr::create2DViewer( BufferString title, 
	const int dpid ) 
{
    uiFlatViewWin* viewwin = new uiFlatViewMainWin( 
	    ODMainWin()->applMgr().seisServer()->appserv().parent(), 
	    uiFlatViewMainWin::Setup(title) );
    
    viewwin->setWinTitle( title );
    viewwin->setDarkBG( false );
    
    uiFlatViewer& vwr = viewwin->viewer();
    vwr.appearance().annot_.setAxesAnnot( true );
    vwr.appearance().setGeoDefaults( true );
    vwr.appearance().ddpars_.show( false, true );
    vwr.appearance().ddpars_.wva_.overlap_ = 1;

    DataPack* dp = DPM(DataPackMgr::FlatID).obtain( dpid );
    if ( !dp )
	return 0;

    mDynamicCastGet( const FlatDataPack*, fdp, dp );
    if ( !fdp )
    {
	DPM(DataPackMgr::FlatID).release( dp );
	return false;
    }

    vwr.setPack( false, dpid, false, true );
    int pw = 200 + 10 * fdp->data().info().getSize( 1 );
    if ( pw < 400 ) pw = 400; if ( pw > 800 ) pw = 800;
    
    vwr.setInitialSize( uiSize(pw,500) );  
    viewwin->addControl( new uiFlatViewStdControl( vwr,
			 uiFlatViewStdControl::Setup().withstates(false) ) );
    vwr.drawBitMaps();
    vwr.drawAnnot();
    DPM(DataPackMgr::FlatID).release( dpid );
    return viewwin;
}


void uiPSViewerMgr::sceneChangeCB( CallBacker* )
{
    for ( int idx = 0; idx<viewers_.size(); idx++ )
    {
	PreStackView::PreStackViewer* psv = viewers_[idx];
	visBase::Scene* scene = psv->getScene();	

	int dpid = psv->getDataPackID();
	const visSurvey::PlaneDataDisplay* pdd = psv->getSectionDisplay();
	const visSurvey::Seis2DDisplay*    s2d = psv->getSeis2DDisplay();
	if ( pdd && (!scene || scene->getFirstIdx( pdd )==-1 ) )
	{
	    removeViewWin( dpid );
	    viewers_.remove( idx );
	    if ( scene ) visserv_->removeObject( psv, scene->id() );
	    psv->unRef();
	    idx--;
	}
	
	if ( s2d && (!scene || scene->getFirstIdx( s2d )==-1 ) )
	{
	    removeViewWin( dpid );
	    viewers_.remove( idx );
	    if ( scene ) visserv_->removeObject( psv, scene->id() );
	    psv->unRef();
	    idx--;
	}
    }
}


void uiPSViewerMgr::removeViewWin( const int dpid )
{
    for ( int idx=0; idx<viewwindows_.size(); idx++ )
    {
	if ( viewwindows_[idx]->viewer().packID(false) !=dpid )
	    continue;
	
	viewwindows_ -= viewwindows_[idx];
	delete viewwindows_[idx];
    }
}


void uiPSViewerMgr::sessionRestoreCB( CallBacker* )
{
    deepErase( viewwindows_ );

    TypeSet<int> vispsviewids;
    visserv_->findObject( typeid(PreStackView::PreStackViewer), vispsviewids );

    for ( int idx=0; idx<vispsviewids.size(); idx++ )
    {
	mDynamicCastGet( PreStackView::PreStackViewer*, psv,
			 visserv_->getObject(vispsviewids[idx]) );
	if ( !psv )
	    continue;

	if ( psv->getScene() )
	    psv->getScene()->change.notifyIfNotNotified( mCB( this, 
			uiPSViewerMgr,sceneChangeCB ) );
	viewers_ += psv;
	psv->ref();
    }
    
    PtrMan<IOPar> allwindowspar = ODMainWin()->sessionPars().subselect(
	    			  sKey2DViewers() );
    int nrwindows;
    if ( !allwindowspar || !allwindowspar->get(sKeyNrWindows(), nrwindows) )
	return;

    for ( int idx=0; idx<nrwindows; idx++ )
    {
	BufferString key = sKeyViewerPrefix();
	key += idx;
	PtrMan<IOPar> viewerpar = allwindowspar->subselect( key.buf() );
	if ( !viewerpar )
	    continue;

	MultiID mid;
	bool is3d;
	int trcnr;
	BinID bid;
	BufferString name2d;
	if ( !viewerpar->get( sKeyMultiID(), mid ) ||
	     !viewerpar->get( sKeyBinID(), bid ) ||
	     !viewerpar->get( sKeyTraceNr(), trcnr ) ||
	     !viewerpar->get( sKeySeis2DName(), name2d ) ||
	     !viewerpar->getYN( sKeyIs3D(), is3d ) )
	    continue;
    
	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj )
	   continue;

	BufferString title = !is3d ? getSeis2DTitle( trcnr, name2d ) :
	    			    getSeis3DTitle( bid, ioobj->name() );

	PreStack::Gather* gather = new PreStack::Gather;
	int dpid;
	if ( is3d && gather->readFrom(mid,bid) )
	    dpid = gather->id();
	else if ( gather->readFrom( *ioobj, trcnr, name2d ) )
	    dpid = gather->id();
	else 
	{
	    delete gather;
	    continue;	    
	}

	DPM(DataPackMgr::FlatID).add( gather );
	DPM(DataPackMgr::FlatID).obtain( dpid );
	uiFlatViewWin* viewwin = create2DViewer( title, dpid );
	DPM(DataPackMgr::FlatID).release( gather );
	if ( !viewwin )
	    continue;

	viewwindows_ += viewwin;
	viewwin->start();
    }
    
    if ( preprocmgr_ )
	preprocmgr_->usePar( *allwindowspar );

    for ( int idx=0; idx<viewers_.size(); idx++ )
	viewers_[idx]->setPreProcessor( preprocmgr_ );
}


const char* uiPSViewerMgr::getSeis2DTitle( const int tracenr, BufferString nm )
{
    BufferString title( "Gather from [" );
    title += nm;
    title += "] at trace " ;
    title += tracenr;

    return title;
}


const char* uiPSViewerMgr::getSeis3DTitle( BinID bid, BufferString name )
{
    BufferString title( "Gather from [" );
    title += name;
    title += "] at ";
    title += bid.inl;
    title += "/";
    title += bid.crl;

    return title;
}


void uiPSViewerMgr::sessionSaveCB( CallBacker* ) 
{
    IOPar allwindowpar;
    int nrsaved = 0;
    for ( int idx=0; idx<viewwindows_.size(); idx++ )
    {
	const FlatDataPack* dp = viewwindows_[idx]->viewer().pack( false );
	mDynamicCastGet( const PreStack::Gather*, gather, dp );
	if ( !gather )
	    continue;

	IOPar viewerpar;
	viewwindows_[idx]->viewer().fillPar( viewerpar );
	viewerpar.set( sKeyBinID(), gather->getBinID() );
	viewerpar.set( sKeyMultiID(), gather->getStorageID() );
	viewerpar.set( sKeyTraceNr(), gather->getSeis2DTraceNr() );
	viewerpar.set( sKeySeis2DName(), gather->getSeis2DName() );
	viewerpar.setYN( sKeyIs3D(), gather->is3D() );

	BufferString key = sKeyViewerPrefix();
	key += nrsaved;
	nrsaved++;

	allwindowpar.mergeComp( viewerpar, key );
    }

    if ( preprocmgr_ )
	preprocmgr_->fillPar( allwindowpar );

    allwindowpar.set( sKeyNrWindows(), nrsaved );
    ODMainWin()->sessionPars().mergeComp( allwindowpar, sKey2DViewers() );
}


void  uiPSViewerMgr::removeAllCB( CallBacker* )
{
    deepUnRef( viewers_ );
    deepErase( viewwindows_ );
}    


void uiPSViewerMgr::surveyToBeChangedCB( CallBacker* )
{
    deepUnRef( viewers_ );
    deepErase( viewwindows_ );
}

}; // Namespace
