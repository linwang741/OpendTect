#ifndef uiodviewer2d_h
#define uiodviewer2d_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodviewer2d.h,v 1.26 2011-06-03 14:10:26 cvsbruno Exp $
________________________________________________________________________

-*/

#include "datapack.h"
#include "emposid.h"

class CubeSampling;
class uiFlatViewAuxDataEditor;
class uiFlatViewStdControl;
class uiFlatViewWin;
class uiMainWin;
class uiODMain;
class uiODVw2DTreeTop;
class uiSlicePos2DView;
class uiTreeFactorySet;
class uiToolBar;
class Vw2DDataManager;

namespace Attrib { class SelSpec; }

/*!\brief Manages the 2D Viewers
*/

mClass uiODViewer2D : public CallBacker
{
public:
				uiODViewer2D(uiODMain&,int visid);
				~uiODViewer2D();

    void			setUpView(DataPack::ID,bool wva);
    void			setSelSpec(const Attrib::SelSpec*,bool wva);

    uiFlatViewWin* 		viewwin() 		{ return  viewwin_; }
    const uiFlatViewWin* 	viewwin() const		{ return  viewwin_; }
    Vw2DDataManager*		dataMgr()		{ return datamgr_; }
    const Vw2DDataManager*	dataMgr() const		{ return datamgr_; }

    uiODVw2DTreeTop*		treeTop() 		{ return treetp_; }
    
    const ObjectSet<uiFlatViewAuxDataEditor>&	dataEditor()	
    				{ return auxdataeditors_; }

    Attrib::SelSpec&		selSpec( bool wva )
    				{ return wva ? wvaselspec_ : vdselspec_; }
    const Attrib::SelSpec&	selSpec( bool wva ) const
    				{ return wva ? wvaselspec_ : vdselspec_; }

    void			setLineSetID( const MultiID& lsetid )
				{ linesetid_ = lsetid; }
    const MultiID&		lineSetID() const
				{ return linesetid_; }
    uiFlatViewStdControl*	viewControl() 
    				{ return viewstdcontrol_; }
    uiSlicePos2DView*		slicePos() 
    				{ return slicepos_; }
    uiODMain&			appl_;

    int				visid_;
    BufferString		basetxt_;

    void			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

protected:

    uiSlicePos2DView*				slicepos_;
    uiFlatViewStdControl*			viewstdcontrol_;
    ObjectSet<uiFlatViewAuxDataEditor>		auxdataeditors_;

    Attrib::SelSpec&		wvaselspec_;
    Attrib::SelSpec&		vdselspec_;

    Vw2DDataManager*		datamgr_;
    uiTreeFactorySet*		tifs_;
    uiODVw2DTreeTop*		treetp_;
    uiFlatViewWin*		viewwin_;

    MultiID			linesetid_;

    int				polyseltbid_;
    bool			isPolySelect_;

    void			createViewWin(bool isvert);
    virtual void		createTree(uiMainWin*);
    virtual void		createPolygonSelBut(uiToolBar*);
    void			createViewWinEditors();
    void			adjustOthrDisp(bool wva,const CubeSampling&);

    void			winCloseCB(CallBacker*);
    void			posChg(CallBacker*);
    void			selectionMode(CallBacker*);
    void			handleToolClick(CallBacker*);
    void			removeSelected(CallBacker*);
};

#endif
