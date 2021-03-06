/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : somewhere around 1999
-*/


#include "survsubsel.h"
#include "cubesampling.h"
#include "cubesubsel.h"
#include "keystrs.h"
#include "linesubsel.h"
#include "survgeom2d.h"
#include "survgeom3d.h"
#include "trckeyzsampling.h"
#include "uistrings.h"


mDefineEnumUtils(CubeSubSel,SliceType,"Slice Direction") {
    "Inline",
    "CrossLine",
    "ZSlice",
    0
};

template<>
void EnumDefImpl<CubeSubSel::SliceType>::init()
{
    uistrings_ += uiStrings::sInline();
    uistrings_ += uiStrings::sCrossline();
    uistrings_ += uiStrings::sZSlice();
}


bool Survey::SubSel::getInfo( const IOPar& iop, bool& is2d, GeomID& geomid )
{
    is2d = false;
    geomid = GeomID::get3D();

    int igs = (int)OD::VolBasedGeom;
    if ( !iop.get(sKey::GeomSystem(),igs) )
    {
	if ( !iop.get(sKey::SurveyID(),igs) )
	    return false;
	if ( igs < (int)OD::SynthGeom || igs > (int)OD::LineBasedGeom )
	    igs = (int)OD::VolBasedGeom;
    }

    is2d = igs == (int)OD::LineBasedGeom;
    if ( is2d )
	return iop.get( sKey::GeomID(), geomid )
	    && Survey::Geometry::isUsable( geomid );

    return true;
}


void Survey::SubSel::fillParInfo( IOPar& iop, bool is2d, GeomID gid )
{
    const OD::GeomSystem gs = is2d ? OD::LineBasedGeom : OD::VolBasedGeom;
    iop.set( sKey::GeomSystem(), (int)gs );
    if ( is2d )
	iop.set( sKey::GeomID(), gid );
    else
	iop.removeWithKey( sKey::GeomID() );
}


LineHorSubSel* Survey::HorSubSel::asLineHorSubSel()
{
    return mGetDynamicCast( LineHorSubSel*, this );
}


const LineHorSubSel* Survey::HorSubSel::asLineHorSubSel() const
{
    return mGetDynamicCast( const LineHorSubSel*, this );
}


CubeHorSubSel* Survey::HorSubSel::asCubeHorSubSel()
{
    return mGetDynamicCast( CubeHorSubSel*, this );
}


const CubeHorSubSel* Survey::HorSubSel::asCubeHorSubSel() const
{
    return mGetDynamicCast( const CubeHorSubSel*, this );
}


Survey::HorSubSel* Survey::HorSubSel::create( const IOPar& iop )
{
    bool is2d; GeomID gid;
    if ( !getInfo(iop,is2d,gid) )
	return 0;

    HorSubSel* ret = 0;
    if ( is2d )
	ret = new LineHorSubSel( gid );
    else
	ret = new CubeHorSubSel;
    if ( ret )
	ret->usePar( iop );

    return ret;
}


bool Survey::HorSubSel::usePar( const IOPar& iop )
{
    return doUsePar( iop );
}


void Survey::HorSubSel::fillPar( IOPar& iop ) const
{
    fillParInfo( iop, is2D(), geomID() );
    doFillPar( iop );
}


Survey::FullSubSel::FullSubSel( const z_steprg_type& zrg )
    : zss_( zrg )
{
}


LineSubSel* Survey::FullSubSel::asLineSubSel()
{
    return mGetDynamicCast( LineSubSel*, this );
}


const LineSubSel* Survey::FullSubSel::asLineSubSel() const
{
    return mGetDynamicCast( const LineSubSel*, this );
}


CubeSubSel* Survey::FullSubSel::asCubeSubSel()
{
    return mGetDynamicCast( CubeSubSel*, this );
}


const CubeSubSel* Survey::FullSubSel::asCubeSubSel() const
{
    return mGetDynamicCast( const CubeSubSel*, this );
}


Survey::FullSubSel* Survey::FullSubSel::create( const IOPar& iop )
{
    bool is2d; GeomID gid;
    if ( !getInfo(iop,is2d,gid) )
	return 0;

    FullSubSel* ret = 0;
    if ( is2d )
	ret = new LineSubSel( gid );
    else
	ret = new CubeSubSel;
    if ( ret )
	ret->usePar( iop );

    return ret;
}


bool Survey::FullSubSel::usePar( const IOPar& iop )
{
    if ( !horSubSel().usePar(iop) )
	return false;
    zss_.usePar( iop );
    return true;
}


void Survey::FullSubSel::fillPar( IOPar& iop ) const
{
    horSubSel().fillPar( iop );
    zss_.fillPar( iop );
}


LineHorSubSel::LineHorSubSel( GeomID gid )
    : LineHorSubSel( Geometry::get2D(gid) )
{
}


LineHorSubSel::LineHorSubSel( const Geometry2D& geom )
    : LineHorSubSel( geom.trcNrRange() )
{
    geomid_ = geom.geomID();
}


