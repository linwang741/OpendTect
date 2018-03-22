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
* The Writer has a notion of a 'current' DataSet.
* You can write info for an entire group by simpy leaving the dataset name
  empty in the DataSetKey.
* To be able to put info for a DataSet, you have to create it first. Info for
  a group can be written at all times.

  */

mExpClass(General) Writer : public Access
{
public:

    virtual void	setChunkSize(int)				= 0;

    uiRetVal		createDataSet(const DataSetKey&,const ArrayNDInfo&,
				      ODDataType);

    uiRetVal		putInfo(const DataSetKey&,const IOPar&);

    uiRetVal		putAll(const void*);
    uiRetVal		putSlab(const SlabSpec&,const void*);

protected:

    virtual void	ptInfo(const DataSetKey&,const IOPar&,uiRetVal&)= 0;
    virtual void	crDS(const DataSetKey&,const ArrayNDInfo&,
				ODDataType,uiRetVal&)			= 0;
    virtual void	ptAll(const void*,uiRetVal&)			= 0;
    virtual void	ptSlab(const SlabSpec&,const void*,uiRetVal&)	= 0;

};

} // namespace HDF5
