/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : December 2016
-*/


#include "profilebase.h"
#include "iopar.h"
#include "ioman.h"
#include "keystrs.h"
#include "profileposprovider.h"
#include "survinfo.h"
#include "wellman.h"
#include "welldata.h"
#include "welld2tmodel.h"

static const float poseps_ = 1e-6;
static const char* sKeyMarkers()	{ return "Markers"; }


BufferString ProfileBase::name() const
{
    return wellid_.isUdf() ? BufferString("")
			   : BufferString( IOM().nameOf( wellid_ ) );
}


void ProfileBase::copyFrom( const ProfileBase& oth )
{
    pos_ = oth.pos_;
    markers_ = oth.markers_;
    wellid_ = oth.wellid_;
    coord_ = oth.coord_;
}


void ProfileBase::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), typeStr() );
    iop.setYN( sKey::Well(), isWell() );
    iop.set( sKey::Position(), pos_ );
    markers_.fillPar( iop );
}


void ProfileBase::usePar( const IOPar& iop )
{
    iop.get( sKey::Position(), pos_ );
    PtrMan<IOPar> markerspars = iop.subselect( sKeyMarkers() );
    if ( markerspars && !markerspars->isEmpty() )
	markers_.usePar( *markerspars );
    else
	markers_.usePar( iop );
}


void ProfileBase::createMarkersFrom( const ProfileBase* p0,
				     const ProfileBase* p1 )
{
    markers_.setEmpty();
    if ( !p0 ) p0 = p1; if ( !p1 ) p1 = p0;
    if ( !p0 )
	{ pErrMsg("No profile to create markers"); return; }
    if ( p1 == p0 )
	{ markers_ = p0->markers_; return; }

    if ( p0->pos_ > p1->pos_ )	Swap( p0, p1 );
    if ( p0->pos_ > pos_ )	{ markers_ = p0->markers_; return; }
    if ( pos_ > p1->pos_ )	{ markers_ = p1->markers_; return; }

    const bool p0isnear = pos_ - p0->pos_ < p1->pos_ - pos_;
    const ProfileBase& pnear = p0isnear ? *p0 : *p1;
    const ProfileBase& pfar = p0isnear ? *p1 : *p0;
    const float fracnear = mIsEqual(p1->pos_,p0->pos_,poseps_)
	? 0.5f : fabs( (pfar.pos_ - pos_) / (p1->pos_ - p0->pos_) );

    // Create markers as in near profile
    const Well::MarkerSet& mrkrsnear = pnear.markers_;
    ObjectSet<const Well::Marker> mrkrsfar; mrkrsfar.allowNull( true );
    for ( int imrk=0; imrk<mrkrsnear.size(); imrk++ )
    {
	markers_ += new Well::Marker( *pnear.markers_[imrk] );
	mrkrsfar += pfar.markers_.getByName( mrkrsnear[imrk]->name() );
    }

    // Position markers present both in near and far:
    for ( int imrk=0; imrk<markers_.size(); imrk++ )
    {
	const Well::Marker* mrkfar = mrkrsfar[imrk];
	if ( !mrkfar ) continue;

	const Well::Marker* mrknear = mrkrsnear[imrk];
	Well::Marker& mrk = *markers_[imrk];
	mrk.setDah( mrknear->dah()*fracnear + mrkfar->dah()*(1-fracnear) );
    }

    positionMarkersNotInFar( mrkrsnear, mrkrsfar, fracnear );
}