LineHorSubSel::LineHorSubSel( const pos_steprg_type& trcnrrg )
    : Pos::IdxSubSel1D( trcnrrg )
{
}


LineHorSubSel::LineHorSubSel( const TrcKeySampling& tks )
    : Pos::IdxSubSel1D( tks.trcRange() )
    , geomid_(tks.getGeomID())
{
}


bool LineHorSubSel::includes( const LineHorSubSel& oth ) const
{
    const auto trcnrrg = trcNrRange();
    const auto othtrcnrrg = oth.trcNrRange();
    return trcnrrg.step == othtrcnrrg.step
	&& includes( othtrcnrrg.start )
	&& includes( othtrcnrrg.stop );
}


bool LineHorSubSel::doUsePar( const IOPar& inpiop )
{
    const IOPar* iop = &inpiop;
    PtrMan<IOPar> pardestroyer;
    if ( !iop->isPresent(sKey::GeomID()) )
    {
	IOPar* subiop = inpiop.subselect( sKey::Line() );
	pardestroyer = subiop;
	iop = subiop;
    }

    auto geomid = geomid_;
    if ( !iop->get(sKey::GeomID(),geomid)
      || !Survey::Geometry::isPresent(geomid) )
	return false;

    auto trcrg( trcNrRange() );
    *this = LineHorSubSel( geomid ); // clear to no subselection
    iop->get( sKey::FirstTrc(), trcrg.start );
    iop->get( sKey::LastTrc(), trcrg.stop );
    if ( !iop->get(sKey::StepTrc(),trcrg.step) )
	iop->get( sKey::StepCrl(), trcrg.step );
    data_.setOutputPosRange( trcrg );

    return true;
}


void LineHorSubSel::doFillPar( IOPar& iop ) const
{
    const auto trcrg( trcNrRange() );
    iop.set( sKey::FirstTrc(), trcrg.start );
    iop.set( sKey::LastTrc(), trcrg.stop );
    iop.set( sKey::StepTrc(), trcrg.step );
}


CubeHorSubSel::CubeHorSubSel( OD::SurvLimitType slt )
    : CubeHorSubSel( Geometry::get3D(slt) )
{
}


CubeHorSubSel::CubeHorSubSel( const Geometry3D& geom )
    : CubeHorSubSel( geom.inlRange(), geom.crlRange() )
{
}


CubeHorSubSel::CubeHorSubSel( const pos_steprg_type& inlrg,
			      const pos_steprg_type& crlrg )
    : Pos::IdxSubSel2D( inlrg, crlrg )
{
}


CubeHorSubSel::CubeHorSubSel( const HorSampling& hsamp )
    : CubeHorSubSel( hsamp.inlRange(), hsamp.crlRange() )
{
}


CubeHorSubSel::CubeHorSubSel( const TrcKeySampling& tks )
    : CubeHorSubSel( tks.lineRange(), tks.trcRange() )
{
}


Pos::GeomID CubeHorSubSel::geomID() const
{
    return GeomID::get3D();
}


CubeHorSubSel::totalsz_type CubeHorSubSel::totalSize() const
{
    return ((totalsz_type)nrInl()) * nrCrl();
}


void CubeHorSubSel::limitTo( const CubeHorSubSel& oth )
{
    inlSubSel().limitTo( oth.inlSubSel() );
    crlSubSel().limitTo( oth.crlSubSel() );
}


bool CubeHorSubSel::includes( const CubeHorSubSel& oth ) const
{
    const auto inlrg = inlRange();
    const auto crlrg = crlRange();
    return includes( BinID(inlrg.start,crlrg.start) )
	&& includes( BinID(inlrg.start,crlrg.stop) )
	&& includes( BinID(inlrg.stop,crlrg.start) )
	&& includes( BinID(inlrg.stop,crlrg.stop) );
}


bool CubeHorSubSel::doUsePar( const IOPar& iop )
{
    auto rg( inlRange() );
    if ( !iop.get(sKey::InlRange(),rg) )
    {
	iop.get( sKey::FirstInl(), rg.start );
	iop.get( sKey::LastInl(), rg.stop );
	iop.get( sKey::StepInl(), rg.stop );
    }
    setInlRange( rg );
    rg = crlRange();
    if ( !iop.get(sKey::CrlRange(),rg) )
    {
	iop.get( sKey::FirstCrl(), rg.start );
	iop.get( sKey::LastCrl(), rg.stop );
	iop.get( sKey::StepCrl(), rg.stop );
    }
    setCrlRange( rg );
    return true;
}


void CubeHorSubSel::doFillPar( IOPar& iop ) const
{
    const auto inlrg( inlRange() );
    iop.set( sKey::FirstInl(), inlrg.start );
    iop.set( sKey::LastInl(), inlrg.stop );
    iop.set( sKey::StepInl(), inlrg.step );
    const auto crlrg( crlRange() );
    iop.set( sKey::FirstCrl(), crlrg.start );
    iop.set( sKey::LastCrl(), crlrg.stop );
    iop.set( sKey::StepCrl(), crlrg.step );
}



