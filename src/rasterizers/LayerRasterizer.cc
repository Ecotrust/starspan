//
// StarSpan project
// LayerRasterizer
// Carlos A. Rueda
// $Id: LayerRasterizer.cc,v 1.1 2008-04-18 07:46:56 crueda Exp $
// See LayerRasterizer.h for public documentation
//

#include "LayerRasterizer.h"

#include <cstdlib>
#include <cassert>
#include <cstring>

// for polygon processing:
#include "geos/opPolygonize.h"

#if GEOS_VERSION_MAJOR >= 3
	using namespace geos::operation::polygonize;
#endif



LayerRasterizer::LayerRasterizer(
    OGRLayer* layer,
    int width, int height,
    double x0, double y0, double x1, double y1,
    double pix_x_size, double pix_y_size
)
: layer(layer),
  pixelProportion(0.5),
  width(width), height(height),
  x0(x0), y0(y0), x1(x1), y1(y1),
  pix_x_size(pix_x_size), pix_y_size(pix_y_size),
  lineRasterizer(0),
  progress_out(0),
  verbose(false),
  logstream(0),
  skip_invalid_polys(false)
{
    pix_abs_area = fabs(pix_x_size*pix_y_size);

    // create a geometry for this envelope:
    OGRLinearRing raster_ring;
    raster_ring.addPoint(x0, y0);
    raster_ring.addPoint(x1, y0);
    raster_ring.addPoint(x1, y1);
    raster_ring.addPoint(x0, y1);
    globalInfo.rasterPoly.addRing(&raster_ring);
    globalInfo.rasterPoly.closeRings();
    globalInfo.rasterPoly.getEnvelope(&raster_env);
}



LayerRasterizer::~LayerRasterizer() {
	if ( lineRasterizer )
		delete lineRasterizer;
}



