#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          March 2018
________________________________________________________________________

-*/

#include "hdf5access.h"


namespace HDF5
{

/*!\brief writes to HDF5 file

Notes:
* The Writer has a notion of a 'current' DataSet. Using 'setScope' implies
  that you have created the Data Set first.
* You can write info for an entire group by simpy leaving the dataset name
  empty in the DataSetKey.
* To be able to put info for a DataSet, you have to create it first. Info for
  a group can be written at all times.
* When opened in 'edit' mode, you can get info about the file from a coupled
  Reader. Do not try to use such a Reader when the Writer is closed/destroyed.
* Every group or DataSet can have an IOPar attached with info. Although HDF5
  supports all types of 'attributes', this is the form we support. If there
  are already attributes for the data set they will be completely replaced.

  */

mExpClass(General) Writer : public Access
{
public:

			// For normal 'create', use 'open()
    uiRetVal		open4Edit(const char*);

    virtual Reader*	createCoupledReader() const			= 0;

    virtual void	setChunkSize(int)				= 0;

    uiRetVal		createDataSet(const DataSetKey&,const ArrayNDInfo&,
				      ODDataType);
    uiRetVal		createDataSet(const DataSetKey&,int,ODDataType);
    bool		deleteObject(const DataSetKey&);

    uiRetVal		putInfo(const IOPar&); //!< current scope only
    uiRetVal		putInfo(const DataSetKey&,const IOPar&);
			    //!< also for file/group info

    template <class T>
    uiRetVal		put(const DataSetKey&,const T*,int sz);
    template <class T>
    uiRetVal		put(const DataSetKey&,const TypeSet<T>&);
    uiRetVal		put(const DataSetKey&,const BufferStringSet&);

    uiRetVal		putAll(const void*);
    uiRetVal		putSlab(const SlabSpec&,const void*);

    virtual bool	isReader() const	{ return false; }

protected:

    virtual void	crDS(const DataSetKey&,const ArrayNDInfo&,
				ODDataType,uiRetVal&)			= 0;
    virtual bool	rmObj(const DataSetKey&)			= 0;

    virtual void	ptStrings(const DataSetKey&,const BufferStringSet&,
				  uiRetVal&)				= 0;
    virtual void	ptInfo(const IOPar&,uiRetVal&,const DataSetKey*)= 0;
    virtual void	ptAll(const void*,uiRetVal&)			= 0;
    virtual void	ptSlab(const SlabSpec&,const void*,uiRetVal&)	= 0;

};


template <class T>
inline uiRetVal	Writer::put( const DataSetKey& dsky, const TypeSet<T>& vals )
{
    return put( dsky, vals.arr(), vals.size() );
}


template <class T>
inline uiRetVal	Writer::put( const DataSetKey& dsky, const T* vals, int sz )
{
    uiRetVal uirv;
    if ( !vals )
	return uirv;

    uirv = createDataSet( dsky, sz, OD::GetDataRepType<T>() );
    if ( uirv.isOK() )
	uirv = putAll( vals );

    return uirv;
}

} // namespace HDF5