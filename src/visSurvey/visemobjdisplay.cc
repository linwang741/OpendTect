/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
 RCS:           $Id: visemobjdisplay.cc,v 1.4 2005-01-17 08:03:34 kristofer Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: visemobjdisplay.cc,v 1.4 2005-01-17 08:03:34 kristofer Exp $";


#include "vissurvemobj.h"

#include "attribsel.h"
#include "cubicbeziersurface.h"
#include "emmanager.h"
#include "emobject.h"
#include "mpeengine.h"
#include "viscubicbeziersurface.h"
#include "visevent.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "vistransform.h"


namespace visSurvey
{


mCreateFactoryEntry( EMObjectDisplay );


EMObjectDisplay::EMObjectDisplay()
    : VisualObjectImpl(true)
    , mid(-1)
    , as(*new AttribSelSpec)
    , colas(*new ColorAttribSel)
    , usestexture(false)
    , editor( 0 )
    , eventcatcher( 0 )
    , transformation( 0 )
    , translation( 0 )
{}


EMObjectDisplay::~EMObjectDisplay()
{
    removeAll();

    delete &as;
    delete &colas;
    if ( transformation ) transformation->unRef();
    if ( eventcatcher ) eventcatcher->unRef();
}


mVisTrans* EMObjectDisplay::getDisplayTransformation() {return transformation;}


void EMObjectDisplay::setDisplayTransformation(mVisTrans* nt)
{
    if ( transformation ) transformation->unRef();
    transformation = nt;
    if ( transformation ) transformation->ref();

    for ( int idx=0; idx<sections.size(); idx++ )
	sections[idx]->setDisplayTransformation(transformation);

    if ( editor ) editor->setDisplayTransformation(transformation);
}


void EMObjectDisplay::setSceneEventCatcher(visBase::EventCatcher* nec )
{
    if ( eventcatcher ) eventcatcher->unRef();
    eventcatcher = nec;
    if ( eventcatcher ) eventcatcher->ref();

    for ( int idx=0; idx<sections.size(); idx++ )
	sections[idx]->setSceneEventCatcher(nec);

    if ( editor ) editor->setSceneEventCatcher(nec);
}


void EMObjectDisplay::removeAll()
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	removeChild( sections[idx]->getInventorNode() );
	sections[idx]->unRef();
    }

    sections.erase();
    sectionids.erase();

    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobject = em.getObject(em.multiID2ObjectID(mid));
    if ( emobject ) emobject->unRef();
    if ( editor ) editor->unRef();
    if ( translation )
    {
	removeChild( translation->getInventorNode() );
	translation->unRef();
	translation = 0;
    }
}


bool EMObjectDisplay::setEMObject(const MultiID& newmid)
{
    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobject = em.getObject(em.multiID2ObjectID(newmid));
    if ( !emobject ) return false;

    if ( sections.size() ) removeAll();

    mid = newmid;
    emobject->ref();
    return updateFromEM();
}


bool EMObjectDisplay::updateFromEM()
{ 
    if ( sections.size() ) removeAll();

    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobject = em.getObject(em.multiID2ObjectID(mid));
    if ( !emobject ) return false;

    for ( int idx=0; idx<emobject->nrSections(); idx++ )
    {
	const EM::SectionID sectionid = emobject->sectionID(idx);
	Geometry::Element* ge = const_cast<Geometry::Element*>(
	    const_cast<const EM::EMObject*>(emobject)->getElement(sectionid));
	if ( !ge ) continue;

	visBase::VisualObject* vo = createSection( ge );
	if ( !vo ) continue;

	vo->ref();
	vo->setMaterial( 0 );
	addChild( vo->getInventorNode() );

	sections += vo;
	sectionids += sectionid;
    }

    const EM::ObjectID objid = EM::EMM().multiID2ObjectID(mid);
    if ( MPE::engine().getEditor(objid,false) )
	enableEditing(true);

    return true;
}


