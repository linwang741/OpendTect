#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		9-3-1999
________________________________________________________________________

*/

#include "basicmod.h"
#include "valseries.h"
#include "arrayndinfo.h"
#include "varlenarray.h"
#include "ptrman.h"

/*!\brief An ArrayND is an array with a given number of dimensions and a size.

  The ArrayND can always be accessed via set() and get(). ArrayND as such is
  not often used; the most common 1D, 2D and 3D versions have their own
  subclasses Array1D, Array2D and Array3D. Still, you can work 'ND' mode to
  do operations nicely generalised, like calculating stats.

  There is a simple array-based implementation in arrayndimpl.h of each of
  ArrayND and its subclasses ArrayxD. These have data in a single array with a
  standard layout (last dim is fastest).

  If you get an ArrayND, you always have to be able to handle the situation that
  the data is not in contiguous memory, not in the standard layout, or even not
  in memory. In that case, getData() returns a null pointer.

  Most ArrayND's are also ValueSeries<float>. That interface can be used
  instead of the fallback get() and set(), use 'getStorage()'. For
  non-critical performance stuff get() and set() are your usually easier to
  use, no extra code required. If you really need performance, try getData().
  Most bulk data in OpendTect (like in DataPacks) nicely return non-null for
  getData(), and when this happens the extra code branch may speed up your
  data-crunchy thingy considerably (yes, you still need to also handle the
  case of getData() returning null, because it does happen).

  So the following recommendations:

  1) For performance-critial stuff, try getData() first.
  2) If that returns null, easy old-fashioned index-based work using get()
  and set() is easy to understand and always works.

Example:

    float getSum( const Array2D<float>& arr2d )
    {
	const float* vals = arr2d.getData();
	if ( vals )
	    return std::accumulate( vals, vals+arr2d.info().totalSize(), 0.f );
	else
	{
	    const int dim0 = arr2d.getSize( 0 );
	    const int dim1 = arr2d.getSize( 1 );
	    float sum = 0.f;
	    for ( int idx0=0; idx0<dim0; idx0++ )
	    {
		for ( int idx1=0; idx1<dim1; idx1++ )
		    sum += arr2d.get( idx0, idx1 );
	    }
	    return sum;
	}
    }

*/

template <class T>
mClass(Basic) ArrayND
{
public:

    typedef T			DataType;

    virtual			~ArrayND()	{}

    virtual inline bool		isOK() const;
    virtual inline bool		isEmpty() const;

    virtual DataType		getND(const int*) const	= 0;
    virtual bool		isSettable() const	{ return true; }
    virtual void		setND(const int*,T)	= 0;

    inline const ValueSeries<T>* getStorage() const { return getStorage_(); }
    inline ValueSeries<T>*	getStorage();
    virtual bool		canSetStorage() const { return false; }
    virtual bool		setStorage( ValueSeries<T>* s )
				    { delete s; return true; }
				    /*!<becomes mine. size must be settable */

    inline const DataType*	getData() const		{ return getData_(); }
    inline DataType*		getData();
    virtual const DataType*	get1D(const int*) const;
    virtual DataType*		get1D(const int*);
    virtual int			get1DDim() const;


    virtual const ArrayNDInfo&	info() const		= 0;
    virtual bool		canSetInfo() const	{ return false; }
    virtual bool		canChangeNrDims() const	{ return false; }
    virtual bool		setInfo( const ArrayNDInfo& ) { return false; }

    virtual void		setAll(DataType);
    virtual void		getAll(DataType*) const;
				/*!< ptr must be allocated, min totalSize() */
    virtual void		getAll(ValueSeries<T>& vs) const;

				// rank/size info
    inline int			nrDims() const
				{ return info().nrDims(); }
    inline int			getSize( int dim ) const
				{ return info().getSize(dim); }
    inline od_uint64		totalSize() const
				{ return info().totalSize(); }

				// aliases
    inline int			rank() const		{ return nrDims(); }
    inline const ValueSeries<T>* valueSeries() const	{ return getStorage(); }
    inline ValueSeries<T>*	valueSeries()		{ return getStorage(); }

protected:

    virtual const ValueSeries<T>* getStorage_() const { return 0; }

    virtual const T*		getData_() const
				{ return getStorage_() ? getStorage_()->arr()
						       : 0; }

};


/*!\brief 1-Dim ArrayND */

template <class T>
mClass(Basic) Array1D : public ArrayND<T>
		      , public ValueSeries<T>
{
public:

    virtual void		set(int,T)				= 0;
    virtual T			get(int) const				= 0;
    void			setND(const int* pos,T v) { set( pos[0], v ); }
    T				getND(const int* pos) const
				{ return get(pos[0]); }

				// implement ValueSeries interface
    T				value(od_int64 i) const	{ return get( (int) i);}
    bool			writable() const	{ return true; }
    void			setValue(od_int64 i,T t){ set( (int) i,t); }
    virtual void		setAll( T t )         { ArrayND<T>::setAll(t); }

    virtual const Array1DInfo&	info() const = 0;

    inline T			operator []( int idx ) const
				{ return get( idx ); }

};


