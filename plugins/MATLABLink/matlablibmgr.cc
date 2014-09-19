/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "matlablibmgr.h"

#include "file.h"
#include "filepath.h"
#include "ptrman.h"
#include "sharedlibs.h"

#ifdef HAS_MATLAB

#include "matlabarray.h"

extern "C" {
    void mclmcrInitialize();
    bool mclInitializeApplication(const char**,size_t);
    void mclTerminateApplication();

    typedef bool (*initfn)();			// XX_cInitialize(void)
    typedef void (*termfn)();			// XX_cTerminate(void)

    typedef bool (*getparfn)(int,mxArray**);	// mlfOd_getparameters
    typedef bool (*getnrinfn)(int,mxArray**);	// mlfOd_getnrinputs
    typedef bool (*getnroutfn)(int,mxArray**);	// mlfOd_getnroutputs
};

static const char* sGetParametersStr = "mlfOd_getparameters";
static const char* sGetNrInputsStr = "mlfOd_getnrinputs";
static const char* sGetNrOutputsStr = "mlfOd_getnroutputs";

#endif

// MatlabLibAccess
MatlabLibAccess::MatlabLibAccess( const char* libfnm )
    : shlibfnm_(libfnm)
    , inited_(false)
    , sla_(0)
{
}


MatlabLibAccess::~MatlabLibAccess()
{
    if ( inited_ ) terminate();
    if ( sla_ ) sla_->close();
    delete sla_;
}


#ifdef HAS_MATLAB
static BufferString getFnm( const char* libfnm, bool init )
{
    FilePath fp( libfnm );
    fp.setExtension( "" );
    return BufferString ( fp.fileName(), init ? "Initialize" : "Terminate" );
}


#define mErrRet(msg) { errmsg_ = msg; return false; }

bool MatlabLibAccess::init()
{
    if ( inited_ ) return true;

    if ( !File::exists(shlibfnm_) )
	mErrRet( tr("Cannot find shared library %1").arg(shlibfnm_) );

    sla_ = new SharedLibAccess( shlibfnm_ );
    if ( !sla_->isOK() )
	return false;

    const BufferString initfnm = getFnm( shlibfnm_, true );
    initfn ifn = (initfn)sla_->getFunction( initfnm.buf() );
    if ( !ifn )
	mErrRet( tr("Cannot find function %1").arg(initfnm) );

    if ( !(*ifn)() )
	mErrRet( tr("Cannot initialize shared library") );

    inited_ = true;
    return true;
}


bool MatlabLibAccess::terminate()
{
    if ( !inited_ ) return true;

    const BufferString termfnm = getFnm( shlibfnm_, false );
    termfn tfn = (termfn)sla_->getFunction( termfnm.buf() );
    if ( !tfn )
	mErrRet( tr("Cannot find function %1").arg(termfnm) );
    (*tfn)();

    inited_ = false;
    return true;
}


bool MatlabLibAccess::getParameters( BufferStringSet& names,
				     BufferStringSet& values ) const
{
    const char* getparfnm = sGetParametersStr;
    getparfn fn = (getparfn)sla_->getFunction( getparfnm );
    if ( !fn )
	mErrRet( tr("Cannot find function %1").arg(getparfnm) );

    mxArray* parsarr = NULL;
    const bool res = (*fn)( 1, &parsarr );
    if ( !res )
	mErrRet( tr("Function %1 returned false").arg(getparfnm) );

    const int nrvars = mxGetNumberOfFields( parsarr );
    for ( int idx=0; idx<nrvars; idx++ )
    {
	BufferString str = mxGetFieldNameByNumber( parsarr, idx );
	names.add( str );

	mxArray* vals = mxGetFieldByNumber( parsarr, 0, idx );
	if ( !vals ) continue;

	str = mxGetScalar( vals );
	values.add( str );
    }

    return res;
}

