/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiempartserv.cc,v 1.107 2007-05-31 12:16:08 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiempartserv.h"

#include "binidselimpl.h"
#include "binidvalset.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "datainpspec.h"
#include "emfault.h"
#include "emhistory.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ioobj.h"
#include "parametricsurface.h"
#include "ptrman.h"
#include "surfaceinfo.h"
#include "survinfo.h"

#include "uichangesurfacedlg.h"
#include "uihor3dfrom2ddlg.h"
#include "uiexecutor.h"
#include "uiexphorizon.h"
#include "uigeninputdlg.h"
#include "uiimpfault.h"
#include "uiimphorattrib.h"
#include "uiimphorizon.h"
#include "uiioobjsel.h"
#include "uiiosurfacedlg.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uimultisurfaceread.h"
#include "uiselsimple.h"
#include "uisurfaceman.h"

#include <math.h>

const int uiEMPartServer::evDisplayHorizon	= 0;
const int uiEMPartServer::evRemoveTreeObject	= 1;

#define mErrRet(s) { BufferString msg( "Cannot load '" ); msg += s; msg += "'";\
    			uiMSG().error( msg ); return false; }

#define mDynamicCastAll(objid) \
    EM::EMObject* object = em_.getObject(objid); \
    mDynamicCastGet(EM::Surface*,surface,object) \
    mDynamicCastGet(EM::Horizon2D*,hor2d,object) \
    mDynamicCastGet(EM::Horizon3D*,hor3d,object) \
    mDynamicCastGet(EM::Fault*,fault,object) \


uiEMPartServer::uiEMPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , selemid_(-1)
    , em_(EM::EMM())
{
}


uiEMPartServer::~uiEMPartServer()
{
    em_.empty();
}


MultiID uiEMPartServer::getStorageID( const EM::ObjectID& emid ) const
{
    return em_.getMultiID(emid);
}


EM::ObjectID uiEMPartServer::getObjectID( const MultiID& mid ) const
{
    return em_.getObjectID(mid);
}


void uiEMPartServer::manageSurfaces( const char* typ )
{
    uiSurfaceMan dlg( appserv().parent(), typ );
    dlg.go();
}


bool uiEMPartServer::ioHorizon( bool imp )
{
    bool res = false;
    if ( imp )
    {
	uiImportHorizon dlg( appserv().parent() );
	res = dlg.go();
	if ( res && dlg.doDisplay() )
	{
	    const MultiID mid = dlg.getSelID();	
	    selemid_ = em_.getObjectID(mid);
	    sendEvent( evDisplayHorizon );
	}
    }
    else
    {
	uiExportHorizon dlg( appserv().parent() );
	res = dlg.go();
    }

    return res;    
}


bool uiEMPartServer::importHorizonAttribute()
{
    uiImportHorAttrib dlg( appserv().parent() );
    return dlg.go();
}


bool uiEMPartServer::importHorizon() { return ioHorizon( true ); }

bool uiEMPartServer::exportHorizon() { return ioHorizon( false ); }


BufferString uiEMPartServer::getName( const EM::ObjectID& id ) const
{
    return em_.objectName(em_.getMultiID(id));
}


const char* uiEMPartServer::getType( const EM::ObjectID& emid ) const
{
    return em_.objectType(em_.getMultiID(emid));
}


bool uiEMPartServer::isChanged( const EM::ObjectID& emid ) const
{
    EM::EMObject* emobj = em_.getObject(emid);
    return emobj && emobj->isChanged();
}


bool uiEMPartServer::isEmpty( const EM::ObjectID& emid ) const
{
    EM::EMObject* emobj = em_.getObject(emid);
    return emobj && emobj->isEmpty();
}


bool uiEMPartServer::isFullResolution( const EM::ObjectID& emid ) const
{
    mDynamicCastGet(const EM::Surface*,emsurf,em_.getObject(emid));
    return emsurf && emsurf->geometry().isFullResolution();
}


bool uiEMPartServer::isFullyLoaded( const EM::ObjectID& emid ) const
{
    const EM::EMObject* emobj = em_.getObject(emid);
    return emobj && emobj->isFullyLoaded();
}


bool uiEMPartServer::isShifted( const EM::ObjectID& emid ) const
{
    const EM::EMObject* emobj = em_.getObject( emid );
    mDynamicCastGet(const EM::Horizon3D*,hor3d,emobj)
    if ( !hor3d ) return false;

    const float shift = hor3d->geometry().getShift();
    return !mIsZero(shift,mDefEps);
}


