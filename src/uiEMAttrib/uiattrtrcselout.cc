/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id: uiattrtrcselout.cc,v 1.28 2007-12-04 12:25:06 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiattrtrcselout.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "ctxtioobj.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "multiid.h"
#include "ptrman.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "survinfo.h"

#include "uiattrsel.h"
#include "uibinidsubsel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseissel.h"


using namespace Attrib;

uiAttrTrcSelOut::uiAttrTrcSelOut( uiParent* p, const DescSet& ad,
				  const NLAModel* n, const MultiID& mid, 
				  bool usesinglehor )
    : uiFullBatchDialog(p,Setup("Create Horizon related cube output")
	    		  .procprognm("process_attrib_em"))
    , ctio_( mkCtxtIOObjHor( ad.is2D() ) )
    , ctio2_( mkCtxtIOObjHor( ad.is2D() ) )
    , ctioout_(*mMkCtxtIOObj(SeisTrc))
    , ads_(const_cast<DescSet&>(ad))
    , nlamodel_(n)
    , nlaid_(mid)
    , usesinglehor_(usesinglehor)
    , extraztopfld_(0)
    , extrazbotfld_(0)
    , gatefld_(0)
    , mainhorfld_(0)
    , widthfld_(0)
    , addwidthfld_(0)
    , interpfld_(0)
    , nrsampfld_(0)
    , xparsdlg_(0)
{
    if ( usesinglehor_)
	setHelpID( usesinglehor_ ? "104.4.2" : "104.4.1" );

    setTitleText( "" );

    if ( usesinglehor_ )
	createSingleHorUI();
    else
	createTwoHorUI();
    
    addStdFields( false, true );

    objSel(0);
    if ( usesinglehor_ && !ads_.is2D() )
	interpSel(0);

    if ( usesinglehor_ || ads_.is2D() )
	cubeBoundsSel(0);
}


void uiAttrTrcSelOut::createSingleHorUI()
{
    createAttrFld( uppgrp_ );

    ctio_.ctxt.forread = true;
    objfld_ = new uiIOObjSel( uppgrp_, ctio_, "Calculate along surface" );
    objfld_->attach( alignedBelow, attrfld_ );
    objfld_->selectiondone.notify( mCB(this,uiAttrTrcSelOut,objSel) );

    createSubSelFld( uppgrp_ );
    createZIntervalFld( uppgrp_ );
    createOutsideValFld( uppgrp_ );
    if ( !ads_.is2D() )
    {
	createInterpFld( uppgrp_ );
	createNrSampFld( uppgrp_ );
    }
    createCubeBoundsFlds( uppgrp_ );
    createOutputFld( uppgrp_ );

    uppgrp_->setHAlignObj( attrfld_ );
}


void uiAttrTrcSelOut::createTwoHorUI()
{
    xparsdlg_ = new uiDialog( this, uiDialog::Setup("Extra options dialog",
					    	    "Select extra options",
						    "104.4.1" ) );
    xparsdlg_->finaliseDone.notify( mCB(this,uiAttrTrcSelOut,extraDlgDone) );
    
    createAttrFld( uppgrp_ );
    
    ctio_.ctxt.forread = true;
    objfld_ = new uiIOObjSel( uppgrp_, ctio_,"Calculate between top surface:");
    objfld_->attach( alignedBelow, attrfld_ );
    
    ctio2_.ctxt.forread = true;
    obj2fld_ = new uiIOObjSel( uppgrp_, ctio2_, "and bottom surface:" );
    obj2fld_->attach( alignedBelow, objfld_ );
    obj2fld_->selectiondone.notify( mCB(this,uiAttrTrcSelOut,objSel) );

    createExtraZTopFld( uppgrp_ );
    createExtraZBotFld( uppgrp_ );
    createSubSelFld( uppgrp_ );
    createOutsideValFld( uppgrp_ );
    if ( !ads_.is2D() )
    {
	createInterpFld( xparsdlg_ );
	createNrSampFld( xparsdlg_ );
	createAddWidthFld( xparsdlg_ );
	createWidthFld( xparsdlg_ );
	createMainHorFld( xparsdlg_ );
    }

    createCubeBoundsFlds( ads_.is2D() ? (uiParent*) uppgrp_
	    			      : (uiParent*) xparsdlg_ );
    createOutputFld( uppgrp_ );

    if ( !ads_.is2D() )
    {
	CallBack cb = mCB(this,uiAttrTrcSelOut,extraParsCB);
	uiPushButton* extrabut =
			new uiPushButton( uppgrp_, "Extra options", cb, true );
	extrabut->attach( alignedBelow, outpfld_ );
    }
    
    uppgrp_->setHAlignObj( attrfld_ );
}


