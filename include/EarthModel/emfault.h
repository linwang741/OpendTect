#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.5 2002-10-17 13:42:44 niclas Exp $
________________________________________________________________________


-*/
#include "emobject.h"
#include "position.h"

namespace Geometry
{
    class GridSurface;
};

namespace EarthModel
{

/*!\brief

*/
class Fault : public EMObject
{
public:
			Fault( EarthModel::EMManager&, const MultiID &);
			~Fault();

    PosID		setPos( int row, int col, const Coord3& );
			/*!<
			 */
    Coord3 		getPos( int row, int col ) const;
    PosID		addPosOnRow( int row, bool start,
	    			     const Coord3& );
			/*!< If start==true, adding pos first on row,
			     else last */
        
    PosID		insertPosOnRow( int row, int column, bool moveup,
					const Coord3& );
			/*!< If moveup==true, shifting upwards and inserting,
			  else downwards. */
		 
    void		insertRow( int row, bool moveup );
			/*!< If moveup==true, shifting up. */

    Executor*		loader();
    bool		isLoaded() const { return surface; }
    Executor*		saver();

    Geometry::GridSurface*		getSurface() { return surface; }
    const Geometry::GridSurface*	getSurface() const { return surface; }
    
protected:
    Geometry::GridSurface*		surface;
};

}; // Namespace


#endif