void uiEMPartServer::fillHoles( const EM::ObjectID& emid )
{
    mDynamicCastGet(EM::Horizon3D*,hor3d,em_.getObject(emid))
    uiInterpolHorizonDlg dlg( appserv().parent(), hor3d );
    dlg.go();
}


void uiEMPartServer::filterSurface( const EM::ObjectID& emid )
{
    mDynamicCastGet(EM::Horizon3D*,hor3d,em_.getObject(emid))
    uiFilterHorizonDlg dlg( appserv().parent(), hor3d );
    dlg.go();
}


void uiEMPartServer::removeTreeObject( const EM::ObjectID& emid )
{
    selemid_ = emid;
    sendEvent( evRemoveTreeObject );
}


void uiEMPartServer::deriveHor3DFrom2D( const EM::ObjectID& emid )
{
    mDynamicCastGet(EM::Horizon2D*,h2d,em_.getObject(emid))
    uiHor3DFrom2DDlg dlg( appserv().parent(), *h2d, this );

    if ( dlg.go() && dlg.doDisplay() )
    {
	const MultiID mid = dlg.getSelID();
	selemid_ = em_.getObjectID(mid);
	sendEvent( evDisplayHorizon );
    }
}


#define mMkMsg(s) \
	BufferString msg( emobj->getTypeStr() ); \
	msg += " '"; msg += emobj->name(); msg += "' "; msg += s; \
	msg += ".\nDo you want to save it?"

void uiEMPartServer::askUserToSave( const EM::ObjectID& emid ) const
{
    const EM::EMObject* emobj = em_.getObject(emid);
    if ( !emobj || emobj->isEmpty() ) return;

    const MultiID& mid = emobj->multiID();
    PtrMan<IOObj> ioobj = IOM().get(mid);
    bool mustsave = false;
    if ( !ioobj )
    {
	mMkMsg( "was removed from storage" );
        mustsave = uiMSG().askGoOn( msg );
	if ( !mustsave ) return;

	const IOObjContext* ctxt = EM::EMM().getContext( emobj->getTypeStr() );
	CtxtIOObj ctio( *ctxt ); ctio.setName( emobj->name() );
	IOM().getEntry( ctio ); IOM().commitChanges( *ctio.ioobj );
	ioobj = ctio.ioobj;
    }

    if ( !mustsave )
    {
	if ( !isChanged(emid) )
	    return;
	mMkMsg( "has changed" );
	if ( !uiMSG().askGoOn(msg) )
	    return;
    }

    storeObject( emid, !isFullyLoaded(emid) );
}


void uiEMPartServer::selectHorizons( TypeSet<EM::ObjectID>& ids )
{ selectSurfaces( ids, EMHorizon3DTranslatorGroup::keyword ); }


void uiEMPartServer::select2DHorizons( TypeSet<EM::ObjectID>& ids )
{ selectSurfaces( ids, EMHorizon2DTranslatorGroup::keyword ); }


void uiEMPartServer::selectFaults( TypeSet<EM::ObjectID>& ids )
{ selectSurfaces( ids, EMFaultTranslatorGroup::keyword ); }


void uiEMPartServer::selectSurfaces( TypeSet<EM::ObjectID>& objids,
				     const BufferString& typ )
{
    BufferString lbl( typ ); lbl += " selection";
    uiDialog dlg( appserv().parent(), uiDialog::Setup(lbl,0,"104.3.1") );
    uiMultiSurfaceRead* uiobj = new uiMultiSurfaceRead( &dlg, typ );
    uiobj->singleSurfaceSelected.notify( mCB(&dlg,uiDialog,accept) );
    if ( !dlg.go() ) return;

    TypeSet<MultiID> surfaceids;
    uiobj->getSurfaceIds( surfaceids );

    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sel( sd );
    uiobj->getSurfaceSelection( sel );

    PtrMan<Executor> exec = em_.objectLoader( surfaceids, &sel );
    if ( !exec ) return;

    for ( int idx=0; idx<surfaceids.size(); idx++ )
    {
	EM::EMObject* obj = em_.getObject( em_.getObjectID(surfaceids[idx]) );
	obj->ref();
    }

    uiExecutor execdlg( appserv().parent(), *exec );
    if ( !execdlg.go() )
    {
	for ( int idx=0; idx<surfaceids.size(); idx++ )
	{
	    EM::EMObject* obj =
			em_.getObject( em_.getObjectID(surfaceids[idx]) );
	    obj->unRef();
	}

	return;
    }

    exec.erase(); //We don't want executor to unref objs at end of function

    for ( int idx=0; idx<surfaceids.size(); idx++ )
    {
	const EM::ObjectID objid = em_.getObjectID( surfaceids[idx] );
	EM::EMObject* obj = em_.getObject( objid );
	obj->unRefNoDelete();
	objids += objid;
    }
}


