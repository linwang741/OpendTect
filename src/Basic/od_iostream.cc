/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "od_iostream.h"
#include "filepath.h"
#include "strmprov.h"
#include "strmoper.h"
#include "bufstring.h"
#include "fixedstring.h"
#include "separstr.h"
#include "compoundkey.h"
#include "iopar.h"
#include "ascstream.h"
#include <iostream>
#include <string.h>


#define mMkoStrmData(fnm) StreamProvider(fnm).makeOStream()
#define mMkiStrmData(fnm) StreamProvider(fnm).makeIStream()
#define mInitList(ismine) sd_(*new StreamData), mine_(ismine)

od_stream::od_stream( const char* fnm, bool forwrite )
    : mInitList(true)
{
    sd_ = forwrite ? mMkoStrmData( fnm ) : mMkiStrmData( fnm );
}


od_stream::od_stream( const FilePath& fp, bool forwrite )
    : mInitList(true)
{
    const BufferString fnm( fp.fullPath() );
    sd_ = forwrite ? mMkoStrmData( fnm ) : mMkiStrmData( fnm );
}


od_stream::od_stream( std::ostream* strm )
    : mInitList(true)
{
    sd_.ostrm = strm;
}


od_stream::od_stream( std::istream* strm )
    : mInitList(true)
{
    sd_.istrm = strm;
}


od_stream::od_stream( std::ostream& strm )
    : mInitList(false)
{
    sd_.ostrm = &strm;
}


od_stream::od_stream( std::istream& strm )
    : mInitList(false)
{
    sd_.istrm = &strm;
}


// all private, setting to empty
od_stream::od_stream(const od_stream&) : mInitList(false) {}
od_stream& od_stream::operator =(const od_stream&) { close(); return *this; }
od_istream& od_istream::operator =(const od_istream&) { close(); return *this; }
od_ostream& od_ostream::operator =(const od_ostream&) { close(); return *this; }


od_stream::~od_stream()
{
    close();
    delete &sd_;
}


void od_stream::close()
{
    if ( mine_ )
	sd_.close();
}


bool od_stream::isOK() const
{
    if ( forWrite() )
	return sd_.ostrm && sd_.ostrm->good();
    else
	return sd_.istrm && sd_.istrm->good();
}


bool od_stream::isBad() const
{
    if ( forWrite() )
	return !sd_.ostrm || !sd_.ostrm->good();
    else
	return !sd_.istrm || sd_.istrm->bad();
}


const char* od_stream::errMsg() const
{
    return StrmOper::getErrorMessage( streamData() );
}


od_stream::Pos od_stream::position() const
{
    if ( sd_.ostrm )
	return StrmOper::tell( *sd_.ostrm );
    else if ( sd_.istrm )
	return StrmOper::tell( *sd_.istrm );
    return -1;
}


static std::ios::seekdir getSeekdir( od_stream::Ref ref )
{
    return ref == od_stream::Rel ? std::ios::cur
	: (ref == od_stream::Beg ? std::ios::beg
				 : std::ios::end);
}


void od_stream::setPosition( od_stream::Pos offs, od_stream::Ref ref )
{
    if ( sd_.ostrm )
    {
	if ( ref == Abs )
	    StrmOper::seek( *sd_.ostrm, offs );
	else
	    StrmOper::seek( *sd_.ostrm, offs, getSeekdir(ref) );
    }
    else if ( sd_.istrm )
    {
	if ( ref == Abs )
	    StrmOper::seek( *sd_.istrm, offs );
	else
	    StrmOper::seek( *sd_.istrm, offs, getSeekdir(ref) );
    }
}


const char* od_stream::fileName() const
{
    if ( sd_.fileName() )
	return sd_.fileName();
    if ( sd_.istrm == &std::cin || sd_.ostrm == &std::cout )
	return StreamProvider::sStdIO();
    else if ( sd_.ostrm == &std::cerr )
	return StreamProvider::sStdErr();
    return "";
}


void od_stream::setFileName( const char* fnm )
{
    sd_.setFileName( fnm );
}


od_int64 od_stream::endPosition() const
{
    const Pos curpos = position();
    od_stream& self = *const_cast<od_stream*>( this );
    self.setPosition( 0, End );
    const Pos ret = position();
    self.setPosition( curpos, Abs );
    return ret;
}


bool od_stream::forWrite() const
{
    return sd_.ostrm;
}