void ProfileBase::positionMarkersNotInFar(
	const Well::MarkerSet& mrkrsnear,
	const ObjectSet<const Well::Marker>& mrkrsfar, float fracnear )
{
    for ( int imrk=0; imrk<markers_.size(); imrk++ )
    {
	if ( mrkrsfar[imrk] )
	    continue;

	int imrkfirstnull = imrk, imrklastnull = markers_.size() - 1;
	for ( int imrk2=imrk+1; imrk2<markers_.size(); imrk2++ )
	    if ( mrkrsfar[imrk2] )
		{ imrklastnull = imrk2-1; break; }

	const bool isattop = imrkfirstnull == 0;
	const bool isatbot = imrklastnull == markers_.size() - 1;
	if ( isattop )
	{
	    if ( isatbot )
		break; // all far markers absent - copy of near markers is OK

	    const float z1 = mrkrsfar[imrklastnull+1]->dah();
	    for ( int imrk2=0; imrk2<=imrklastnull; imrk2++ )
	    {
		Well::Marker& wm = *markers_[imrk2];
		wm.setDah( z1 -  fracnear * (z1 - wm.dah()) );
	    }
	}
	else if ( isatbot )
	{
	    const float z0 = mrkrsfar[imrkfirstnull-1]->dah();
	    for ( int imrk2=imrkfirstnull; imrk2<=imrklastnull; imrk2++ )
	    {
		Well::Marker& wm = *markers_[imrk2];
		wm.setDah( z0 +  fracnear * (wm.dah() - z0) );
	    }
	}
	else
	{
	    const float z0 = mrkrsfar[imrkfirstnull-1]->dah();
	    const float z1 = mrkrsfar[imrklastnull+1]->dah();
	    const float znear0 = mrkrsnear[imrkfirstnull-1]->dah();
	    const float znear1 = mrkrsnear[imrklastnull+1]->dah();
	    const float th = z1 - z0;
	    const float nearth = znear1 - znear0;
	    const bool iszeroth = mIsZero(nearth,0.01);
	    for ( int imrk2=imrkfirstnull; imrk2<=imrklastnull; imrk2++ )
	    {
		Well::Marker& wm = *markers_[imrk2];
		if ( iszeroth )
		    { wm.setDah(z0); continue; }

		const float znear = mrkrsnear[imrk2]->dah();
		const float zrel = (znear-znear0) / nearth;
		const float zfar = z0 + zrel * th;
		wm.setDah( znear*fracnear + zfar*(1-fracnear) );
	    }
	}

	imrk = imrklastnull + 1;
    }
}


bool ProfileBase::moveMarker( int imrk, float newz, bool push,
					  bool pull )
{
    if ( !markers_.validIdx(imrk) )
	return false;
    const float deltaz = newz - markers_[imrk]->dah();
    if ( mIsZero(deltaz,0.0001) )
	return false;

    const Interval<int> toprg( 0, imrk - 1 );
    const Interval<int> botrg( imrk + 1, markers_.size()-1 );
    const bool isdownward = deltaz > 0;
    if ( push )
	shiftMarkerZVals( isdownward ? botrg : toprg, deltaz );
    if ( pull )
	shiftMarkerZVals( isdownward ? toprg : botrg, deltaz );
    ensureNoMarkerZCrossings( isdownward ? botrg : toprg, isdownward, newz );

    markers_[imrk]->setDah( newz );
    return true;
}


void ProfileBase::shiftMarkerZVals( Interval<int> rg, float deltaz )
{
    for ( int imrk=rg.start; imrk<=rg.stop; imrk++ )
	markers_[imrk]->setDah( markers_[imrk]->dah() + deltaz );
}


void ProfileBase::ensureNoMarkerZCrossings( Interval<int> rg,
					bool downward, float edgez )
{
    for ( int imrk=rg.start; imrk<=rg.stop; imrk++ )
    {
	const bool isbelow = markers_[imrk]->dah() < edgez;
	if ( (downward && isbelow) || (!downward && !isbelow) )
	    markers_[imrk]->setDah( edgez );
    }
}


int ProfileModelBase::nrWells( bool unique ) const
{
    int ret = 0;
    TypeSet<MultiID> ids;
    for ( int idx=0; idx<profs_.size(); idx++ )
    {
	if ( profs_[idx]->isWell() )
	{
	    if ( unique && ids.isPresent(profs_[idx]->wellid_) )
		continue;

	    ret++;
	}
    }

    return ret;
}


void ProfileModelBase::removeAll()
{
    while ( size() )
	delete profs_.removeSingle( 0 );
}


