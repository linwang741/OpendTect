/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February  2006
 RCS:           $Id: uifingerprintattrib.cc,v 1.3 2006-04-25 14:47:46 cvshelene Exp $

________________________________________________________________________

-*/

#include "uifingerprintattrib.h"
#include "fingerprintattrib.h"
#include "attribdesc.h"
#include "attribsel.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "attribdescset.h"
#include "attribprocessor.h"
#include "attribengman.h"
#include "uiattrsel.h"
#include "uistepoutsel.h"
#include "uiioobjsel.h"
#include "uitable.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "pixmap.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "oddirs.h"
#include "binidvalset.h"
#include "survinfo.h"
#include "transl.h"
#include "pickset.h"
#include "picksettr.h"
#include "runstat.h"

using namespace Attrib;

static const int sInitNrRows = 4;

static const char* valinpstrs[] =
{
	"Mamual",
	"Reference Position",
	"Pickset",
	0
};


static const char* statstrs[] =
{
	"Average",
	"Median",
	"Variance",
	"Min",
	"Max",
	0
};

uiFingerPrintAttrib::uiFingerPrintAttrib( uiParent* p )
	: uiAttrDescEd(p)
    	, ctio_(*mMkCtxtIOObj(PickSetGroup))
{
    refgrp_ = new uiButtonGroup( this, "Get values from", false );
    manualbut_ = new uiRadioButton( refgrp_, "Manual" );
    manualbut_->activated.notify( mCB(this,uiFingerPrintAttrib,refSel ) );
    refposbut_ = new uiRadioButton( refgrp_, "Reference position" );
    refposbut_->activated.notify( mCB(this,uiFingerPrintAttrib,refSel ) );
    picksetbut_ = new uiRadioButton( refgrp_, "Pickset" );
    picksetbut_->activated.notify( mCB(this,uiFingerPrintAttrib,refSel ) );

    refposfld_ = new uiStepOutSel( this, "Trace position`Inl/Crl position");
    refposfld_->attach( alignedBelow, (uiParent*)refgrp_ );
    BufferString zlabel = "Z"; zlabel += SI().getZUnit();
    refposzfld_ = new uiGenInput( this, zlabel );
    refposzfld_->setElemSzPol( uiObject::Small );
    refposzfld_->attach( rightOf, refposfld_ );
    
    CallBack cb = mCB(this,uiFingerPrintAttrib,getPosPush);
    const ioPixmap pm( GetIconFileName("pick.png") );
    getposbut_ = new uiToolButton( this, "", pm, cb );
    getposbut_->attach( rightOf, refposzfld_ );

    picksetfld_ = new uiIOObjSel( this, ctio_, "Pickset file" );
    picksetfld_->attach( alignedBelow, (uiParent*)refgrp_ );
    picksetfld_->display(false);

    statsfld_ = new uiGenInput( this, "PickSet statistic", 
	    		       StringListInpSpec(statstrs) );
    statsfld_->attach( alignedBelow, picksetfld_ );
    statsfld_->display(false);
    
    table_ = new uiTable( this, uiTable::Setup().rowdesc("")
						.rowgrow(true)
						.defrowlbl("")
						.fillcol(true)
						.fillrow(false) );

    const char* collbls[] = { "Reference attributes", "Values", 0 };
    table_->setColumnLabels( collbls );
    table_->setNrRows( sInitNrRows );
    table_->setColumnWidth(0,240);
    table_->setColumnWidth(1,90);
    table_->setRowHeight( -1, 16 );
    table_->setToolTip( "Right-click to add, insert or remove an attribute" );
    table_->attach( alignedBelow, statsfld_ );

    uiLabel* calclbl = 
	new uiLabel( this, "Attribute values at reference position " );
    calclbl->attach( alignedBelow, table_ );
    CallBack cbcalc = mCB(this,uiFingerPrintAttrib,calcPush);
    calcbut_ = new uiPushButton( this, "C&alculate", cbcalc, true );
    calcbut_->attach( rightTo, calclbl );

    setHAlignObj( table_ );
    refSel(0);
}


uiFingerPrintAttrib::~uiFingerPrintAttrib()
{
    delete ctio_.ioobj;
    delete &ctio_;
}