void od_stream::releaseStream( StreamData& out )
{
    sd_.transferTo( out );
}


std::istream& od_istream::stdStream()
{
    return sd_.istrm ? *sd_.istrm : std::cin;
}


std::ostream& od_ostream::stdStream()
{
    return sd_.ostrm ? *sd_.ostrm : std::cerr;
}


void od_ostream::flush()
{
    if ( sd_.ostrm )
	sd_.ostrm->flush();
}


od_stream::Count od_istream::lastNrBytesRead() const
{
    if ( sd_.istrm )
	return mCast(od_stream::Count,StrmOper::lastNrBytesRead(*sd_.istrm));
    return 0;
}


#define mImplStrmAddFn(typ,tostrm) \
od_ostream& od_ostream::add( typ t ) \
{ \
    stdStream() << tostrm; return *this; \
}

#define mImplStrmGetFn(typ,tostrm) \
od_istream& od_istream::get( typ& t ) \
{ \
    stdStream() >> tostrm; return *this; \
}

#define mImplSimpleAddFn(typ) mImplStrmAddFn(typ,t)
#define mImplSimpleGetFn(typ) mImplStrmGetFn(typ,t)
#define mImplSimpleAddGetFns(typ) mImplSimpleAddFn(typ) mImplSimpleGetFn(typ)

mImplSimpleAddGetFns(char)
mImplSimpleAddGetFns(unsigned char)
mImplSimpleAddGetFns(od_int16)
mImplSimpleAddGetFns(od_uint16)
mImplSimpleAddGetFns(od_int32)
mImplSimpleAddGetFns(od_uint32)
mImplSimpleAddGetFns(od_int64)
mImplSimpleAddGetFns(od_uint64)
mImplSimpleAddGetFns(float)
mImplSimpleAddGetFns(double)


mImplSimpleAddFn(const char*)
od_istream& od_istream::get( char* str )
    { pErrMsg("Dangerous: od_istream::get(char*)"); return getC( str, 0 ); }

mImplStrmAddFn(const BufferString&,t.buf())
od_istream& od_istream::get( BufferString& bs )
{
    StrmOper::readWord( stdStream(), &bs );
    return *this;
}

od_ostream& od_ostream::add( const FixedString& fs )
    { return fs.str() ? add( fs.str() ) : *this; }
od_istream& od_istream::get( FixedString& fs )
    { pErrMsg("od_istream::get(FixedString&) called"); return *this; }


od_istream& od_istream::getC( char* str, int maxnrch )
{
    if ( str )
    {
	BufferString bs; get( bs );
	if ( maxnrch > 0 )
	    strncpy( str, bs.buf(), maxnrch );
	else
	    strcpy( str, bs.buf() ); // still dangerous, but intentional
    }
    return *this;
}


bool od_istream::getBin( void* buf, od_stream::Count nrbytes )
{
    if ( nrbytes == 0 || !buf )
	return true;
    return StrmOper::readBlock( stdStream(), buf, nrbytes );
}


bool od_ostream::putBin( const void* buf, od_stream::Count nrbytes )
{
    return nrbytes <= 0 || !buf ? true
	: StrmOper::writeBlock( stdStream(), buf, nrbytes );
}


bool od_istream::getLine( BufferString& bs )
{
    return StrmOper::readLine( stdStream(), &bs );
}


bool od_istream::getAll( BufferString& bs )
{
    return StrmOper::readFile( stdStream(), bs );
}


char od_istream::peek() const
{
    return (char)(const_cast<od_istream*>(this)->stdStream()).peek();
}


void od_istream::ignore( od_stream::Count nrbytes )
{
    stdStream().ignore( (std::streamsize)nrbytes );
}


od_istream& od_istream::get( IOPar& iop )
{
    ascistream astrm( *this, false );
    iop.getFrom( astrm );
    return *this;
}

od_ostream& od_ostream::add( const IOPar& iop )
{
    ascostream astrm( *this );
    iop.putTo( astrm );
    return *this;
}


mImplStrmAddFn(const SeparString&,t.buf())
od_istream& od_istream::get( SeparString& ss )
{
    BufferString bs; get( bs ); ss = bs.buf();
    return *this;
}
mImplStrmAddFn(const CompoundKey&,t.buf())
od_istream& od_istream::get( CompoundKey& ck )
{
    BufferString bs; get( bs ); ck = bs.buf();
    return *this;
}