bool uiEMPartServer::loadAuxData( const EM::ObjectID& id,
				  const TypeSet<int>& selattribs )
{
    mDynamicCastAll(id);
    if ( !hor3d ) return false;

    hor3d->auxdata.removeAll();
    ExecutorGroup exgrp( "Surface data loader" );
    exgrp.setNrDoneText( "Nr done" );
    for ( int idx=0; idx<selattribs.size(); idx++ )
	exgrp.add( hor3d->auxdata.auxDataLoader(selattribs[idx]) );

    uiExecutor exdlg( appserv().parent(), exgrp );
    return exdlg.go();
}



int uiEMPartServer::loadAuxData( const EM::ObjectID& id, const char* attrnm )
{
    mDynamicCastAll(id);
    if ( !hor3d ) return -1;
    
    EM::SurfaceIOData sd;
    const MultiID mid = em_.getMultiID( id );
    em_.getSurfaceData( mid, sd );
    const int nritems = sd.valnames.size();
    int selidx = -1;
    for ( int idx=0; idx<nritems; idx++ )
    {
	const BufferString& nm = *sd.valnames[idx];
	if ( nm == attrnm )
	{ selidx= idx; break; }
    }

    if ( selidx<0 ) return -1;
    TypeSet<int> selattribs( 1, selidx );
    return loadAuxData( id, selattribs ) ? selidx : -1;
}


bool uiEMPartServer::showLoadAuxDataDlg( const EM::ObjectID& id )
{
    mDynamicCastAll(id);
    if ( !hor3d ) return false;

    EM::SurfaceIOData sd;
    const MultiID mid = em_.getMultiID( id );
    em_.getSurfaceData( mid, sd );
    uiSelectFromList::Setup setup( "Surface data", sd.valnames );
    setup.dlgtitle( "Select one or more attributes to be displayed\n"
	    	    "on the horizon. After loading, use 'Page Up'\n"
		    "and 'Page Down' buttons to scroll." );
    uiSelectFromList dlg( appserv().parent(), setup );
    if ( dlg.selFld() )
	dlg.selFld()->setMultiSelect( true );
    if ( !dlg.go() || !dlg.selFld() ) return false;

    TypeSet<int> selattribs;
    dlg.selFld()->getSelectedItems( selattribs );
    if ( selattribs.isEmpty() ) return false;

    hor3d->auxdata.removeAll();
    ExecutorGroup exgrp( "Loading surface data" );
    exgrp.setNrDoneText( "Nr done" );
    for ( int idx=0; idx<selattribs.size(); idx++ )
	exgrp.add( hor3d->auxdata.auxDataLoader(selattribs[idx]) );

    uiExecutor exdlg( appserv().parent(), exgrp );
    return exdlg.go();
}


bool uiEMPartServer::storeObject( const EM::ObjectID& id, bool storeas ) const
{
    mDynamicCastAll(id);
    if ( !object ) return false;

    PtrMan<Executor> exec = 0;

    if ( storeas && surface )
    {
	uiWriteSurfaceDlg dlg( appserv().parent(), *surface );
	if ( !dlg.go() ) return false;

	EM::SurfaceIOData sd;
	EM::SurfaceIODataSelection sel( sd );
	dlg.getSelection( sel );

	const MultiID& key = dlg.ioObj() ? dlg.ioObj()->key() : "";
	exec = surface->geometry().saver( &sel, &key );
	if ( exec ) surface->setMultiID( key );
    }
    else
	exec = object->saver();

    if ( !exec )
	return false;

    uiExecutor exdlg( appserv().parent(), *exec );
    return exdlg.go();
}


bool uiEMPartServer::storeAuxData( const EM::ObjectID& id, bool storeas ) const
{
    mDynamicCastAll(id);
    if ( !hor3d ) return false;

    int dataidx = -1;
    bool overwrite = false;
    if ( storeas )
    {
	uiStoreAuxData dlg( appserv().parent(), *hor3d );
	if ( !dlg.go() ) return false;

	dataidx = 0;
	overwrite = dlg.doOverWrite();
    }

    PtrMan<Executor> saver = hor3d->auxdata.auxDataSaver( dataidx, overwrite );
    if ( !saver )
    {
	uiMSG().error( "Cannot save attribute" );
	return false;
    }

    uiExecutor exdlg( appserv().parent(), *saver );
    return exdlg.go();
}