//
// Implementation as a LineRasterizerObserver, but also called directly. 
// Checks for duplicate pixel.
//
void LayerRasterizer::pixelFound(double x, double y) {
	// get pixel location:
	int col, row;
	toColRow(x, y, &col, &row);
	
	if ( col < 0 || col >= width 
	||   row < 0 || row >= height ) {
		return;
	}
	
	// check this location has not been processed
	if ( pixset.contains(col, row) ) {
		return;
	}
	
	toGridXY(col, row, &x, &y);

	RastEvent event(col, row, x, y);
	summary.num_processed_pixels++;
	
	// notify observers:
	for ( vector<LayerRasterizerObserver*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
		(*obs)->addPixel(event);
	
	// keep track of processed pixels
	pixset.insert(col, row);
}


//
// process a point intersection
//
void LayerRasterizer::processPoint(OGRPoint* point) {
	pixelFound(point->getX(), point->getY());
}

//
// process a multi point intersection
//
void LayerRasterizer::processMultiPoint(OGRMultiPoint* pp) {
	for ( int i = 0; i < pp->getNumGeometries(); i++ ) {
		OGRPoint* point = (OGRPoint*) pp->getGeometryRef(i);
		pixelFound(point->getX(), point->getY());
	}
}


//
// process a line string intersection
//
void LayerRasterizer::processLineString(OGRLineString* linstr) {
	int num_points = linstr->getNumPoints();
	//cout<< "      num_points = " <<num_points<< endl;
	if ( num_points > 0 ) {
		OGRPoint point0;
		linstr->getPoint(0, &point0);
		//
		// Note that connecting pixels between lines are not repeated.
		//
		for ( int i = 1; i < num_points; i++ ) {
			OGRPoint point;
			linstr->getPoint(i, &point);

			//
			// Traverse line from point0 to point:
			//
			bool last = i == num_points - 1;
			lineRasterizer->line(point0.getX(), point0.getY(), point.getX(), point.getY(), last);
			
			point0 = point;
		}
	}
}

//
// process a multi line string intersection
//
void LayerRasterizer::processMultiLineString(OGRMultiLineString* coll) {
	for ( int i = 0; i < coll->getNumGeometries(); i++ ) {
		OGRLineString* linstr = (OGRLineString*) coll->getGeometryRef(i);
		processLineString(linstr);
	}
}


//
// processValidPolygon(Polygon* geos_poly): Process a valid polygon.
// Direct call to processValidPolygon_QT(geos_poly).
//
void LayerRasterizer::processValidPolygon(Polygon* geos_poly) {
	processValidPolygon_QT(geos_poly);
}


// To avoid some OGR overhead, I use GEOS directly.
GeometryFactory* global_factory = new GeometryFactory();
const CoordinateSequenceFactory* global_cs_factory = global_factory->getCoordinateSequenceFactory();

	
//
// process a polygon intersection.
// The area of intersections are used to determine if a pixel is to be
// included.  
//
void LayerRasterizer::processPolygon(OGRPolygon* poly) {
	Polygon* geos_poly = (Polygon*) poly->exportToGEOS();
	if ( geos_poly->isValid() ) {
		processValidPolygon(geos_poly);
	}
	else {
		summary.num_invalid_polys++;
		
		if ( skip_invalid_polys ) {
			//cerr<< "--skipping invalid polygon--"<< endl;
			if ( debug_dump_polys ) {
				cerr<< "geos_poly = " << wktWriter.write(geos_poly) << endl;
			}
		}
		else {
			// try to explode this poly into smaller ones:
			if ( geos_poly->getNumInteriorRing() > 0 ) {
				//cerr<< "--Invalid polygon has interior rings: cannot explode it--" << endl;
				summary.num_polys_with_internal_ring++;
			} 
			else {
				const LineString* lines = geos_poly->getExteriorRing();
				// get noded linestring:
				Geometry* noded = 0;
				const int num_points = lines->getNumPoints();
				const CoordinateSequence* coordinates = lines->getCoordinatesRO();
				for ( int i = 1; i < num_points; i++ ) {
					vector<Coordinate>* subcoordinates = new vector<Coordinate>();
					subcoordinates->push_back(coordinates->getAt(i-1));
					subcoordinates->push_back(coordinates->getAt(i));
					CoordinateSequence* cs = global_cs_factory->create(subcoordinates);
					LineString* ln = global_factory->createLineString(cs);
					
					if ( !noded )
						noded = ln;
					else {
						noded = noded->Union(ln);
						delete ln;
					}
				}
				
				if ( noded ) {
					// now, polygonize:
					Polygonizer polygonizer;
					polygonizer.add(noded);
					
					// and process generated sub-polygons:
					vector<Polygon*>* polys = polygonizer.getPolygons();
					if ( polys ) {
						summary.num_polys_exploded++;
						summary.num_sub_polys += polys->size();
						if ( verbose )
							cout << polys->size() << " sub-polys obtained\n";
						for ( unsigned i = 0; i < polys->size(); i++ ) {
							processValidPolygon((*polys)[i]);
						}
					}
					else {
						cerr << "could not explode polygon\n";
					}
					
					delete noded;
				}
			}
		}
	}
	delete geos_poly;
}


//
// process a multi-polygon intersection.
//
void LayerRasterizer::processMultiPolygon(OGRMultiPolygon* mpoly) {
	for ( int i = 0; i < mpoly->getNumGeometries(); i++ ) {
		OGRPolygon* poly = (OGRPolygon*) mpoly->getGeometryRef(i);
		processPolygon(poly);
	}
}

//
// process a geometry collection intersection.
//
void LayerRasterizer::processGeometryCollection(OGRGeometryCollection* coll) {
	for ( int i = 0; i < coll->getNumGeometries(); i++ ) {
		OGRGeometry* geo = (OGRGeometry*) coll->getGeometryRef(i);
		processGeometry(geo, false);
	}
}

//
// get intersection type and process accordingly
//
void LayerRasterizer::processGeometry(OGRGeometry* intersection_geometry, bool count) {
	OGRwkbGeometryType intersection_type = intersection_geometry->getGeometryType();
	switch ( intersection_type ) {
		case wkbPoint:
		case wkbPoint25D:
			if ( count ) 
				summary.num_point_features++;
			processPoint((OGRPoint*) intersection_geometry);
			break;
	
		case wkbMultiPoint:
		case wkbMultiPoint25D:
			if ( count ) 
				summary.num_multipoint_features++;
			processMultiPoint((OGRMultiPoint*) intersection_geometry);
			break;
	
		case wkbLineString:
		case wkbLineString25D:
			if ( count ) 
				summary.num_linestring_features++;
			processLineString((OGRLineString*) intersection_geometry);
			break;
	
		case wkbMultiLineString:
		case wkbMultiLineString25D:
			if ( count ) 
				summary.num_multilinestring_features++;
			processMultiLineString((OGRMultiLineString*) intersection_geometry);
			break;
			
		case wkbPolygon:
		case wkbPolygon25D:
			if ( count ) 
				summary.num_polygon_features++;
			processPolygon((OGRPolygon*) intersection_geometry);
			break;
			
		case wkbMultiPolygon:
		case wkbMultiPolygon25D:
			if ( count ) 
				summary.num_multipolygon_features++;
			processMultiPolygon((OGRMultiPolygon*) intersection_geometry);
			break;
			
		case wkbGeometryCollection:
		case wkbGeometryCollection25D:
			if ( count ) 
				summary.num_geometrycollection_features++;
			processGeometryCollection((OGRGeometryCollection*) intersection_geometry);
			break;
			
		default:
			throw (string(OGRGeometryTypeToName(intersection_type))
			    + ": intersection type not considered."
			);
	}
}


//
// processes a given feature
//
void LayerRasterizer::process_feature(OGRFeature* feature) {
	if ( verbose ) {
		fprintf(stdout, "\n\nFID: %ld", feature->GetFID());
	}
	
	//
	// get feature geometry
	//
	OGRGeometry* feature_geometry = feature->GetGeometryRef();

	//
	// the geometry to be interected with raster envelope,
    // initialized with feature's geometry:
	//
	OGRGeometry* geometryToIntersect = feature_geometry;


	//
	// intersect this feature with raster (raster ring)
	//
	OGRGeometry* intersection_geometry = 0;
	
	try {
		intersection_geometry = globalInfo.rasterPoly.Intersection(geometryToIntersect);
	}
	catch(GEOSException* ex) {
		cerr<< ">>>>> FID: " << feature->GetFID()
		    << "  GEOSException: " << EXC_STRING(ex) << endl;
		goto done;
	}

	if ( !intersection_geometry ) {
		if ( verbose ) {
			cout<< " NO INTERSECTION:\n";
		}
		goto done;
	}

	summary.num_intersecting_features++;

	if ( verbose ) {
		cout<< " Type of intersection: "
		    << intersection_geometry->getGeometryName()<< endl
		;
	}
	
    
    RastIntersectionInfo intersInfo;
    intersInfo.feature = feature;
    intersInfo.geometryToIntersect = geometryToIntersect;
    intersInfo.intersection_geometry = intersection_geometry;
    
	//
	// Notify observers about this feature:
	// NOTE: Particularly in the case of a GeometryCollection, it might be the
	// case that this feature is found to be an intersecting one even though
	// that no pixel will be found to be intersecting. Observers may in general
	// want to confirm the actual intersection by looking at the number of pixels
	// that are reported to be intersected.
	// 
	for ( vector<LayerRasterizerObserver*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ ) {
		(*obs)->intersectionFound(intersInfo);
	}
	
	pixset.clear();
	try {
		processGeometry(intersection_geometry, true);
	}
	catch(string err) {
		cerr<< "starspan: FID=" <<feature->GetFID()
		    << ", " << OGRGeometryTypeToName(feature_geometry->getGeometryType())
		    << endl << err << endl;
	}

	//
	// notify observers that processing of this feature has finished
	// 
	for ( vector<LayerRasterizerObserver*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ ) {
		(*obs)->intersectionEnd(intersInfo);
	}

done:
	delete intersection_geometry;
	if ( geometryToIntersect != feature_geometry ) {
		delete geometryToIntersect;
	}
}


//
// main method 
//
void LayerRasterizer::rasterize() {
	// do some checks:
	if ( observers.size() == 0 ) {
		cerr<< "traverser: No observers registered!\n";
		return;
	}

    
   	lineRasterizer = new LineRasterizer(x0, y0, pix_x_size, pix_y_size);
	lineRasterizer->setObserver(this);
	
	memset(&summary, 0, sizeof(summary));

    
	// for polygon rasterization:
	pixelProportion_times_pix_abs_area = pixelProportion * pix_abs_area;

    globalInfo.layer = layer;
    
	//
	// notify observers about initialization of process
	//
	for ( vector<LayerRasterizerObserver*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ ) {
		(*obs)->init(globalInfo);
	}

	OGRFeature* feature;
	
        double minX = raster_env.MinX;
        double minY = raster_env.MinY; 
        double maxX = raster_env.MaxX; 
        double maxY = raster_env.MaxY; 
        layer->SetSpatialFilterRect(minX, minY, maxX, maxY);
    
		Progress* progress = 0;
		if ( progress_out ) {
			long psize = layer->GetFeatureCount();
			if ( psize >= 0 ) {
				progress = new Progress(psize, progress_perc, *progress_out);
			}
			else {
				progress = new Progress((long)progress_perc, *progress_out);
			}
			*progress_out << "\t";
			progress->start();
		}
		while( (feature = layer->GetNextFeature()) != NULL ) {
			process_feature(feature);
			delete feature;
			if ( progress )
				progress->update();
		}
		if ( progress ) {
			progress->complete();
			delete progress;
			progress = 0;
			*progress_out << endl;
		}

        
	//
	// notify observers about finalization of process
	//
	for ( vector<LayerRasterizerObserver*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ ) {
		(*obs)->end();
	}
    
	delete lineRasterizer;
	lineRasterizer = 0;
}


void LayerRasterizer::reportSummary() {
	cout<< "Summary:" <<endl;
	cout<< "  Intersecting features: " << summary.num_intersecting_features<< endl;
	if ( summary.num_point_features )
		cout<< "      Points: " <<summary.num_point_features<< endl;
	if ( summary.num_multipoint_features )
		cout<< "      MultiPoints: " <<summary.num_multipoint_features<< endl;
	if ( summary.num_linestring_features )
		cout<< "      LineStrings: " <<summary.num_linestring_features<< endl;
	if ( summary.num_multilinestring_features )
		cout<< "      MultiLineStrings: " <<summary.num_multilinestring_features<< endl;
	if ( summary.num_polygon_features )
		cout<< "      Polygons: " <<summary.num_polygon_features<< endl;
	if ( summary.num_invalid_polys )
		cout<< "          invalid: " <<summary.num_invalid_polys<< endl;
	if ( summary.num_polys_with_internal_ring )
		cout<< "          with internalring: " <<summary.num_polys_with_internal_ring<< endl;
	if ( summary.num_polys_exploded )
		cout<< "          exploded: " <<summary.num_polys_exploded<< endl;
	if ( summary.num_sub_polys )
		cout<< "          sub polys: " <<summary.num_sub_polys<< endl;
	if ( summary.num_multipolygon_features )
		cout<< "      MultiPolygons: " <<summary.num_multipolygon_features<< endl;
	if ( summary.num_geometrycollection_features )
		cout<< "      GeometryCollections: " <<summary.num_geometrycollection_features<< endl;
	cout<< endl;
	cout<< "  Processed pixels: " <<summary.num_processed_pixels<< endl;
}
