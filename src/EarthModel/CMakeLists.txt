#_______________________Pmake___________________________________________________
#
#	Makefile : 	Earth Model
# 	MNov 1999	Kris Tingdahl
#_______________________________________________________________________________



SET(OD_MODULE_DEPS Geometry)
set(OD_MODULE_SOURCES
		em2ddatafmtconverter.cc
		embody.cc
		embodyoperator.cc
		embodytr.cc
		emfact.cc
		emfault.cc
		emfault3d.cc
		emfault3dpainter.cc
		emfaultauxdata.cc
		emfaultstickset.cc
		emfaultsticksetflatvieweditor.cc
		emfaultstickpainter.cc
		emfaultset3d.cc
		emfsstofault3d.cc
		emhorizon2d.cc
		emhorizon3d.cc
		emhorizonpainter2d.cc
		emhorizonpainter3d.cc
		emhorizonpreload.cc
		emhorizonztransform.cc
		emhorizonztransformer.cc
		emhorizonutils.cc
		emhor2dto3d.cc
		emioobjinfo.cc
		emmanager.cc
		emmarchingcubessurface.cc
		emobject.cc
		emobjectio.cc
		emobjectposselector.cc
		emprobe.cc
		emrandomposbody.cc
		empolygonbody.cc
		emrandlinegen.cc
		emregion.cc
		emrowcoliterator.cc
		emstoredobjaccess.cc
		emsurface.cc
		emsurfaceauxdata.cc
		emsurfacegeometry.cc
		emsurfaceio.cc
		emsurfaceposprov.cc
		emsurfacetr.cc
		emsurfauxdataio.cc
		faulttrace.cc
		horizongridder.cc
		horizonmerger.cc
		horizonmodifier.cc
		horizonrelation.cc
		horizonscanner.cc
		horizonsorter.cc
		lmkemfaulttransl.cc
		poly2horvol.cc
		emundo.cc
		initearthmodel.cc
)
OD_INIT_MODULE()