void ProfileModelBase::removeCtrlProfiles()
{
    for ( int idx=profs_.size()-1; idx>=0; idx-- )
    {
	if ( profs_[idx]->isCtrl() )
	    delete profs_.removeSingle( idx );
    }
}


void ProfileModelBase::removeProfiles( const char* typestr )
{
    BufferString proftype( typestr );
    for ( int idx=0; idx<profs_.size(); idx++ )
    {
	if ( !typestr || proftype==profs_[idx]->typeStr() )
	    { delete profs_.removeSingle( idx ); idx--; }
    }
}


bool posEqual( float p1, float p2 )
{
    return mIsEqual(p1,p2,1e-2);
}


void ProfileModelBase::removeAtSamePos( int idxtokeep )
{
    for ( int idx=1; idx<profs_.size(); idx++ )
    {
	ProfileBase* p0 = profs_[idx-1]; ProfileBase* p1 = profs_[idx];
	if ( posEqual(p0->pos_,p1->pos_) )
	{
	    // we'll remove the second one unless it's the one to keep ...
	    ProfileBase* prof2del = idx == idxtokeep ? p0 : p1;
	    ProfileBase* othprof = prof2del == p0 ? p1 : p0;
	    // prefer a well anyway
	    if ( prof2del->isWell() && !othprof->isWell() )
		Swap( prof2del, othprof );

	    // ... and: not if it's the last well in this model
	    if ( prof2del->isWell() && nrWells() == 1 )
		prof2del = prof2del == p0 ? p1 : p0;

	    profs_ -= prof2del;
	    delete prof2del;
	    idx--;
	}
    }
}


void ProfileModelBase::removeMarker( const char* markernm )
{
    for ( int iprof=0; iprof<profs_.size(); iprof++ )
    {
	ProfileBase* prof = profs_[iprof];
	const int markeridx = prof->markers_.indexOf( markernm );
	if ( markeridx>= 0 )
	    prof->markers_.removeSingle( markeridx );
    }
}


ProfileFactory& ProfFac()
{
    mDefineStaticLocalObject(ProfileFactory,proffac_,);
    return proffac_;
}


void ProfileFactory::addCreateFunc( CreateFunc crfn, const char* key )
{
    const int keyidx = keys_.indexOf( key );
    if ( keyidx >= 0 )
    {
	createfuncs_[keyidx] = crfn;
	return;
    }

    createfuncs_ += crfn;
    keys_.add( key );
}


ProfileBase* ProfileFactory::create( float pos, const char* keystr )
{
    const int keyidx = keys_.indexOf( keystr );
    if ( keyidx < 0 )
	return createfuncs_.isEmpty() ? 0 : (*createfuncs_[0])( pos );

    return (*createfuncs_[keyidx])( pos );
}


ProfileBase* ProfileFactory::create( const char* keystr )
{
    const int keyidx = keys_.indexOf( keystr );
    if ( keyidx < 0 )
	return createfuncs_.isEmpty() ? 0 : (*createfuncs_[0])( 0.5 );

    return (*createfuncs_[keyidx])( 0.5 );
}

ProfileModelBase::~ProfileModelBase()
{
    deepErase( profs_ );
}


int ProfileModelBase::idxBefore( float pos, bool& isat ) const
{
    isat = false;

    for ( int idx=0; idx<profs_.size(); idx++ )
    {
	const float profpos = profs_[idx]->pos_;
	if ( posEqual(profpos,pos) )
	    { isat = true; return idx; }
	else if ( profpos > pos )
	    return idx - 1;
    }

    return profs_.size() - 1;
}


