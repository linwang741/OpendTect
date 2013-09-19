/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 7-7-1994
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "ascstream.h"
#include "string2.h"
#include "general.h"
#include "timefun.h"
#include "convert.h"
#include "odver.h"
#include "strmoper.h"
#include "separstr.h"
#include "od_istream.h"
#include "od_ostream.h"
#include <string.h>
#include <stdlib.h>

static const char* valsep_replacement = "\\:";
static const char* newline_replacement = "#-NL-#";


static BufferString& getPVN()
{
    static BufferString* projvernm = 0;
    if ( !projvernm )
	projvernm = new BufferString;
    return *projvernm;
}


extern "C" const char* GetProjectVersionName()
{
    BufferString& pvn = getPVN();
    if ( pvn.isEmpty() )
    {
	pvn = "dTect";
	pvn += " V";
	pvn += mODMajorVersion;
	pvn += ".";
	pvn += mODMinorVersion;
    }
    return pvn.buf();
}


mExternC( Basic)  void SetProjectVersionName(const char*);
extern "C" void SetProjectVersionName( const char* s )
{
    getPVN() = s;
}


//--- ascostream

#define mImplConstr(var,ismine) : strm_(var), strmmine_(ismine)	{}

ascostream::ascostream( od_ostream& strm )
    mImplConstr( strm, false )
ascostream::ascostream( std::ostream& strm )
    mImplConstr( *new od_ostream(strm), true )
ascostream::ascostream( od_ostream* strm )
    mImplConstr( strm ? *strm : *new od_ostream(((std::ostream*)0)), true )
ascostream::ascostream( std::ostream* strm )
    mImplConstr( *new od_ostream(strm), true )
ascostream::~ascostream()
    { if ( strmmine_ ) delete &strm_; }
std::ostream& ascostream::stdstream()
    { return strm_.stdStream(); }
bool ascostream::isOK() const
    { return !strm_.isBad(); }



bool ascostream::putHeader( const char* fltyp )
{
    strm_ << GetProjectVersionName() << od_newline << fltyp << od_newline
	     << Time::getDateTimeString() << od_newline;
    newParagraph();
    return strm_.isOK();
}


void ascostream::newParagraph()
{
    strm_ << mAscStrmParagraphMarker << od_newline;
    strm_.flush();
}


void ascostream::putKeyword( const char* keyword, bool withsep )
{
    if ( !keyword || !*keyword ) return;

    BufferString towrite = keyword;
    char* ptr = strchr( towrite.buf(), mAscStrmKeyValSep );
    while ( ptr )
    {
	BufferString tmp( ptr + 1 );
	*ptr = 0;
	towrite += valsep_replacement;
	const od_int64 prevlen = towrite.size();
	towrite += tmp;
	ptr = strchr( towrite.buf() + prevlen, mAscStrmKeyValSep );
    }

    strm_ << towrite;
    if ( withsep )
	strm_ << mAscStrmKeyValSep << ' ';
}


bool ascostream::put( const char* keyword, const char* value )
{
    putKeyword( keyword, (bool)value );

    if ( value )
    {
	const char* nlptr = strchr( value, '\n' );
	if ( nlptr )
	{
	    BufferString str( value );
	    char* startptr = str.buf();
	    char* ptr = startptr + (nlptr - value);
	    while ( ptr )
	    {
		*ptr++ = '\0';
		strm_ << startptr << newline_replacement;
		startptr = ptr; 
		ptr = strchr( ptr, '\n' );
	    }
	    value += startptr - str.buf();
	}
	strm_ << value;
    }

    strm_ << od_newline;
    return strm_.isOK();
}


#define mDeclPut1IFn(typ) \
bool ascostream::put( const char* keyword, typ value ) \
{ \
    putKeyword( keyword ); strm_ << value << od_newline; \
    return strm_.isOK(); \
}
mDeclPut1IFn(int)
mDeclPut1IFn(od_uint32)
mDeclPut1IFn(od_int64)
mDeclPut1IFn(od_uint64)


bool ascostream::put( const char* keyword, float value )
{
    return mIsUdf(value) ? strm_.isOK() : put( keyword, toString(value) );
}


bool ascostream::put( const char* keyword, double value )
{
    return mIsUdf(value) ? strm_.isOK()
	 : put( keyword, toString(value) );
}


bool ascostream::putYN( const char* keyword, bool yn )
{
    putKeyword( keyword );
    strm_ << getYesNoString(yn) << od_newline;
    return strm_.isOK();
}


#define mDeclPut2Fn(fn,typ) \
bool ascostream::fn( const char* keyword, typ v1, typ v2 ) \
{ \
    FileMultiString fms; fms += v1; fms += v2; \
    putKeyword( keyword ); strm_ << fms << od_newline; \
    return strm_.isOK(); \
}
mDeclPut2Fn(put,int)
mDeclPut2Fn(put,od_uint32)
mDeclPut2Fn(put,od_int64)
mDeclPut2Fn(put,od_uint64)
mDeclPut2Fn(put,float)
mDeclPut2Fn(put,double)
mDeclPut2Fn(putYN,bool)


#define mDeclPut3Fn(fn,typ) \
bool ascostream::fn( const char* keyword, typ v1, typ v2, typ v3 ) \
{ \
    FileMultiString fms; fms += v1; fms += v2; fms += v3; \
    putKeyword( keyword ); strm_ << fms << od_newline; \
    return strm_.isOK(); \
}
mDeclPut3Fn(put,int)
mDeclPut3Fn(put,od_uint32)
mDeclPut3Fn(put,od_int64)
mDeclPut3Fn(put,od_uint64)
mDeclPut3Fn(put,float)
mDeclPut3Fn(put,double)
mDeclPut3Fn(putYN,bool)


