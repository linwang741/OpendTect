#ifndef uitreeitemmanager_h
#define uitreeitemmanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: uitreeitemmanager.h,v 1.21 2006-01-30 14:51:38 cvskris Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "iopar.h"
#include "callback.h"
#include "bufstring.h"

/*!\brief
are helping classes for uiListViews, wich makes it easy to bundle inteligence
and data to treeitems. Usage is normally to create the uiTreeTopItem, run the
its init() and add Childrens.
*/

class uiListViewItem;
class uiListView;
class uiParent;

class uiTreeItem	: public CallBacker
{
public:

    				uiTreeItem( const char* name__ );
    virtual			~uiTreeItem();
    virtual void		prepareForShutdown();
    				/*!<Override if you want to popup dlg
				    for saving various things (or similar) */
    				

    const char*			name() const;

    virtual int			selectionKey() const { return -1; }
    virtual bool		select();
    				/*!<Selects this item */
    virtual void		setChecked(bool yn);

    virtual int			siblingIndex() const;
    				/*\returns the index of this item among
				   its siblings.
				  \note this index is not neseccarely the same
				        as the item's index in the parent's
					child-list. */

    virtual uiTreeItem*		siblingAbove();
    virtual uiTreeItem*		siblingBelow();
    virtual void		moveItem( uiTreeItem* after );
    virtual void		moveItemToTop();
				 
    virtual bool		addChild( uiTreeItem* child );
    				/*!<Adds a child. If the child does not fit
				    (i.e. the child's parentType() is not
				    the same as this), it will try to find
				    a valid parent somewhere else in the tree.
				    \note child becomes mine regardless of
				    	  return value.
				*/
    virtual void		removeChild( uiTreeItem* );
    virtual const uiTreeItem*	findChild( const char* name ) const;
    				/*!<Finds a child in the tree below
				    this item.
				*/
    virtual const uiTreeItem*	findChild( int selkey ) const;
    				/*!<Finds a child in the tree below
				    this item.
				*/
    virtual uiTreeItem*		findChild( const char* name );
    				/*!<Finds a child in the tree below
				    this item.
				*/
    virtual uiTreeItem*		findChild( int selkey );
    				/*!<Finds a child in the tree below
				    this item.
				*/

    template<class T> inline void setProperty(const char* key, const T&);
    				/*!<Sets a keyed value that has been retrieved
				    with getProperty().
				    \note		Should not be used if T
				    			is a pointer. Use
							setPropertyPtr(
							const char*, T& )
							instead.
				*/
    inline void			setPropertyPtr(const char* key,void*);
    				/*!<Sets a keyed pointer that may have been
				    retrieved with getPropertyPtr().
				*/
    template<class T> inline bool getProperty(const char* key, T& res) const;
    				/*!<Gets a keyed value that has been stored
				    with setProperty().
				    \retval true	the key was found and
				    			res is set
				    \retval false	the key was not found
				    			and res is not set
				    \note		Should not be used if T
				    			is a pointer. Use
							getPropertyPtr(
							const char*, T& )
							instead.
				*/
    inline bool			getPropertyPtr(const char* key,void*&) const;
    				/*!<Gets a keyed pointer that has been stored
				    with setPropertyPtr().
				    \retval true	the key was found and
				    			res is set
				    \retval false	the key was not found
				    			and res is not set
				*/

    virtual void		updateColumnText(int col);
protected:
    virtual int			uiListViewItemType() const;
    				/*!<\returns the uiListViewItem::Type that
				    should be created */
    virtual uiParent*		getUiParent();

    virtual bool		addChild( uiTreeItem*, bool downwards );
    				/*!< Adds a child to this item. If the child
				    does not fit (i.e. its parentType() is not
				    equal to this), the object tries to add
				    it to its parent if downwards is false.
				    If downwards is true, it tries to add it
				    to its children if it does not fit.
				*/

    virtual const char*		parentType() const = 0;
    				/*!<\returns typeid(parentclass).name() */
    virtual bool		init() { return true; }

    virtual bool		rightClick(uiListViewItem* item);
    virtual bool		anyButtonClick(uiListViewItem* item);
    virtual void		setListViewItem( uiListViewItem* );
    uiListViewItem*		getItem() { return uilistviewitem; }

    virtual bool		showSubMenu() { return true; }
    virtual bool		select(int selkey);

    virtual bool		isSelectable() const { return false; }
    virtual bool		isExpandable() const { return true; }
    
    virtual void		updateSelection(int selectionKey,
	    					bool dw=false );
    				/*!< Does only update the display */

    IOPar			properties;

    uiTreeItem*			parent;
    BufferString		name_;

    uiListViewItem*		uilistviewitem;
    ObjectSet<uiTreeItem>	children;
    friend			class uiTreeTopItem;
    friend			class uiODTreeTop;
};


class uiTreeTopItem : public uiTreeItem
{
public:
    			uiTreeTopItem(uiListView*);
    virtual bool	addChild( uiTreeItem*);
    virtual void	updateSelection(int selectionkey, bool=false );
    			/*!< Does only update the display */
    virtual void	updateColumnText(int col);

    void		disabRightClick(bool yn) 	{ disabrightclick=yn; }
    void		disabAnyClick(bool yn) 		{ disabanyclick=yn; }


			~uiTreeTopItem();
protected:
    virtual bool	addChild( uiTreeItem*, bool downwards);
    uiListView*		listview;

    void		selectionChanged(CallBacker*);
    void		rightClickCB(CallBacker*);
    void		anyButtonClickCB(CallBacker*);

    virtual const char*	parentType() const { return 0; } 
    virtual uiParent*	getUiParent();

    bool		disabrightclick;
    bool		disabanyclick;
};


class uiTreeItemFactory
{
public:
    virtual		~uiTreeItemFactory()		{}
    virtual const char*	name() const			= 0;
    virtual uiTreeItem*	create() const			= 0;
};


class uiTreeFactorySet : public CallBacker
{
public:
					uiTreeFactorySet();
					~uiTreeFactorySet();
    void				addFactory(uiTreeItemFactory* ptr,
	    					   int placementindex=-1 );
					/*!<\param ptr	pointer to new factory.
							Object is managed by me.
					    \param placementindex
					    		Indicates how the
							created treeitems should
							be placed when making
							a new tree.
					*/
    void				remove( const char* );

    int					nrFactories() const;
    const uiTreeItemFactory*		getFactory( int idx ) const;
    int					getPlacementIdx(int idx) const;

    CNotifier<uiTreeFactorySet,int>	addnotifier;
    CNotifier<uiTreeFactorySet,int>	removenotifier;

protected:

    ObjectSet<uiTreeItemFactory>	factories;
    TypeSet<int>			placementidxs;

};


template<class T>
bool inline uiTreeItem::getProperty( const char* propertykey, T& res ) const
{
    if ( properties.get( propertykey, res ))
	return true;

    return parent ? parent->getProperty( propertykey, res ) : false;
}


inline bool uiTreeItem::getPropertyPtr( const char* propertykey, void*& res ) const
{
    if ( properties.getPtr( propertykey, res ))
	return true;

    return parent ? parent->getPropertyPtr( propertykey, res ) : false;
}


template<class T>
void inline uiTreeItem::setProperty( const char* propertykey, const T& val )
{
    if ( typeid(T)==typeid(void*) )
	properties.set( propertykey, (int64)val );
    else
	properties.set( propertykey, val );
}


void inline uiTreeItem::setPropertyPtr( const char* propertykey, void* val )
{
    properties.setPtr( propertykey, val );
}

#endif