uiAttrTrcSelOut::~uiAttrTrcSelOut()
{
    delete ctio_.ioobj;
    delete &ctio_;

    if ( !usesinglehor_ )
    {
	delete ctio2_.ioobj;
	delete &ctio2_;
    }
    
    delete ctioout_.ioobj;
    delete &ctioout_;
}


void uiAttrTrcSelOut::createAttrFld( uiParent* prnt )
{
    attrfld_ = new uiAttrSel( prnt, &ads_, ads_.is2D(), "Quantity to output" );
    attrfld_->selectiondone.notify( mCB(this,uiAttrTrcSelOut,attrSel) );
    attrfld_->setNLAModel( nlamodel_ );
}


void uiAttrTrcSelOut::createZIntervalFld( uiParent* prnt )
{
    const char* gatelabel = "Z Interval required around surfaces";
    gatefld_ = new uiGenInput( prnt, gatelabel, FloatInpIntervalSpec() );
    gatefld_->attach( alignedBelow, subselfld_ );
    uiLabel* lbl = new uiLabel( prnt, SI().getZUnit() );
    lbl->attach( rightOf, (uiObject*)gatefld_ );
}


void uiAttrTrcSelOut::createExtraZTopFld( uiParent* prnt )
{
    extraztopfld_ = new uiGenInput( prnt, "plus", IntInpSpec() );
    extraztopfld_->setElemSzPol(uiObject::Small);
    extraztopfld_->attach( rightOf, objfld_ );
    extraztopfld_->setValue(0);
    uiLabel* toplbl = new uiLabel( prnt, SI().getZUnit() );
    toplbl->attach( rightOf, extraztopfld_ );
}


void uiAttrTrcSelOut::createExtraZBotFld( uiParent* prnt )
{
    extrazbotfld_ = new uiGenInput( prnt, "plus", IntInpSpec() );
    extrazbotfld_->setElemSzPol(uiObject::Small);
    extrazbotfld_->attach( rightOf, obj2fld_ );
    extrazbotfld_->setValue(0);
    uiLabel* botlbl = new uiLabel( prnt, SI().getZUnit() );
    botlbl->attach( rightOf, extrazbotfld_ );
}


void uiAttrTrcSelOut::createSubSelFld( uiParent* prnt )
{
    subselfld_ = new uiBinIDSubSel( prnt, uiBinIDSubSel::Setup()
	    				   .withz(false).withstep(true) );
    subselfld_->attach( alignedBelow, usesinglehor_ ? (uiGroup*)objfld_
	    					    : (uiGroup*)obj2fld_ );
}


void uiAttrTrcSelOut::createOutsideValFld( uiParent* prnt )
{
    const char* outsidevallabel = "Value outside computed area";
    outsidevalfld_ = new uiGenInput( prnt, outsidevallabel, FloatInpSpec() );
    outsidevalfld_->attach( alignedBelow, usesinglehor_ ? (uiGroup*)gatefld_ 
	    					       : (uiGroup*)subselfld_ );
    outsidevalfld_->setValue(0);
}


void uiAttrTrcSelOut::createInterpFld( uiParent* prnt )
{
    const char* interplbl = "Interpolate surfaces";
    const char* flbl = "Full interpolation";
    const char* plbl = "Partial interpolation";
    interpfld_ = new uiGenInput( prnt, interplbl, BoolInpSpec(true,flbl,plbl) );
    interpfld_->setValue( true );
    interpfld_->setWithCheck( true );
    interpfld_->setChecked( true );
    interpfld_->valuechanged.notify( mCB(this,uiAttrTrcSelOut,interpSel) );
    interpfld_->checked.notify( mCB(this,uiAttrTrcSelOut,interpSel) );
    if ( usesinglehor_ )
	interpfld_->attach( alignedBelow, outsidevalfld_ );
}