void EMObjectDisplay::useTexture(bool yn)
{
    usestexture = yn;
    if ( usesTexture()==yn )
	return;

    Color newcolor = yn ? Color::White : nontexturecol;
    if ( yn ) nontexturecol = getMaterial()->getColor();

    for ( int idx=0; idx<sections.size(); idx++ )
    {
	//TODO: DynamicCast and set texture
    }
}


bool EMObjectDisplay::usesTexture() const { return usestexture; }



const AttribSelSpec* EMObjectDisplay::getSelSpec() const
{ return &as; }


void EMObjectDisplay::setSelSpec( const AttribSelSpec& as_ )
{
    removeAttribCache();
    as = as_;
}


const ColorAttribSel* EMObjectDisplay::getColorSelSpec() const
{ return &colas; }


void EMObjectDisplay::setColorSelSpec( const ColorAttribSel& as_ )
{ colas = as_; }


void EMObjectDisplay::setDepthAsAttrib()
{
    pErrMsg("Not impl");
}


bool EMObjectDisplay::hasStoredAttrib() const
{
    const char* ref = as.userRef();
    return as.id() == AttribSelSpec::otherAttrib && ref && *ref;
}


Coord3 EMObjectDisplay::getTranslation() const
{ return translation ? translation->getTranslation() : Coord3( 0, 0, 0 ); }


void EMObjectDisplay::setTranslation( const Coord3& nt )
{
    if ( !translation )
    {
	translation = visBase::Transformation::create();
	translation->ref();
	insertChild( 0, translation->getInventorNode() );
    }

    translation->setTranslation( nt );
}


bool EMObjectDisplay::usesWireframe() const
{
    if ( !sections.size() )
	return false;

    mDynamicCastGet( const visBase::CubicBezierSurface*, cbs, sections[0] );
    return cbs->usesWireframe();
}


void EMObjectDisplay::useWireframe( bool yn )
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::CubicBezierSurface*,cbs,sections[idx]);
	if ( cbs ) cbs->useWireframe(yn);
    }
}


MPEEditor* EMObjectDisplay::getEditor() { return editor; }


void EMObjectDisplay::enableEditing(bool yn)
{
    if ( yn && !editor )
    {
	const EM::ObjectID objid = EM::EMM().multiID2ObjectID(mid);
	MPE::ObjectEditor* mpeeditor = MPE::engine().getEditor(objid,true);

	if ( !mpeeditor ) return;

	editor = MPEEditor::create();
	editor->ref();
	editor->setSceneEventCatcher( eventcatcher );
	editor->setDisplayTransformation( transformation );
	editor->setEditor(mpeeditor);
	addChild( editor->getInventorNode() );
    }

    if ( editor ) editor->turnOn(yn);
}


bool EMObjectDisplay::isEditingEnabled() const
{ return editor && editor->isOn(); }


EM::SectionID EMObjectDisplay::getSectionID(int visid) const
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	if ( sections[idx]->id()==visid )
	    return sectionids[idx];
    }

    return -1;
}


EM::SectionID EMObjectDisplay::getSectionID(const TypeSet<int>* path) const
{
    for ( int idx=0; path && idx<path->size(); idx++ )
    {
	const EM::SectionID sectionid = getSectionID((*path)[idx]);
	if ( sectionid!=-1 ) return sectionid;
    }

    return -1;
}


visBase::VisualObject*
EMObjectDisplay::createSection( Geometry::Element* ge )
{
    mDynamicCastGet( Geometry::CubicBezierSurface*, cbs, ge );
    if ( cbs )
    {
	visBase::CubicBezierSurface* surf =
	    		visBase::CubicBezierSurface::create();
	surf->setSurface( *cbs, false, true );
	return surf;
    }

    return 0;
}


void EMObjectDisplay::removeAttribCache()
{
    deepEraseArr( attribcache );
    attribcachesz.erase();
}


};