#define mDeclPut4Fn(fn,typ) \
bool ascostream::fn( const char* keyword, typ v1, typ v2, typ v3, typ v4 ) \
{ \
    FileMultiString fms; fms += v1; fms += v2; fms += v3; fms += v4; \
    putKeyword( keyword ); strm_ << fms << od_newline; \
    return strm_.isOK(); \
}
mDeclPut4Fn(put,int)
mDeclPut4Fn(put,od_uint32)
mDeclPut4Fn(put,od_int64)
mDeclPut4Fn(put,od_uint64)
mDeclPut4Fn(put,float)
mDeclPut4Fn(put,double)
mDeclPut4Fn(putYN,bool)


//--- ascistream

#undef mImplConstr
#define mImplConstr(var,ismine) \
	: strm_(var), strmmine_(ismine)	{ init(rdhead); }

ascistream::ascistream( od_istream& strm, bool rdhead )
    mImplConstr( strm, false )
ascistream::ascistream( std::istream& strm, bool rdhead )
    mImplConstr( *new od_istream(strm), true )
ascistream::ascistream( od_istream* strm, bool rdhead )
    mImplConstr( strm ? *strm : *new od_istream(((std::istream*)0)), true )
ascistream::ascistream( std::istream* strm, bool rdhead )
    mImplConstr( *new od_istream(strm), true )
ascistream::~ascistream()
    { if ( strmmine_ ) delete &strm_; }
std::istream& ascistream::stdstream()
    { return strm_.stdStream(); }
bool ascistream::isOK() const
    { return strm_.isOK(); }


void ascistream::init( bool rdhead )
{
    filetype = header = timestamp = "";
    if ( !rdhead )
	return;

    if ( !StrmOper::readLine(stdstream(),&header)
      || !StrmOper::readLine(stdstream(),&filetype)
      || !StrmOper::readLine(stdstream(),&timestamp) )
	return;

    removeTrailingBlanks(filetype.buf());
    if ( filetype.size() >= 4 )
    {
	char* ptr = filetype.buf() + strlen(filetype) - 4;
	if ( caseInsensitiveEqual(ptr,"file",0) )
	    *ptr = '\0';
	removeTrailingBlanks(filetype.buf());
    }

    next();
}


bool ascistream::hasStandardHeader() const
{
    return matchString( "dTect", header.buf() );
}


ascistream& ascistream::next()
{
    keybuf.setEmpty(); valbuf.setEmpty();
    if ( !strm_.isOK() )
	return *this;

    BufferString lineread;
    if ( !StrmOper::readLine(stdstream(),&lineread) )
	return *this;
    char* linebuf = lineread.buf();
    if ( linebuf[0] == '\0' || ( linebuf[0]=='-' && linebuf[1]=='-' ) )
	return next();
    if ( linebuf[0] == mAscStrmParagraphMarker[0] )
	{ keybuf = mAscStrmParagraphMarker; return *this; }

    const int sz = lineread.size();
    char* keywptr = linebuf;
    char* valptr = 0;
    for ( int ich=1; ich<sz; ich++ )
    {
	if ( linebuf[ich] == mAscStrmKeyValSep && linebuf[ich-1]!='\\' )
	{
	    valptr = linebuf + ich;
	    *valptr++ = '\0';
	    mTrimBlanks(valptr);
	    break;
	}
    }

    static const char keyvalsepstr[] = { mAscStrmKeyValSep, '\0' };
    mTrimBlanks( keywptr );
    replaceString( keywptr, valsep_replacement, keyvalsepstr );
    keybuf = keywptr;
    if ( valptr )
    {
	replaceString( valptr, newline_replacement, "\n" );
	valbuf = valptr;
    }

    return *this;
}


bool ascistream::isOfFileType( const char* ftyp ) const
{
    return matchStringCI( ftyp, filetype.buf() );
}


const char* ascistream::version() const
{
    const char* vptr = strrchr( header.buf(), 'V' );
    if ( vptr ) return vptr + 1;
    return "0.0";
}


int ascistream::majorVersion() const
{
    BufferString v( version() );
    char* ptr = strchr( v.buf(), '.' );
    if ( ptr ) *ptr = '\0';
    return toInt( v.buf() );
}


int ascistream::minorVersion() const
{
    BufferString v( version() );
    char* ptr = strrchr( v.buf(), '.' );
    return toInt( ptr ? ptr+1 : v.buf() );
}


ascistream::EntryType ascistream::type() const
{
    if ( !strm_.isOK() )
	return EndOfFile;
    if ( keybuf.isEmpty() )
	return Empty;
    if ( *keybuf.buf() == *mAscStrmParagraphMarker )
	return ParagraphMark;
    if ( valbuf.isEmpty() )
	return Keyword;

    return KeyVal;
}


bool ascistream::hasKeyword( const char* keyw ) const
{
    if ( !keyw ) return keybuf.isEmpty();
    mSkipBlanks(keyw);
    return keybuf == keyw;
}


bool ascistream::hasValue( const char* val ) const
{
    if ( !val ) return valbuf.isEmpty();
    mSkipBlanks(val);
    return valbuf == val;
}


#define mDeclGetFn(typ,fn) \
typ ascistream::get##fn( int idx ) const \
{ \
    if ( idx < 1 ) return Conv::to<typ>( valbuf.buf() ); \
    FileMultiString fms( valbuf.buf() ); \
    return Conv::to<typ>( fms[idx] ); \
}

mDeclGetFn(int,IValue)
mDeclGetFn(od_uint32,UIValue)
mDeclGetFn(od_int64,I64Value)
mDeclGetFn(od_uint64,UI64Value)
mDeclGetFn(float,FValue)
mDeclGetFn(double,DValue)
mDeclGetFn(bool,YN)