void uiAttrTrcSelOut::createNrSampFld( uiParent* prnt )
{
    const char* nrsamplabel = "Interpolate if hole is smaller than N traces";
    nrsampfld_ = new uiGenInput( prnt, nrsamplabel, IntInpSpec() );
    nrsampfld_->attach( alignedBelow, interpfld_ );
}


void uiAttrTrcSelOut::createAddWidthFld( uiParent* prnt )
{
    BufferString zlabel = createAddWidthLabel();
    addwidthfld_ = new uiGenInput( prnt, zlabel, BoolInpSpec(true) );
    addwidthfld_->setValue( false );
    addwidthfld_->attach( alignedBelow, nrsampfld_ );
    addwidthfld_->valuechanged.notify( mCB(this,uiAttrTrcSelOut,
					  extraWidthSel) );
}


void uiAttrTrcSelOut::createWidthFld( uiParent* prnt )
{
    widthfld_ = new uiGenInput( prnt,"Extra interval length", FloatInpSpec() );
    widthfld_->attach( alignedBelow, addwidthfld_ );
    widthfld_->checked.notify( mCB(this,uiAttrTrcSelOut,extraWidthSel) );
}


void uiAttrTrcSelOut::createMainHorFld( uiParent* prnt )
{
    const char* mainhorlabel = "Main surface";
    mainhorfld_ = new uiGenInput( prnt, mainhorlabel, 
	    			 BoolInpSpec(true,"Top","Bottom") );
    mainhorfld_->attach( alignedBelow, widthfld_ );
}

   
void uiAttrTrcSelOut::createCubeBoundsFlds( uiParent* prnt )
{
    const char* choicelbl = "Define Z limits for the output cube";
    setcubeboundsfld_ = new uiGenInput ( prnt, choicelbl, BoolInpSpec(true) );
    setcubeboundsfld_->attach( alignedBelow,
	    		       mainhorfld_ ? mainhorfld_ 
					   : nrsampfld_ ? nrsampfld_
					   		: outsidevalfld_ );
    setcubeboundsfld_->setValue( false );
    setcubeboundsfld_->valuechanged.notify( mCB(this,uiAttrTrcSelOut,
		                                cubeBoundsSel) );

    cubeboundsfld_ = new uiGenInput ( prnt, "Z start/stop", 
	    			      FloatInpIntervalSpec() );
    cubeboundsfld_->attach( alignedBelow, setcubeboundsfld_ );
}


void uiAttrTrcSelOut::createOutputFld( uiParent* prnt )
{
    ctioout_.ctxt.forread = false;
    ctioout_.ctxt.parconstraints.set( sKey::Type, sKey::Steering );
    ctioout_.ctxt.includeconstraints = false;
    ctioout_.ctxt.allowcnstrsabsent = true;
    outpfld_ = new uiSeisSel( prnt, ctioout_,
	    		      Seis::SelSetup(ads_.is2D()).selattr(false) );
    bool noadvdlg = usesinglehor_ || ads_.is2D();
    outpfld_->attach( alignedBelow, noadvdlg ? cubeboundsfld_ : outsidevalfld_);
}


bool uiAttrTrcSelOut::prepareProcessing()
{
    if ( !objfld_->commitInput(false) )
    {
	uiMSG().error( "Please select first surface" );
	return false;
    }

    if ( !usesinglehor_ && !obj2fld_->commitInput(false) )
    {
	uiMSG().error( "Please select second surface" );
	return false;
    }

    attrfld_->processInput();
    if ( attrfld_->attribID() < 0 && attrfld_->outputNr() < 0 )
    {
	uiMSG().error( "Please select the output quantity" );
	return false;
    }

    bool haveoutput = outpfld_->commitInput( true );
    if ( !haveoutput || !ctioout_.ioobj )
    {
	uiMSG().error( "Please select output" );
	return false;
    }
    
    return true;
}