void uiFingerPrintAttrib::initTable()
{
    attribflds_.erase();
    for ( int idx=0; idx<sInitNrRows; idx++ )
    {
	uiAttrSel* attrbox = new uiAttrSel( 0, 0, "" );
	attribflds_ += attrbox;
	table_->setCellObject( RowCol(idx,0), attrbox->attachObj() );
    }

    table_->rowInserted.notify( mCB(this,uiFingerPrintAttrib,insertRowCB) );
    table_->rowDeleted.notify( mCB(this,uiFingerPrintAttrib,deleteRowCB) );
}


void uiFingerPrintAttrib::insertRowCB( CallBacker* cb )
{
    int newrowidx = table_->currentRow();
    uiAttrSel* attrbox = new uiAttrSel( 0, 0, "" );
    attrbox->setDescSet( ads );
    attribflds_.insertAt( attrbox, newrowidx );
    table_->setCellObject( RowCol(newrowidx,0), attrbox->attachObj() );
}


void uiFingerPrintAttrib::deleteRowCB( CallBacker* cb )
{
    int deletedrowidx = table_->currentRow();
    delete attribflds_[deletedrowidx];
    attribflds_.remove( deletedrowidx );
}


void uiFingerPrintAttrib::set2D( bool yn )
{
    refposfld_->set2D( yn );
}


bool uiFingerPrintAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),FingerPrint::attribName()) )
	return false;

    mIfGetBinID( FingerPrint::refposStr(), refpos, refposfld_->setBinID(refpos))
    mIfGetFloat( FingerPrint::refposzStr(), refposz,
	    	 refposzfld_->setValue( refposz ) );

    mIfGetString( FingerPrint::picksetStr(), pick, 
	    	  picksetfld_->setInputText(pick) )

    mIfGetInt( FingerPrint::reftypeStr(), type, refgrp_->selectButton(type) )

    refSel(0);
    
    table_->clearTable();
    table_->setNrRows( sInitNrRows );
    initTable();

    int nrvals = 0;
    if ( desc.getParam( FingerPrint::valStr() ) )
    {
	mDescGetConstParamGroup(FloatParam,valueset,desc,FingerPrint::valStr())
	nrvals = valueset->size();
    }

    while ( nrvals > table_->nrRows() )
	table_->insertRows(0);

    if ( desc.getParam( FingerPrint::valStr() ) )
    {
	mDescGetConstParamGroup(FloatParam,valueset,desc,FingerPrint::valStr());
	for ( int idx=0; idx<valueset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*valueset)[idx];
	    table_->setValue( RowCol(idx,1), param.getfValue(0) );
	}
    }

    return true;
}


bool uiFingerPrintAttrib::setInput( const Desc& desc )
{
    const int nrflds = table_->nrRows();
    for ( int idx=0; idx<nrflds; idx++ )
	putInp( attribflds_[idx], desc, idx );
    
    return true;
}


bool uiFingerPrintAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(), FingerPrint::attribName()) )
	return false;

    mSetInt( FingerPrint::reftypeStr(), refgrp_->selectedId() );
    const int refgrpval = refgrp_->selectedId();

    if ( refgrpval == 1 )
    {
	mSetBinID( FingerPrint::refposStr(), refposfld_->binID() );
	mSetFloat( FingerPrint::refposzStr(), refposzfld_->getfValue() );
    }
    else if ( refgrpval == 2 )
    {
	mSetInt( FingerPrint::statstypeStr(), statsfld_->getIntValue() );
	mSetString( FingerPrint::picksetStr(), picksetfld_->getInput() );
    }

    TypeSet<float> values;
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	float val = table_->getfValue( RowCol(idx,1) );
	if ( mIsUdf(val) ) continue;

	values += val;
    }

    mDescGetParamGroup(FloatParam,valueset,desc,FingerPrint::valStr())
    valueset->setSize( values.size() );
    for ( int idx=0; idx<values.size(); idx++ )
    {
	ValParam& valparam = (ValParam&)(*valueset)[idx];
	valparam.setValue(values[idx] );
    }
    
    return true;
}


bool uiFingerPrintAttrib::getInput( Desc& desc )
{
    for ( int idx=0; idx<attribflds_.size(); idx++ )
    {
	attribflds_[idx]->processInput();
	fillInp( attribflds_[idx], desc, idx );
    }
    return true;
}


void uiFingerPrintAttrib::refSel( CallBacker* )
{
    const bool refbutchecked = refposbut_->isChecked();
    const bool pickbutchecked = picksetbut_->isChecked();
    refposfld_->display( refbutchecked );
    refposzfld_->display( refbutchecked );
    getposbut_->display( refbutchecked );
    picksetfld_->display( pickbutchecked );
    statsfld_->display( pickbutchecked );
}