int uiEMPartServer::setAuxData( const EM::ObjectID& id,
				ObjectSet<const BinIDValueSet>& data, 
				const char* attribnm, int idx )
{
    mDynamicCastAll(id);
    if ( !hor3d ) { uiMSG().error( "Cannot find horizon" ); return -1; }
    if ( data.isEmpty() ) { uiMSG().error( "No data calculated" ); return -1; }

    const int nrdatavals = data[0]->nrVals();
    if ( idx>=nrdatavals ) return -1;

    BufferString auxnm;
    if ( attribnm )
	auxnm = attribnm;
    else
    {
	auxnm = "AuxData";
	auxnm += idx;
    }

    hor3d->auxdata.removeAll();
    const int auxdatanr = hor3d->auxdata.addAuxData( auxnm );

    BinID bid;
    BinIDValueSet::Pos pos;
    const int nrvals = hor3d->auxdata.nrAuxData();
    float vals[nrvals];
    for ( int sidx=0; sidx<data.size(); sidx++ )
    {
	const EM::SectionID sectionid = hor3d->sectionID( sidx );
	const BinIDValueSet& bivs = *data[sidx];

	EM::PosID posid( id, sectionid );
	while ( bivs.next(pos) )
	{
	    bivs.get( pos, bid, vals );
	    RowCol rc( bid.inl, bid.crl );
	    EM::SubID subid = rc.getSerialized();
	    posid.setSubID( subid );
	    hor3d->auxdata.setAuxDataVal( auxdatanr, posid, vals[idx] );
	}
    }

    return auxdatanr;
}


bool uiEMPartServer::getAuxData( const EM::ObjectID& oid, int auxdatanr,
				 BufferString& auxdataname,
	                         ObjectSet<BinIDValueSet>& auxdata ) const
{
    mDynamicCastAll(oid);
    if ( !hor3d || !hor3d->auxdata.auxDataName(auxdatanr) )
	return false;

    auxdataname = hor3d->auxdata.auxDataName( auxdatanr );
    deepErase( auxdata );
    auxdata.allowNull( true );

    for ( int idx=0; idx<hor3d->nrSections(); idx++ )
    {
	const EM::SectionID sid = hor3d->sectionID( idx );
	if ( !hor3d->geometry().sectionGeometry(sid) )
	{
	    auxdata += 0;
	    continue;
	}

	BinIDValueSet* res = new BinIDValueSet( 2, false );
	auxdata += res;

	float auxvals[2];
	BinID bid;
	auxvals[0] = 0;
	PtrMan<EM::EMObjectIterator> iterator = hor3d->createIterator( sid );
	while ( true )
	{
	    const EM::PosID pid = iterator->next();
	    if ( pid.objectID()==-1 )
		break;

	    auxvals[1] = hor3d->auxdata.getAuxDataVal( auxdatanr, pid );
	    bid.setSerialized( pid.subID() );
	    res->add( bid, auxvals );
	}
    }

    return true;
}


bool uiEMPartServer::getAllAuxData( const EM::ObjectID& oid,
				    BufferStringSet& nms,
				    ObjectSet<BinIDValueSet>& data ) const
{
    mDynamicCastAll(oid);
    if ( !hor3d ) return false;

    deepErase( data );
    data.allowNull( true );

    for ( int sidx=0; sidx<hor3d->nrSections(); sidx++ )
    {
	const EM::SectionID sid = hor3d->sectionID( sidx );
	if ( !hor3d->geometry().sectionGeometry(sid) )
	{
	    data += 0;
	    continue;
	}

	for ( int idx=0; idx<hor3d->auxdata.nrAuxData(); idx++ )
	{
	    if ( hor3d->auxdata.auxDataName(idx) )
		nms.add( hor3d->auxdata.auxDataName(idx) );
	}

	const int nrauxdata = nms.size()+1;
	BinIDValueSet* res = new BinIDValueSet( nrauxdata, false );
	data += res;

	float auxvals[nrauxdata];
	auxvals[0] = 0;
	BinID bid;
	PtrMan<EM::EMObjectIterator> iterator = hor3d->createIterator( sid );
	while ( true )
	{
	    const EM::PosID pid = iterator->next();
	    if ( pid.objectID()==-1 )
		break;

	    for ( int idx=0; idx<nms.size(); idx++ )
	    {
		const int auxidx = hor3d->auxdata.auxDataIndex( nms.get(idx) );
		auxvals[idx+1] = hor3d->auxdata.getAuxDataVal( auxidx, pid );
	    }
	    bid.setSerialized( pid.subID() );
	    res->add( bid, auxvals );
	}
    }

    return true;
}