bool uiAttrTrcSelOut::fillPar( IOPar& iopar )
{
    DescID nladescid( -1, true );
    if ( nlamodel_ && attrfld_->outputNr() >= 0 )
    {
	if ( !nlaid_ || !(*nlaid_) )
	{ 
	    uiMSG().message( "NN needs to be stored before creating volume" ); 
	    return false; 
	}
	if ( !addNLA( nladescid ) )	return false;
    }

    const DescID targetid = nladescid < 0 ? attrfld_->attribID() : nladescid;
    DescSet* clonedset = ads_.optimizeClone( targetid );
    if ( !clonedset )
	return false;

    IOPar attrpar( "Attribute Descriptions" );
    clonedset->fillPar( attrpar );
    
    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
        const char* nm = attrpar.getKey( idx );
        iopar.add( IOPar::compKey(SeisTrcStorOutput::attribkey,nm),
		   attrpar.getValue(idx) );
    }

    BufferString key;
    BufferString keybase = Output::outputstr; keybase += ".1.";
    key = keybase; key += sKey::Type;
    iopar.set( key, Output::tskey );

    key = keybase; key += SeisTrcStorOutput::attribkey;
    key += "."; key += DescSet::highestIDStr();
    iopar.set( key, 1 );

    key = keybase; key += SeisTrcStorOutput::attribkey; key += ".0";
    iopar.set( key, nladescid < 0 ? attrfld_->attribID().asInt() 
	    			  : nladescid.asInt() );

    key = keybase; key += SeisTrcStorOutput::seisidkey;
    iopar.set( key, ctioout_.ioobj->key() );

    BufferString outnm = outpfld_->getInput();
    iopar.set( "Target value", outnm );

    keybase = sKey::Geometry; keybase += ".";
    key = keybase; key += LocationOutput::surfidkey; key += ".0";
    iopar.set( key, ctio_.ioobj->key() );

    if ( !usesinglehor_ )
    {
	key = keybase; key += LocationOutput::surfidkey; key += ".1";
	iopar.set( key, ctio2_.ioobj->key() );
    }

    PtrMan<IOPar> subselpar = new IOPar;
    subselfld_->fillPar( *subselpar );

    HorSampling horsamp; horsamp.usePar( *subselpar );
    if ( horsamp.isEmpty() )
	getComputableSurf( horsamp );

    key = keybase; key += SeisTrcStorOutput::inlrangekey;
    iopar.set( key, horsamp.start.inl, horsamp.stop.inl );

    key = keybase; key += SeisTrcStorOutput::crlrangekey;
    iopar.set( key, horsamp.start.crl, horsamp.stop.crl );

    Interval<float> zinterval;
    if ( gatefld_ )
	zinterval = gatefld_->getFInterval();
    else
    {
	zinterval.start = extraztopfld_->getfValue();
	zinterval.stop = extrazbotfld_->getfValue();
    }
    
    BufferString gatestr = zinterval.start; gatestr += "`";
    gatestr += zinterval.stop;
    
    key = keybase; key += "ExtraZInterval";
    iopar.set( key, gatestr );

    key = keybase; key += "Outside Value";
    iopar.set( key, outsidevalfld_->getfValue() );

    int nrsamp = 0;
    if ( interpfld_ && interpfld_->isChecked() )
	nrsamp = interpfld_->getBoolValue() ? mUdf(int) 
	    				   : nrsampfld_->getIntValue();
    
    key = keybase; key += "Interpolation Stepout";
    iopar.set( key, nrsamp );

    if ( !usesinglehor_ && addwidthfld_->getBoolValue() )
    {
	key = keybase; key += "Artificial Width";
	iopar.set( key, widthfld_->getfValue() );
	
	key = keybase; key += "Leading Horizon";
	iopar.set( key, mainhorfld_->getBoolValue()? 1 : 2 );
    }
    
    Interval<float> cubezbounds;
    cubezbounds = setcubeboundsfld_->getBoolValue()
				? cubeboundsfld_->getFInterval()
				: Interval<float>( mUdf(float), mUdf(float) );
    BufferString zboundsstr = cubezbounds.start; zboundsstr += "`";
    zboundsstr += cubezbounds.stop;
   
    if ( !mIsUdf(cubezbounds.start) )
    {
	key = keybase; key += "Z Boundaries";
	iopar.set( key, zboundsstr );
    }
    
    ads_.removeDesc( nladescid );
    delete clonedset;

    return true;
}