LineSubSel::LineSubSel( GeomID gid )
    : LineSubSel( Survey::Geometry::get2D(gid) )
{
}


LineSubSel::LineSubSel( const Geometry2D& geom )
    : Survey::FullSubSel( geom.zRange() )
    , hss_( geom )
{
    hss_.setGeomID( geom.geomID() );
}


LineSubSel::LineSubSel( const pos_steprg_type& trnrrg )
    : LineSubSel( trnrrg, Survey::Geometry::get3D().zRange() )
{
}


LineSubSel::LineSubSel( const pos_steprg_type& hrg, const z_steprg_type& zrg )
    : Survey::FullSubSel( zrg )
    , hss_( hrg )
{
}


LineSubSel::LineSubSel( const pos_steprg_type& hrg, const ZSubSel& zss )
    : LineSubSel( hrg, zss.outputZRange() )
{
}


LineSubSel::LineSubSel( const TrcKeySampling& tks )
    : LineSubSel( Survey::Geometry::get2D(tks.getGeomID()) )
{
    setTrcNrRange( tks.trcRange() );
}


LineSubSel::LineSubSel( const TrcKeyZSampling& tkzs )
    : LineSubSel( Survey::Geometry::get2D(tkzs.getGeomID()) )
{
    setTrcNrRange( tkzs.hsamp_.trcRange() );
}


const Survey::Geometry2D& LineSubSel::geometry2D() const
{
    return Geometry2D::get( geomID() );
}


CubeSubSel::CubeSubSel( OD::SurvLimitType slt )
    : CubeSubSel( Geometry::get3D(slt) )
{
}


CubeSubSel::CubeSubSel( const Geometry3D& geom )
    : Survey::FullSubSel( geom.zRange() )
    , hss_( geom )
{
}


CubeSubSel::CubeSubSel( const CubeHorSubSel& hss )
    : CubeSubSel( hss, Geometry::get3D().zRange() )
{
}


CubeSubSel::CubeSubSel( const CubeHorSubSel& hss, const z_steprg_type& zrg )
    : Survey::FullSubSel( zrg )
    , hss_( hss )
{
}


CubeSubSel::CubeSubSel( const CubeHorSubSel& hss, const ZSubSel& zss )
    : CubeSubSel( hss, zss.outputZRange() )
{
}


CubeSubSel::CubeSubSel( const pos_steprg_type& inlrg,
			const pos_steprg_type& crlrg )
    : CubeSubSel( inlrg, crlrg, Geometry::get3D().zRange() )
{
}


CubeSubSel::CubeSubSel( const pos_steprg_type& inlrg,
			const pos_steprg_type& crlrg,
			const z_steprg_type& zrg )
    : Survey::FullSubSel( zrg )
    , hss_( inlrg, crlrg )
{
}


CubeSubSel::CubeSubSel( const HorSampling& hsamp )
    : CubeSubSel( hsamp, Geometry::get3D().zRange() )
{
}


CubeSubSel::CubeSubSel( const HorSampling& hsamp, const z_steprg_type& zrg )
    : Survey::FullSubSel( zrg )
    , hss_( hsamp )
{
}


CubeSubSel::CubeSubSel( const CubeSampling& cs )
    : CubeSubSel( cs.hsamp_, cs.zsamp_ )
{
}


CubeSubSel::CubeSubSel( const TrcKeyZSampling& tkzs )
    : Survey::FullSubSel( Survey::Geometry::get3D().zRange() )
{
    setInlRange( tkzs.hsamp_.lineRange() );
    setCrlRange( tkzs.hsamp_.trcRange() );
    setZRange( tkzs.zsamp_ );
}


void CubeSubSel::setRange( const BinID& start, const BinID& stop,
			   const BinID& stp )
{
    setInlRange( pos_steprg_type(start.inl(),stop.inl(),stp.inl()) );
    setCrlRange( pos_steprg_type(start.crl(),stop.crl(),stp.crl()) );
}


CubeSubSel::SliceType CubeSubSel::defaultDir() const
{
    const auto nrinl = nrInl();
    const auto nrcrl = nrCrl();
    const auto nrz = nrZ();
    return nrz < nrinl && nrz < nrcrl	? OD::ZSlice
		    : (nrinl<=nrcrl	? OD::InlineSlice
					: OD::CrosslineSlice);
}


CubeSubSel::size_type CubeSubSel::size( SliceType st ) const
{
    return st == OD::InlineSlice ?	nrInl()
	: (st == OD::CrosslineSlice ?	nrCrl()
				    :	nrZ());
}


void CubeSubSel::getDefaultNormal( Coord3& ret ) const
{
    const auto defdir = defaultDir();
    if ( defdir == OD::InlineSlice )
	ret = Coord3( Survey::Geometry::get3D().binID2Coord().inlDir(), 0 );
    else if ( defdir == OD::CrosslineSlice )
	ret = Coord3( Survey::Geometry::get3D().binID2Coord().crlDir(), 0 );
    else
	ret = Coord3( 0, 0, 1 );
}