void uiEMPartServer::removeHistory()
{
    em_.history().empty();
}


bool uiEMPartServer::loadSurface( const MultiID& mid,
       				  const EM::SurfaceIODataSelection* newsel )
{
    if ( em_.getObject(em_.getObjectID(mid)) )
	return true;

    Executor* exec = em_.objectLoader( mid, newsel );
    if ( !exec )
    {
	PtrMan<IOObj> ioobj = IOM().get(mid);
	BufferString nm = ioobj ? (const char*) ioobj->name()
				: (const char*) mid;
	mErrRet( nm );
    }

    EM::EMObject* obj = em_.getObject( em_.getObjectID(mid) );
    obj->ref();
    uiExecutor exdlg( appserv().parent(), *exec );
    if ( exdlg.go() <= 0 )
    {
	obj->unRef();
	return false;
    }

    delete exec;
    obj->unRefNoDelete();
    return true;
}


bool uiEMPartServer::importLMKFault()
{
    uiImportLMKFault dlg( appserv().parent() );
    return dlg.go();
}


void uiEMPartServer::getSurfaceInfo( ObjectSet<SurfaceInfo>& hinfos )
{
    for ( int idx=0; idx<em_.nrLoadedObjects(); idx++ )
    {
	mDynamicCastGet(EM::Horizon3D*,hor3d,em_.getObject(em_.objectID(idx)));
	if ( hor3d ) hinfos += new SurfaceInfo( hor3d->name(), hor3d->multiID() );
    }
}


void uiEMPartServer::getSurfaceDef( const TypeSet<EM::ObjectID>& selhorids,
				    BinIDValueSet& bivs,
				    const BinIDRange* br ) const
{
    bivs.empty(); bivs.setNrVals( 2, false );
    PtrMan<BinIDRange> sibr = 0;
    if ( selhorids.isEmpty() ) return;
    else if ( !br )
    {
	sibr = new BinIDRange; br = sibr;
	sibr->start = SI().sampling(false).hrg.start;
	sibr->stop = SI().sampling(false).hrg.stop;
    }

    const EM::ObjectID& id = selhorids[0]; 
    mDynamicCastGet(EM::Horizon3D*,hor3d,em_.getObject(id))
    if ( !hor3d ) return;
    hor3d->ref();

    EM::Horizon3D* hor3d2 = 0;
    if ( selhorids.size() > 1 )
    {
	hor3d2 = (EM::Horizon3D*)(em_.getObject(selhorids[1]));
	hor3d2->ref();
    }

    const BinID step( SI().inlStep(), SI().crlStep() );
    BinID bid;
    for ( bid.inl=br->start.inl; bid.inl<=br->stop.inl; bid.inl+=step.inl )
    {
	for ( bid.crl=br->start.crl; bid.crl<=br->stop.crl; bid.crl+=step.crl )
	{
	    const EM::SubID subid = bid.getSerialized();
	    TypeSet<Coord3> z1pos, z2pos;
	    for ( int idx=hor3d->nrSections()-1; idx>=0; idx-- )
	    {
		const EM::SectionID sid = hor3d->sectionID( idx );
		if ( hor3d->isDefined( sid, subid ) )
		    z1pos += hor3d->getPos( sid, subid );
	    }

	    if ( z1pos.isEmpty() ) continue;

	    if ( !hor3d2 )
	    {
		for ( int posidx=0; posidx<z1pos.size(); posidx++ )
		    bivs.add( bid, z1pos[posidx].z, z1pos[posidx].z );
	    }
	    else
	    {
		for ( int idx=hor3d2->nrSections()-1; idx>=0; idx-- )
		{
		    const EM::SectionID sid = hor3d2->sectionID( idx );
		    if ( hor3d2->isDefined( sid, subid ) )
			z2pos += hor3d2->getPos( sid, subid );
		}

		if ( z2pos.isEmpty() ) continue;

		Interval<float> zintv;
		float dist = 999999;
		for ( int z1idx=0; z1idx<z1pos.size(); z1idx++ )
		{
		    for ( int z2idx=0; z2idx<z2pos.size(); z2idx++ )
		    {
			float dist_ = z2pos[z2idx].z - z1pos[z1idx].z;
			if ( fabs(dist_) < dist )
			{
			    zintv.start = z1pos[z1idx].z;
			    zintv.stop = z2pos[z2idx].z;
			}
		    }
		}

		zintv.sort();
		bivs.add( bid, zintv.start, zintv.stop );
	    }
	}
    }
    
    hor3d->unRef();
    if ( hor3d2 ) hor3d2->unRef();
}