int ProfileModelBase::set( ProfileBase* prof, bool replacesamepos )
{
    if ( !prof )
	return -1;

    const int oldidx = profs_.indexOf( prof );
    if ( oldidx >= 0 )
    {
	// pos_ may have changed, need to put it at the right spot
	profs_.removeSingle( oldidx );
	return set( prof, replacesamepos );
    }

    bool isat;
    int profidx = idxBefore( prof->pos_, isat );
    if ( isat )
    {
	if ( replacesamepos )
	    delete profs_.removeSingle( profidx-- );
	else
	{
	    while ( isat )
	    {
		const ProfileBase* curprof = profs_[profidx];
		prof->pos_ = curprof->pos_
		    + (prof->pos_ < curprof->pos_ ? -2*poseps_ : 2*poseps_);
		profidx = idxBefore( prof->pos_, isat );
	    }
	}
    }

    profs_.insertAfter( prof, profidx++ );
    return profidx;
}


int ProfileModelBase::nearestIndex( float pos ) const
{
    for ( int idx=0; idx<profs_.size(); idx++ )
    {
	const ProfileBase& prof = *profs_[idx];
	if ( posEqual(prof.pos_,pos) )
	    return idx;

	if ( prof.pos_ > pos )
	{
	    if ( !profs_.validIdx(idx-1) )
		return idx;

	    return pos - profs_[idx-1]->pos_ < prof.pos_ - pos ? idx-1 : idx;
	}
    }

    return -1;
}


Interval<int> ProfileModelBase::getIndexes( float pos, bool noinv ) const
{
    const int sz = size();

    Interval<int> ret( -1, sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	const ProfileBase& prof = *profs_[idx];
	if ( posEqual(prof.pos_,pos) )
	    { ret.start = ret.stop = idx; break; }

	if ( pos < prof.pos_ )
	    { ret.stop = idx; ret.start = idx-1; break; }
	else if ( idx == sz - 1 )
	    ret.start = sz - 1;
    }

    if ( noinv && sz )
    {
	if ( ret.start < 0 ) ret.start = 0;
	if ( ret.stop >= sz ) ret.stop = sz - 1;
    }

    return ret;
}


int ProfileModelBase::indexOf( const MultiID& wid ) const
{
    if ( wid.isEmpty() ) return -1;

    for ( int idx=0; idx<size(); idx++ )
    {
	const ProfileBase& prof = *profs_[idx];
	if ( prof.isWell() && prof.wellid_==wid )
	    return idx;
    }

    return -1;
}


int ProfileModelBase::ctrlIndexBefore( float pos ) const
{
    int idxbfore = -1;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( profs_[idx]->pos_ > pos )
	    return idxbfore;

	if ( profs_[idx]->isCtrl() )
	    idxbfore =	idx;
    }

    return idxbfore;
}


int ProfileModelBase::wellIndexBefore( float pos ) const
{
    int idxbfore = -1;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( profs_[idx]->pos_ > pos )
	    return idxbfore;

	if ( profs_[idx]->isWell() )
	    idxbfore =	idx;
    }

    return idxbfore;
}


int ProfileModelBase::primaryIndexBefore( float pos ) const
{
    int idxbfore = -1;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( profs_[idx]->pos_ > pos )
	    return idxbfore;

	if ( profs_[idx]->isPrimary() )
	    idxbfore =	idx;
    }

    return idxbfore;
}


int ProfileModelBase::ctrlIndexAfter( float pos ) const
{
    int idxafter = -1;
    for ( int idx=size()-1; idx>0; idx-- )
    {
	if ( profs_[idx]->pos_ < pos )
	    return idxafter;

	if ( profs_[idx]->isCtrl() )
	    idxafter =	idx;
    }

    return idxafter;
}


int ProfileModelBase::wellIndexAfter( float pos ) const
{
    int idxafter = -1;
    for ( int idx=size()-1; idx>0; idx-- )
    {
	if ( profs_[idx]->pos_ < pos )
	    return idxafter;

	if ( profs_[idx]->isWell() )
	    idxafter =	idx;
    }

    return idxafter;
}


int ProfileModelBase::primaryIndexAfter( float pos ) const
{
    int idxafter = -1;
    for ( int idx=size()-1; idx>0; idx-- )
    {
	if ( profs_[idx]->pos_ < pos )
	    return idxafter;

	if ( profs_[idx]->isPrimary() )
	    idxafter =	idx;
    }

    return idxafter;
}