void uiFingerPrintAttrib::getPosPush(CallBacker*)
{
}


void uiFingerPrintAttrib::calcPush(CallBacker*)
{
    ObjectSet<BinIDValueSet> values;
    
    if ( refgrp_->selectedId() == 1 )
    {
	BinID refpos_ = refposfld_->binID();
	float refposz_ = refposzfld_->getfValue() / SI().zFactor();

	if ( mIsUdf(refpos_.inl) || mIsUdf(refpos_.crl) || mIsUdf(refposz_) )
	{
	    //see if ok in 2d
	    uiMSG().error( "Please fill in the position fields first" );
	    return;
	}

	BinIDValueSet* bidvalset = new BinIDValueSet( 1, false );
	bidvalset->add( refpos_, refposz_ );
	values += bidvalset;
    }
    else if ( refgrp_->selectedId() == 2 )
    {
	BufferString errmsg;
	PickSetGroup psg;
	picksetfld_->processInput();
	const IOObj* ioobj = picksetfld_->ctxtIOObj().ioobj;
	if ( !ioobj )
	{
	    uiMSG().error( "Please choose a pickset first" );
	    return;
	}
	BufferStringSet ioobjids;
	ioobjids.add( ioobj->key() );
	PickSetGroupTranslator::createBinIDValueSets( ioobjids, values );
    }
    else
    {
	uiMSG().error( "In manual mode, values should be filled by the user" );
	return;
    }
    
    BufferString errmsg;
    PtrMan<Attrib::EngineMan> aem = createEngineMan();
    PtrMan<Processor> proc = aem->createLocationOutput( errmsg, values );
    if ( !proc )
    {
	uiMSG().error(errmsg);
	return;
    }

    int res =1;
    while ( res == 1 )
    {
	res = proc->nextStep();
	if ( res == -1 )
	{
	    uiMSG().error( "Cannot reach next position" );
	    return;
	}
    }

    showValues( values[0] );
    deepErase(values);
}


EngineMan* uiFingerPrintAttrib::createEngineMan()
{
    EngineMan* aem = new EngineMan;
    
    TypeSet<SelSpec> attribspecs;
    for ( int idx=0; idx<ads->nrDescs(); idx++ )
    {
	SelSpec sp( 0, ads->desc(idx)->id() );
	attribspecs += sp;
    }

    aem->setAttribSet( ads );
    aem->setAttribSpecs( attribspecs );

    return aem;
    
}


void uiFingerPrintAttrib::showValues( BinIDValueSet* bidvalset )
{
    TypeSet<float> vals;
    vals.setSize(ads->nrDescs());
    if ( bidvalset->totalSize() == 1 )
    {
	float* tmpvals = bidvalset->getVals( bidvalset->getPos(0) );
	for ( int idx=0; idx<ads->nrDescs(); idx++ )
	    vals[idx] = tmpvals[idx+1];
    }
    else
    {
	ObjectSet< RunningStatistics<float> > statsset;
	for ( int idx=0; idx<ads->nrDescs(); idx++ )
	    statsset += new RunningStatistics<float>;

	for ( int idsz=0; idsz<bidvalset->totalSize(); idsz++ )
	{
	    float* values = bidvalset->getVals( bidvalset->getPos(idsz) );
	    for ( int idx=0; idx<ads->nrDescs(); idx++ )
		*(statsset[idx]) += values[idx+1];
	}
	
	for ( int idx=0; idx<ads->nrDescs(); idx++ )
	{
	    switch ( statsfld_->getIntValue() )
	    {
		case 0:
		    {
			vals[idx] = statsset[idx]->mean();
			break;
		    }
		case 1:
		    {
			vals[idx] = statsset[idx]->median(); 
			break;
		    }
		case 2:
		    {
			vals[idx] = statsset[idx]->variance(); 
			break;
		    }
		case 3:
		    {
			vals[idx] = statsset[idx]->min(); 
			break;
		    }
		case 4:
		    {
			vals[idx] = statsset[idx]->max();
			break;
		    }
	    }
	}
	deepErase( statsset );
    }
	
    for ( int idx=0; idx<attribflds_.size(); idx++ )
    {
	BufferString inp = attribflds_[idx]->getInput();
	for ( int idxdesc=0; idxdesc<ads->nrDescs(); idxdesc++ )
	{
	    if ( !strcmp( inp, ads->desc(idxdesc)->userRef() ) )
		table_->setValue( RowCol(idx,1), vals[idxdesc] );
	}
    }
}



