/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2002
________________________________________________________________________

-*/



#include "uipositionattrib.h"
#include "positionattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"
#include "od_helpids.h"

using namespace Attrib;

static const char* opstrs[] =
{
	"Min",
	"Max",
	"Median",
	0
};

mInitGrpDefAttribUI(uiPositionAttrib,Position,uiStrings::sPosition(),
		    sPositionGrp())


uiPositionAttrib::uiPositionAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mPositionAttribHelpID) )

{
    inpfld = createInpFld( is2d, tr("Input attribute") );

    stepoutfld = new uiStepOutSel( this, is2d );
    stepoutfld->setFieldNames( "Inl Stepout", "Crl Stepout" );
    stepoutfld->attach( alignedBelow, inpfld );

    gatefld = new uiGenInput( this, gateLabel(),
			      FloatInpIntervalSpec().setName("Z start",0)
						    .setName("Z stop",1) );
    gatefld->attach( alignedBelow, stepoutfld );

    steerfld = new uiSteeringSel( this, 0, is2d );
    steerfld->steertypeSelected_.notify(
				mCB(this,uiPositionAttrib,steerTypeSel) );
    steerfld->attach( alignedBelow, gatefld );

    operfld = new uiGenInput( this, uiStrings::sOperator(),
                              StringListInpSpec(opstrs) );
    operfld->attach( alignedBelow, steerfld );

    outfld = createInpFld( is2d, tr("Output attribute") );
    outfld->attach( alignedBelow, operfld );

    setHAlignObj( inpfld );
}


bool uiPositionAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != Position::attribName() )
	return false;

    mIfGetFloatInterval( Position::gateStr(), gate, gatefld->setValue(gate) );
    mIfGetBinID( Position::stepoutStr(), stepout,
		 stepoutfld->setBinID(stepout) );
    mIfGetEnum( Position::operStr(), oper, operfld->setValue(oper) );

    return true;
}


bool uiPositionAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    putInp( outfld, desc, 1 );
    putInp( steerfld, desc, 2 );

    return true;
}


bool uiPositionAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != Position::attribName() )
	return false;

    mSetFloatInterval( Position::gateStr(), gatefld->getFInterval() );
    mSetBinID( Position::stepoutStr(), stepoutfld->getBinID() );
    mSetEnum( Position::operStr(), operfld->getIntValue() );
    mSetBool( Position::steeringStr(), steerfld->willSteer() );

    return true;
}


uiRetVal uiPositionAttrib::getInput( Desc& desc )
{
    uiRetVal uirv = fillInp( inpfld, desc, 0 );
    uirv.add( fillInp( outfld, desc, 1 ) );
    uirv.add( fillInp( steerfld, desc, 2 ) );
    return uirv;
}


void uiPositionAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr(), Position::gateStr() );
    params += EvalParam( stepoutstr(), Position::stepoutStr() );
}


void uiPositionAttrib::steerTypeSel( CallBacker* )
{
}