float ProfileModelBase::getMaxZ() const
{
    float maxz = 0;
    for ( int idx=0; idx<profs_.size(); idx++ )
    {
	const Well::MarkerSet& mrkrs = profs_[idx]->markers_;
	const int sz = mrkrs.size();
	if ( sz < 1 ) continue;

	const float profmaxz = mrkrs[sz-1]->dah();
	if ( profmaxz > maxz ) maxz = profmaxz;
    }
    return maxz;
}

#define mGetWellData( wd, welld2t, wellid ) \
const Well::Data* wd = Well::MGR().get( wellid ); \
if ( !wd ) \
{ \
    pErrMsg( "Well not read properly" ); \
    return mUdf(float); \
} \
const Well::D2TModel* welld2t = wd->d2TModel();


float ProfileModelBase::getDepthValBetweenWellPos(
	float timeval, float profpos, bool depthintvdss ) const
{
    const int wellprof1idx = wellIndexBefore( profpos );
    const int wellprof2idx = wellIndexAfter( profpos );
    if ( wellprof1idx<0 && wellprof2idx<0 )
    {
	pErrMsg( "Huh! No wellprofile in model." );
	return mUdf(float);
    }

    if ( wellprof2idx< 0 )
    {
	mGetWellData( wd, welld2t, profs_[wellprof1idx]->wellid_ );
	return depthintvdss ? welld2t->getDepth( timeval, wd->track() )
			    : welld2t->getDah( timeval, wd->track() );
    }
    else if ( wellprof1idx < 0 )
    {
	mGetWellData( wd, welld2t, profs_[wellprof2idx]->wellid_ );
	return depthintvdss ? welld2t->getDepth( timeval, wd->track() )
			    : welld2t->getDah( timeval, wd->track() );
    }

    const ProfileBase& prof1 = *profs_[wellprof1idx];
    const ProfileBase& prof2 = *profs_[wellprof2idx];
    const float profrelpos =
	(profpos - prof1.pos_) / (prof2.pos_-prof1.pos_);
    mGetWellData( wd1, well1d2t, prof1.wellid_ );
    mGetWellData( wd2, well2d2t, prof2.wellid_ );
    const float well1z = depthintvdss ? well1d2t->getDepth(timeval,wd1->track())
				      : well1d2t->getDah(timeval,wd1->track());
    const float well2z = depthintvdss ? well2d2t->getDepth(timeval,wd2->track())
				      : well2d2t->getDah(timeval,wd2->track());
    const float res = well1z*(1-profrelpos) + well2z * profrelpos;
    return res;

}


float ProfileModelBase::getDepthVal( float timeval, float profpos,
				     const ProfilePosProvider* posprov,
				     bool depthintvdss ) const
{
    if ( !SI().zIsTime() || mIsUdf(timeval) )
	return timeval;

    if ( !posprov && !nrWells() )
    {
	pErrMsg( "Cannot transform zvals without ProfilePosProvider or Wells" );
	return mUdf(float);
    }

    if ( ztransform_ )
    {
	const Coord poscrd = posprov->getCoord( profpos );
	TrcKey tk;
	tk.setFrom( poscrd );
	return ztransform_->transformTrc( tk, timeval );
    }

    bool isatpos = false;
    const int profidx = idxBefore( profpos, isatpos );
    return isatpos && profs_[profidx]->isWell()
	? getDepthValAtWellPos( timeval, *profs_[profidx], depthintvdss )
	: getDepthValBetweenWellPos( timeval, profpos, depthintvdss );
}


float ProfileModelBase::getDepthValAtWellPos( float timeval,
				     const ProfileBase& prof,
				     bool depthintvdss ) const
{
    if ( prof.wellid_.isUdf() )
	return mUdf(float);

    mGetWellData( wd, welld2t, prof.wellid_ );
    float res = depthintvdss ? welld2t->getDepth( timeval, wd->track() )
			: welld2t->getDah( timeval, wd->track() );
    return res;
}