/*!\brief 2-Dim ArrayND */

template <class T>
mClass(Basic) Array2D : public ArrayND<T>
{
public:
    virtual void		set( int, int, T )			= 0;
    virtual T			get( int p0, int p1 ) const		= 0;
    void			setND(  const int* pos, T v )
				    { set( pos[0], pos[1], v);}
    T		                getND( const int* pos ) const
				    { return get( pos[0], pos[1] ); }

    virtual T**			get2DData()		{ return 0; }
    virtual const T**		get2DData() const	{ return 0; }

    virtual const Array2DInfo&	info() const = 0;
};


/*!\brief 3-Dim ArrayND */

template <class T>
mClass(Basic) Array3D : public ArrayND<T>
{
public:

    virtual void		set( int, int, int, T )			= 0;
    virtual T			get( int p0, int p1, int p2 ) const	= 0;
    void			setND( const int* pos, T v )
				    { set( pos[0], pos[1], pos[2], v);}
    T		                getND( const int* pos ) const
				    { return get( pos[0], pos[1], pos[2] ); }

    virtual T***		get3DData()		{ return 0; }
    virtual const T***		get3DData() const	{ return 0; }

    virtual const Array3DInfo&	info() const = 0;
};


/*!
\brief Iterates through all samples in an ArrayND.

  ArrayNDIter will be on the first position when initiated, and move to
  the second at the first call to next(). next() will return false when
  no more positions are available.
*/

mExpClass(Basic) ArrayNDIter
{
public:
				ArrayNDIter( const ArrayNDInfo& );
				~ArrayNDIter();

    bool			next();
    void			reset();

    bool			setGlobalPos(od_int64);

    template <class T> void inline setPos(const T& idxabl);
    const int*			getPos() const { return position_; }
    int				operator[](int) const;

protected:

    bool			inc(int);

    int*			position_;
    const ArrayNDInfo&		sz_;

};


#define mArrayNDVSAdapterMaxNrDims 64

/*!\brief Adapter that makes any ArrayND to a (slow) value series.

  If you use this without checking getStorage() (much faster) first you will
  be slapped in the face with a pErrMsg.

*/

template <class T>
mClass(Basic) ArrayNDValseriesAdapter : public ValueSeries<T>
{
public:
			ArrayNDValseriesAdapter( const ArrayND<T>& a )
			    : array_( a )
			{
			    if ( array_.getStorage() )
				{ pErrMsg("Use getStorage() instead"); }
			}

    ValueSeries<T>*	clone() const
			{ return new ArrayNDValseriesAdapter<T>( *this ); }

    T			value( od_int64 idx ) const
			{
			    int pos[mArrayNDVSAdapterMaxNrDims];
			    array_.info().getArrayPos( idx, pos );
			    return array_.getND( pos );
			}

    const T*		arr() const	{ return array_.getData(); }
    T*			arr()		{ return 0; }

protected:

    const ArrayND<T>&	array_;

};


template <class T> inline void ArrayNDIter::setPos( const T& idxable )
{
    for ( int idx=sz_.nrDims()-1; idx>=0; idx-- )
	position_[idx] = idxable[idx];
}


inline
bool ArrayNDIter::setGlobalPos( od_int64 globalidx )
{
    return sz_.getArrayPos(globalidx,position_);
}

#define mDefArrayNDStdMembers(nd) \
public: \
			Array##nd##Conv(Array##nd<TT>* arr) \
			    : arr_(arr)	{} \
			~Array##nd##Conv()	{ delete arr_; } \
 \
    const Array##nd##Info&	info() const	{ return arr_->info(); } \
 \
protected: \
 \
    Array##nd<TT>*	arr_; \
 \
public:

#define mDefArrayNDConverter(nd) \
template <class T, class TT> \
class Array##nd##Conv : public Array##nd<T> \
{ mDefArrayNDStdMembers(nd);



template <class T, class TT>
class Array1DConv : public Array1D<T>
{ mDefArrayNDStdMembers(1D);
public:
    T			get( int p0 ) const
					{ return (T)arr_->get( p0 ); }
    void		set( int p0, T v )
					{ arr_->set( p0, (TT)v ); }

};


template <class T, class TT>
class Array2DConv : public Array2D<T>
{ mDefArrayNDStdMembers(2D);
    T			get( int p0, int p1 ) const
					{ return (T)arr_->get( p0, p1 ); }
    void		set( int p0, int p1, T v )
					{ arr_->set( p0, p1, (TT)v ); }

};

template <class T, class TT>
class Array3DConv : public Array3D<T>
{ mDefArrayNDStdMembers(3D);