int MatlabLibAccess::getNrInputs() const
{
    const char* funcnm = sGetNrInputsStr;
    getnrinfn fn = (getnrinfn)sla_->getFunction( funcnm );
    if ( !fn )
	return 1;

    mxArray* mxarr = NULL;
    const bool res = (*fn)( 1, &mxarr );
    if ( !res )
	mErrRet( tr("Function %1 returned false").arg(funcnm) );

    const int nr = mCast(int,mxGetScalar(mxarr));
    return nr;
}


int MatlabLibAccess::getNrOutputs() const
{
    const char* funcnm = sGetNrOutputsStr;
    getnroutfn fn = (getnroutfn)sla_->getFunction( funcnm );
    if ( !fn )
	return 1;

    mxArray* mxarr = NULL;
    const bool res = (*fn)( 1, &mxarr );
    if ( !res )
	mErrRet( tr("Function %1 returned false").arg(funcnm) );

    const int nr = mCast(int,mxGetScalar(mxarr));
    return nr;
}

#else
bool MatlabLibAccess::init()
{ errmsg_ = tr("This plugin has not been linked to MATLAB."); return false; }
bool MatlabLibAccess::terminate()	{ return true; }
bool MatlabLibAccess::getParameters( BufferStringSet& names,
				     BufferStringSet& values ) const
{ return true; }
int MatlabLibAccess::getNrInputs() const    { return 1; }
int MatlabLibAccess::getNrOutputs() const   { return 1; }
#endif




void* MatlabLibAccess::getFunction( const char* funcnm ) const
{
    void* fn = sla_->getFunction( funcnm );
    if ( !fn )
	errmsg_ = tr("Cannot find function %1").arg(funcnm);

    return fn;
}



// MatlabLibMgr
MatlabLibMgr::MatlabLibMgr()
    : inited_(false)
{
    initApplication();
}


MatlabLibMgr::~MatlabLibMgr()
{
    deepErase( mlas_ );
    terminateApplication();
}


static int indexOf( const BufferStringSet& libnms, const char* libfnm )
{
    const FilePath fp( libfnm );
    const BufferString libnm = fp.fileName();
    return libnms.indexOf( libnm );
}


MatlabLibAccess*
    MatlabLibMgr::getMatlabLibAccess( const char* libfnm, bool doload )
{
    const int idx = indexOf( libnms_, libfnm );
    if ( mlas_.validIdx(idx) )
	return mlas_[idx];

    return doload && load(libfnm) ? getMatlabLibAccess(libfnm,false) : 0;
}


bool MatlabLibMgr::load( const char* libfnm )
{
    errmsg_ = "";
    const FilePath fp( libfnm );
    const BufferString libnm = fp.fileName();

    MatlabLibAccess* mla = new MatlabLibAccess( libfnm );
    if ( !mla->init() ) { errmsg_ = mla->errMsg(); delete mla; return false; }

    mlas_ += mla;
    libnms_.add( libnm );
    return true;
}


bool MatlabLibMgr::isLoaded( const char* libfnm ) const
{
    const int idx = indexOf( libnms_, libfnm );
    return mlas_.validIdx(idx);
}


bool MatlabLibMgr::close( const char* libfnm )
{
    const int idx = indexOf( libnms_, libfnm );
    if ( !mlas_.validIdx(idx) )
	return false;


    delete mlas_.removeSingle( idx );
    libnms_.removeSingle( idx );
    return true;
}


#ifdef HAS_MATLAB
bool MatlabLibMgr::initApplication()
{
    if ( inited_ ) return true;

    mclmcrInitialize();
    const char* options[] = { "-nojvm", "-nojit", 0 };
    if ( !mclInitializeApplication(options,2) )
    {
	errmsg_ = tr("Cannot initialize MATLAB application");
	return false;
    }

    inited_ = true;
    return true;
}


void MatlabLibMgr::terminateApplication()
{
    if ( !inited_ ) return;

    mclTerminateApplication();
    inited_ = false;
}
#else

bool MatlabLibMgr::initApplication()		{ return false; }
void MatlabLibMgr::terminateApplication()	{}

#endif

MatlabLibMgr& MLM()
{
    mDefineStaticLocalObject( MatlabLibMgr, inst, );
    return inst;
}