void uiAttrTrcSelOut::getComputableSurf( HorSampling& horsampling )
{
    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( ctio_.ioobj->key(), sd );

    Interval<int> inlrg(sd.rg.start.inl, sd.rg.stop.inl);
    Interval<int> crlrg(sd.rg.start.crl, sd.rg.stop.crl);

    if ( !usesinglehor_ )
    {
	EM::SurfaceIOData sd2;
	EM::EMM().getSurfaceData( ctio2_.ioobj->key(), sd2 );

	Interval<int> inlrg2(sd2.rg.start.inl, sd2.rg.stop.inl);
	Interval<int> crlrg2(sd2.rg.start.crl, sd2.rg.stop.crl);

	inlrg.start = mMAX( inlrg.start, inlrg2.start);
	inlrg.stop = mMIN( inlrg.stop, inlrg2.stop);
	crlrg.start = mMAX( crlrg.start, crlrg2.start);
	crlrg.stop = mMIN( crlrg.stop, crlrg2.stop);
    }

    horsampling.set( inlrg, crlrg );
}


BufferString uiAttrTrcSelOut::createAddWidthLabel()
{
    BufferString zlabel = "Add fixed interval length to main surface \n";
    BufferString ifinterp = "in case of interpolation conflict";
    BufferString ifnointerp = "in case of holes in second surface";
    BufferString text = zlabel;
    text += interpfld_->isChecked()? ifinterp : ifnointerp;
    return text;
}


void uiAttrTrcSelOut::attrSel( CallBacker* cb )
{
    setParFileNmDef( attrfld_->getInput() );
}


void uiAttrTrcSelOut::objSel( CallBacker* cb )
{
    if ( !objfld_->commitInput(false) || 
	 ( !usesinglehor_ && !obj2fld_->commitInput(false) ) ) 
	return;
    
    HorSampling horsampling;
    getComputableSurf( horsampling );
    uiBinIDSubSel::Data subseldata = subselfld_->data();
    subseldata.cs_.hrg = horsampling; subselfld_->setData( subseldata );
}


void uiAttrTrcSelOut::interpSel( CallBacker* cb )
{
    nrsampfld_->display( interpfld_->isChecked() ? !interpfld_->getBoolValue() 
	    				       : false );
    if ( !addwidthfld_ )
	return;

    BufferString text = createAddWidthLabel();
    addwidthfld_->setTitleText(text);
}


void uiAttrTrcSelOut::extraWidthSel( CallBacker* cb )
{
    if ( !addwidthfld_ )
	return;
    widthfld_->display( addwidthfld_->getBoolValue(), false );
    mainhorfld_->display( addwidthfld_->getBoolValue(), false );
}


void uiAttrTrcSelOut::cubeBoundsSel( CallBacker* cb )
{
    cubeboundsfld_->display( setcubeboundsfld_->getBoolValue() );
}


#define mErrRet(str) { uiMSG().message( str ); return false; }

bool uiAttrTrcSelOut::addNLA( DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid_;

    const int outpnr = attrfld_->outputNr();
    BufferString errmsg;
    EngineMan::addNLADesc( defstr, id, ads_, outpnr, nlamodel_, errmsg);
    if ( errmsg.size() )
	mErrRet( errmsg );

    return true;
}


void uiAttrTrcSelOut::extraParsCB( CallBacker* cb )
{
    xparsdlg_->go();
}


void uiAttrTrcSelOut::extraDlgDone( CallBacker* cb )
{
    if ( !ads_.is2D() )
    {
	interpSel(0);
	extraWidthSel(0);
    }

    cubeBoundsSel(0);
}


CtxtIOObj& uiAttrTrcSelOut::mkCtxtIOObjHor( bool is2d )
{
    if ( is2d )
	return *mMkCtxtIOObj( EMAnyHorizon );
    else
	return *mMkCtxtIOObj( EMHorizon3D );
}