    T			get( int p0, int p1, int p2 ) const
					{ return (T)arr_->get( p0, p1, p2 ); }
    void		set( int p0, int p1, int p2, T v )
					{ arr_->set( p0, p1, p2, (TT)v ); }

};


// Only implementations below

template <class T> inline
bool ArrayND<T>::isOK() const
{
    return getStorage() ? getStorage()->isOK() : true;
}


template <class T> inline
bool ArrayND<T>::isEmpty() const
{
    return !isOK() || totalSize() < 1;
}


template <class T> inline
const T* ArrayND<T>::get1D( const int* i ) const
{
    const T* ptr = getData();
    if ( !ptr )
	return 0;

    int ndim = nrDims();

    mAllocLargeVarLenArr( int, pos, ndim );
    OD::memCopy(pos,i, (int) sizeof(int)*(ndim-1));

    pos[ndim-1] = 0;

    return &ptr[info().getOffset( pos )];
}


template <class T> inline
int ArrayND<T>::get1DDim() const
{ return nrDims()-1; }


template <class T> inline
T* ArrayND<T>::getData()
{
    return !isSettable() ? 0
			 : const_cast<T*>(((const ArrayND*)this)->getData_());
}


template <class T> inline
ValueSeries<T>* ArrayND<T>::getStorage()
{
    return !isSettable() ? 0 :
	const_cast<ValueSeries<T>* >(((const ArrayND*)this)->getStorage_());
}


template <class T> inline
T* ArrayND<T>::get1D( const int* i )
{
    return !isSettable() ? 0 : const_cast<T*>(((const ArrayND*)this)->get1D(i));
}


template <class T> inline
void ArrayND<T>::setAll( T val )
{
    if ( isEmpty() )
	return;

    ValueSeries<T>* stor = getStorage();
    if ( stor )
    {
	if ( stor->canSetAll() )
	    stor->setAll( val );
	else
	{
	    MemSetter<T> setter( *stor, val, totalSize() );
	    setter.execute();
	}

	return;
    }

    ArrayNDIter iterator( info() );
    do
    {
	setND( iterator.getPos(), val );
    } while ( iterator.next() );
}

/*!
\brief Gets a one dimensional array from an ArrayND.
*/

template <class T>
mClass(Basic) ArrayNDDataExtracter : public ParallelTask
{
public:
		ArrayNDDataExtracter( T* ptr, const ArrayND<T>& arr )
		    : ptr_( ptr )
		    , arr_( arr )
		    , totalnr_( arr.totalSize() )
		    , vs_( 0 )
		{}

		ArrayNDDataExtracter( ValueSeries<T>& vs, const ArrayND<T>& arr)
		    : ptr_( vs.arr() )
		    , arr_( arr )
		    , totalnr_( arr.totalSize() )
		    , vs_( vs.arr() ? 0 : &vs )
		{}

    bool	doWork( od_int64 start, od_int64 stop, int )
		{
		    mAllocVarLenArr( int, pos, arr_.nrDims() );
		    if ( !arr_.info().getArrayPos( start, pos ) )
			return false;

		    ArrayNDIter iterator( arr_.info() );
		    iterator.setPos( (int*) pos );

		    if ( vs_ )
		    {
			for ( od_int64 idx=start; idx<=stop; idx++ )
			{
			    vs_->setValue( idx,
				    arr_.getND( iterator.getPos() ) );
			    if ( idx==stop )
				break;

			    if ( !iterator.next() )
				return false;
			}
		    }
		    else
		    {
			T* res = ptr_ + start;
			for ( od_int64 idx=start; idx<=stop; idx++, res++ )
			{
			    *res = arr_.getND( iterator.getPos() );
			    if ( idx==stop )
				break;

			    if ( !iterator.next() )
				return false;
			}
		    }

		    return true;
		}

    od_int64	nrIterations() const { return totalnr_; }

protected:

    od_int64		totalnr_;
    const ArrayND<T>&	arr_;
    T*			ptr_;
    ValueSeries<T>*	vs_;

};


template <class T> inline
void ArrayND<T>::getAll( ValueSeries<T>& vs ) const
{
    if ( vs.arr() )
	{ getAll( vs.arr() ); return; }

    const od_int64 totalsz = totalSize();
    if ( totalsz < 1 )
	return;

    const ValueSeries<T>* stor = getStorage();
    if ( stor )
	stor->getValues( vs, totalsz );
    else
    {
	ArrayNDDataExtracter<T> extr( vs, *this );
	extr.execute();
    }
}


template <class T> inline
void ArrayND<T>::getAll( T* ptr ) const
{
    const od_int64 totalsz = totalSize();
    if ( !totalsz )
	return;

    const T* tdata = getData();
    if ( tdata )
	OD::memCopy( ptr, tdata, totalsz * sizeof(T) );
    else
    {
	const ValueSeries<T>* stor = getStorage();
	if ( stor )
	{
	    MemCopier<T> cpier( ptr, *stor, totalsz );
	    cpier.execute();
	}
	else
	{
	    ArrayNDDataExtracter<T> extr( ptr, *this );
	    extr.execute();
	}
    }
}
