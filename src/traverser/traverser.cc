//
// STARSpan project
// Traverse
// Carlos A. Rueda
// $Id: traverser.cc,v 1.33 2008-07-08 07:43:20 crueda Exp $
// See traverser.h for public documentation
//

#include "traverser.h"           

#include <cstdlib>
#include <cassert>
#include <cstring>

// for polygon processing:
#include "geos/opPolygonize.h"

#if GEOS_VERSION_MAJOR >= 3
	using namespace geos::operation::polygonize;
#endif


bool Traverser::_resetReading = true;

Traverser::Traverser() {
	vect = 0;
	desired_FID = -1;
	desired_fieldName = "";
	desired_fieldValue = "";
	
	// buffer: note that we won't allocate minimumBandBufferSize
	// bytes, but assume the biggest data type, double.  See below.
	bandValues_buffer = 0;
	minimumBandBufferSize = 0;
	
	// assume observers will be all simple:
	notSimpleObserver = false;
	
	lineRasterizer = 0;
	progress_out = 0;
	logstream = 0;
    
    memset(&summary, 0, sizeof(summary));
    
	debug_dump_polys = getenv("STARSPAN_DUMP_POLYS_ON_EXCEPTION") != 0;
	debug_no_spatial_filter = getenv("STARSPAN_NO_SPATIAL_FILTER") != 0;
}


void Traverser::addObserver(Observer* aObserver) { 
	observers.push_back(aObserver);
	//if at least one observer is not simple...
	if ( !aObserver->isSimple() )
		notSimpleObserver = true;
}


void Traverser::releaseObservers(void) { 
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
		delete *obs;
	
	observers.clear();
	notSimpleObserver = false;
}


void Traverser::setDesiredFID(long FID) {
	desired_FID = FID; 
}


void Traverser::setDesiredFeatureByField(const char* field_name, const char* field_value) {
	desired_fieldName = field_name;
	desired_fieldValue = field_value;
}


void Traverser::setVector(Vector* vector) {
	if ( vect )
		cerr<< "traverser: Warning: resetting vector\n";
	vect = vector;
}

void Traverser::setLayerNum(int vector_layernum) {
	layernum = vector_layernum;
}


//
//
void Traverser::addRaster(Raster* raster) {
	rasts.push_back(raster);
	
	GDALDataset* dataset = raster->getDataset();
	
	// add band info from given raster
	for ( int i = 0; i < dataset->GetRasterCount(); i++ ) {
		GDALRasterBand* band = dataset->GetRasterBand(i+1);
		globalInfo.bands.push_back(band);
		
		// update minimumBandBufferSize:
		GDALDataType bandType = band->GetRasterDataType();
		int bandTypeSize = GDALGetDataTypeSize(bandType) >> 3;
		minimumBandBufferSize += bandTypeSize;
	}
	
	if ( rasts.size() == 1 ) {
		// info taken from first raster.
		
		rasts[0]->getSize(&width, &height, NULL);
		rasts[0]->getCoordinates(&x0, &y0, &x1, &y1);
		rasts[0]->getPixelSize(&pix_x_size, &pix_y_size);
		pix_abs_area = fabs(pix_x_size*pix_y_size);
		
		// create a geometry for raster envelope:
		OGRLinearRing raster_ring;
		raster_ring.addPoint(x0, y0);
		raster_ring.addPoint(x1, y0);
		raster_ring.addPoint(x1, y1);
		raster_ring.addPoint(x0, y1);
		globalInfo.rasterPoly.addRing(&raster_ring);
		globalInfo.rasterPoly.closeRings();
		globalInfo.rasterPoly.getEnvelope(&raster_env);
		globalInfo.width = width;
		globalInfo.height = height;
	}
	
	if ( rasts.size() > 1 ) {
		// check compatibility against first raster:
		// (rather strict for now)
		
		int last = rasts.size() -1;
		int _width, _height;
		double _x0, _y0, _x1, _y1;
		double _pix_x_size, _pix_y_size;
		rasts[last]->getSize(&_width, &_height, NULL);
		rasts[last]->getCoordinates(&_x0, &_y0, &_x1, &_y1);
		rasts[last]->getPixelSize(&_pix_x_size, &_pix_y_size);

		if ( _width != width || _height != height ) {
			cerr<< "Different number of lines/cols\n";
			exit(1);
		}
		if ( _x0 != x0 || _y0 != y0 || _x1 != x1 || _y1 != y1) {
			cerr<< "Different geographic location\n";
			exit(1);
		}
		if ( _pix_x_size != pix_x_size || _pix_y_size != pix_y_size ) {
			cerr<< "Different pixel size\n";
			exit(1);
		}
	}
}

void Traverser::removeRasters() {
	rasts.clear();
	globalInfo.bands.clear();
	// make sure we have a an empty rasterPoly:
	globalInfo.rasterPoly.empty();
	memset(&summary, 0, sizeof(summary));
}

//
// destroys this traverser
//
Traverser::~Traverser() {
	if ( bandValues_buffer )
		delete[] bandValues_buffer;
	if ( lineRasterizer )
		delete lineRasterizer;
}

void* Traverser::getBandValuesForPixel(int col, int row, void* buffer) {
	char* ptr = (char*) buffer;
	for ( unsigned i = 0; i < globalInfo.bands.size(); i++ ) {
		GDALRasterBand* band = globalInfo.bands[i];
		GDALDataType bandType = band->GetRasterDataType();
	
		int status = band->RasterIO(
			GF_Read,
			col, row,
			1, 1,             // nXSize, nYSize
			ptr,              // pData
			1, 1,             // nBufXSize, nBufYSize
			bandType,         // eBufType
			0, 0              // nPixelSpace, nLineSpace
		);
		
		if ( status != CE_None ) {
			cerr<< "Error reading band value, status= " <<status<< "\n";
			exit(1);
		}
		
		int bandTypeSize = GDALGetDataTypeSize(bandType) >> 3;
		ptr += bandTypeSize;
	}
	
	return buffer;
}


void Traverser::getBandValuesForPixel(int col, int row) {
	assert(bandValues_buffer);
	getBandValuesForPixel(col, row, bandValues_buffer);
}


int Traverser::getPixelIntegerValuesInBand(
	unsigned band_index, 
	vector<int>& list
) {
	if ( band_index <= 0 || band_index > globalInfo.bands.size() ) {
		cerr<< "Traverser::getPixelIntegerValuesInBand: band_index " <<band_index<< " out of range\n";
		return 1;
	}
	
	GDALRasterBand* band = globalInfo.bands[band_index-1];
	PixSet::Iterator* iter = pixset.iterator();
	while ( iter->hasNext() ) {
		int col, row;
		iter->next(&col, &row);
		int value = 0;
		if ( col < 0 || col >= width || row < 0 || row >= height ) {
			// nothing:  keep the 0 value
		}
		else {
			int status = band->RasterIO(
				GF_Read,
				col, row,
				1, 1,             // nXSize, nYSize
				&value,           // pData
				1, 1,             // nBufXSize, nBufYSize
				GDT_Int32,        // eBufType
				0, 0              // nPixelSpace, nLineSpace
			);
			
			if ( status != CE_None ) {
				cerr<< "Error reading band value, status=" <<status<< "\n";
				exit(1);
			}
		}
		list.push_back(value);
	}
	delete iter;
	return 0;
}

int Traverser::getPixelDoubleValuesInBand(
	unsigned band_index, 
	vector<double>& list
) {
	if ( band_index <= 0 || band_index > globalInfo.bands.size() ) {
		cerr<< "Traverser::getPixelDoubleValuesInBand: band_index " <<band_index<< " out of range\n";
		return 1;
	}
	
	GDALRasterBand* band = globalInfo.bands[band_index-1];
	PixSet::Iterator* iter = pixset.iterator();
	while ( iter->hasNext() ) {
		int col, row;
		iter->next(&col, &row);
		double value = 0.0;
		if ( col < 0 || col >= width || row < 0 || row >= height ) {
			// nothing:  keep the 0.0 value
		}
		else {
			int status = band->RasterIO(
				GF_Read,
				col, row,
				1, 1,             // nXSize, nYSize
				&value,           // pData
				1, 1,             // nBufXSize, nBufYSize
				GDT_Float64,      // eBufType
				0, 0              // nPixelSpace, nLineSpace
			);
			
			if ( status != CE_None ) {
				cerr<< "Error reading band value, status=" <<status<< "\n";
				exit(1);
			}
		}
		list.push_back(value);
	}
	delete iter;
	return 0;
}




//
// Implementation as a LineRasterizerObserver, but also called directly. 
// Checks for duplicate pixel.
//
void Traverser::pixelFound(double x, double y) {
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

	TraversalEvent event(col, row, x, y);
	summary.num_processed_pixels++;
	
	// if at least one observer is not simple...
	if ( notSimpleObserver ) {
		// get also band values
		getBandValuesForPixel(col, row);
		event.bandValues = bandValues_buffer;
	}
	
	// notify observers:
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
		(*obs)->addPixel(event);
	
	// keep track of processed pixels
	pixset.insert(col, row);
}

//
// process a point intersection
//
void Traverser::processPoint(OGRPoint* point) {
	pixelFound(point->getX(), point->getY());
}

//
// process a multi point intersection
//
void Traverser::processMultiPoint(OGRMultiPoint* pp) {
	for ( int i = 0; i < pp->getNumGeometries(); i++ ) {
		OGRPoint* point = (OGRPoint*) pp->getGeometryRef(i);
		pixelFound(point->getX(), point->getY());
	}
}


//
// process a line string intersection
//
void Traverser::processLineString(OGRLineString* linstr) {
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
void Traverser::processMultiLineString(OGRMultiLineString* coll) {
	for ( int i = 0; i < coll->getNumGeometries(); i++ ) {
		OGRLineString* linstr = (OGRLineString*) coll->getGeometryRef(i);
		processLineString(linstr);
	}
}


//
// processValidPolygon(Polygon* geos_poly): Process a valid polygon.
// Direct call to processValidPolygon_QT(geos_poly).
//
void Traverser::processValidPolygon(Polygon* geos_poly) {
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
void Traverser::processPolygon(OGRPolygon* poly) {
	Polygon* geos_poly = (Polygon*) poly->exportToGEOS();
	if ( geos_poly->isValid() ) {
        // 2008-04-18
        if ( geos_poly->getNumInteriorRing() > 0 ) {
            cerr<< "--Valid polygon WITH interior rings: " <<geos_poly->getNumInteriorRing()<< endl;
        } 
		processValidPolygon(geos_poly);
	}
	else {
		summary.num_invalid_polys++;
		
		if ( globalOptions.skip_invalid_polys ) {
			//cerr<< "--skipping invalid polygon--"<< endl;
			if ( debug_dump_polys ) {
				cerr<< "geos_poly = " << wktWriter.write(geos_poly) << endl;
			}
		}
		else {
			// try to explode this poly into smaller ones:
			if ( geos_poly->getNumInteriorRing() > 0 ) {
				cerr<< "--Invalid polygon has " <<geos_poly->getNumInteriorRing()
                    << " interior rings: cannot explode it--" << endl;
				summary.num_polys_with_internal_ring++;
			} 
			else {
				const LineString* lines = geos_poly->getExteriorRing();
				// get noded linestring:
				Geometry* noded = 0;
				const int num_points = lines->getNumPoints();
                if ( globalOptions.verbose ) {
                    cout << "Exploding external ring with " <<num_points<< " points...\n";
                }
				const CoordinateSequence* coordinates = lines->getCoordinatesRO();
				for ( int i = 1; i < num_points; i++ ) {
                    if ( globalOptions.verbose && i % 1000 == 0 ) {
                        cout << "\tpoint " <<i<< "\n";
                    }
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
						if ( globalOptions.verbose ) {
							cout << polys->size() << " sub-polys obtained\n";
                        }
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
void Traverser::processMultiPolygon(OGRMultiPolygon* mpoly) {
	for ( int i = 0; i < mpoly->getNumGeometries(); i++ ) {
		OGRPolygon* poly = (OGRPolygon*) mpoly->getGeometryRef(i);
		processPolygon(poly);
	}
}

//
// process a geometry collection intersection.
//
void Traverser::processGeometryCollection(OGRGeometryCollection* coll) {
	for ( int i = 0; i < coll->getNumGeometries(); i++ ) {
		OGRGeometry* geo = (OGRGeometry*) coll->getGeometryRef(i);
		processGeometry(geo, false);
	}
}

//
// get intersection type and process accordingly
//
void Traverser::processGeometry(OGRGeometry* intersection_geometry, bool count) {
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
void Traverser::process_feature(OGRFeature* feature) {
	if ( globalOptions.verbose ) {
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

	///////////////////////////////////////////////////////////////////
	//
	// apply box operation if so indicated:
	//
	if ( globalOptions.boxParams.given ) {
        //
        // Create a rectangle with the given box sizes and centered w.r.t.
        // the bounding box of the feature geometry.
        //
        
        // requested box dimensions:
        double rbw, rbh;
        globalOptions.boxParams.getParsedDims(&rbw, &rbh);
        
        // bounding box
        OGREnvelope bbox;
        feature_geometry->getEnvelope(&bbox);
        
		// create a geometry for the requested box:
        
        // sizes of bounding box:
        double bbw = bbox.MaxX - bbox.MinX; 
        double bbh = bbox.MaxY - bbox.MinY;
        
        // corners of requested box:
        double x0 = bbox.MinX - (rbw - bbw) / 2; 
        double y0 = bbox.MinY - (rbh - bbh) / 2;
        double x1 = x0 + rbw;
        double y1 = y0 + rbh;
        
		OGRLinearRing ring;
		ring.addPoint(x0, y0);
		ring.addPoint(x1, y0);
		ring.addPoint(x1, y1);
		ring.addPoint(x0, y1);
        OGRPolygon* boxPoly = new OGRPolygon();
		boxPoly->addRing(&ring);
		boxPoly->closeRings();
		geometryToIntersect = boxPoly;
        //
        // Note: this new polygon will be deleted at the end of this method.
        //
    }
    
	///////////////////////////////////////////////////////////////////
	//
	// else: apply buffer operation if so indicated:
	//
	else if ( globalOptions.bufferParams.given ) {
		OGRGeometry* buffered_geometry = 0;
		
		// get distance:
		double distance; 
		if ( globalOptions.bufferParams.distance[0] == '@' ) {
			const char* attr = globalOptions.bufferParams.distance.c_str() + 1;
			int index = feature->GetFieldIndex(attr);
			if ( index < 0 ) {
				cerr<< "\n\tField `" <<attr<< "' not found\n";
				return;
			}
			distance = feature->GetFieldAsInteger(index);
		}
		else {
			distance = atoi(globalOptions.bufferParams.distance.c_str());
		}
		
		// get quadrantSegments:
		int quadrantSegments;
		if ( globalOptions.bufferParams.quadrantSegments[0] == '@' ) {
			const char* attr = globalOptions.bufferParams.quadrantSegments.c_str() + 1;
			int index = feature->GetFieldIndex(attr);
			if ( index < 0 ) {
				cerr<< "\n\tField `" <<attr<< "' not found\n";
				return;
			}
			quadrantSegments = feature->GetFieldAsInteger(index);
		}
		else {
			quadrantSegments = atoi(globalOptions.bufferParams.quadrantSegments.c_str());
		}
		
		
		
		try {
			buffered_geometry = feature_geometry->Buffer(distance, quadrantSegments);
		}
		catch(GEOSException* ex) {
			cerr<< ">>>>> FID: " << feature->GetFID()
				<< "  GEOSException: " << EXC_STRING(ex) << endl;
			return;
		}
		if ( !buffered_geometry ) {
			cout<< ">>>>> FID: " << feature->GetFID()
				<< "  null buffered result!" << endl;
			return;
		}
		
		geometryToIntersect = buffered_geometry;
		// This geometry will be deleted at the end of this method.
	}
	
	
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
		if ( globalOptions.verbose ) {
			cout<< " NO INTERSECTION:\n";
		}
		goto done;
	}

	summary.num_intersecting_features++;

	if ( globalOptions.verbose ) {
		cout<< " Type of intersection: "
		    << intersection_geometry->getGeometryName()<< endl
		;
	}
	
    
    IntersectionInfo intersInfo;
    intersInfo.trv = this;
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
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ ) {
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
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ ) {
		(*obs)->intersectionEnd(intersInfo);
	}

done:
	delete intersection_geometry;
	if ( geometryToIntersect != feature_geometry ) {
		delete geometryToIntersect;
	}
}


//
// main method for traversal
//
void Traverser::traverse() {
	// don't allow a recursive call
	if ( bandValues_buffer ) {
		cerr<< "traverser.traverse: recursive call!\n";
		return;
	}

	// do some checks:
	if ( observers.size() == 0 ) {
		cerr<< "traverser: No observers registered!\n";
		return;
	}

	if ( !vect ) {
		cerr<< "traverser: Vector datasource not specified!\n";
		return;
	}
	if ( rasts.size() == 0 ) {
		cerr<< "traverser: No raster datasets were specified!\n";
		return;
	}
    
    OGRLayer* layer = 0;
    bool releaseLayer = false;
    
    // SQL statement?
    if ( globalOptions.vSelParams.sql.length() > 0 ) {
        OGRDataSource *poDS = vect->getDataSource();
        
		const char *dialect = 0;
        if ( globalOptions.vSelParams.dialect.length() > 0 ) {
            dialect = globalOptions.vSelParams.dialect.c_str();
        }

        OGRGeometry *poSpatialFilter = 0;  // We do not use this here; TODO perhaps enable it later on.
        
        layer = poDS->ExecuteSQL(globalOptions.vSelParams.sql.c_str(), poSpatialFilter, dialect);
        if ( layer != 0 ) {
            if ( globalOptions.vSelParams.where.length() > 0 ) {
                layer->SetAttributeFilter(globalOptions.vSelParams.where.c_str());
            }
            // the OGR API requires this layer to be explicitly released:
            releaseLayer = true;
        }
        else {
            cerr<< "traverser: No result or an error occured while issueing query: "
                << globalOptions.vSelParams.sql << endl;
            return;
        }
    }
	else {
        layer = vect->getLayer(layernum);
        if ( !layer ) {
            cerr<< "Couldn't get layer " <<layernum<< " from " << vect->getName()<< endl;
            return;
        }
        if ( globalOptions.vSelParams.where.length() > 0 ) {
            layer->SetAttributeFilter(globalOptions.vSelParams.where.c_str());
        }
    }
    
	if ( _resetReading ) {
		layer->ResetReading();
	}

    
	if ( globalOptions.boxParams.given ) {
        // parse them:
        if ( globalOptions.boxParams.parse(pix_x_size, pix_y_size) ) {
            cerr<< "\nInvalid box dimension specification:\n"
                << "    width : [" <<globalOptions.boxParams.width<<  "]\n"
                << "    height: [" <<globalOptions.boxParams.height<< "]\n";
            return;
        }
        if ( globalOptions.verbose ) {
            double rbw, rbh;
            globalOptions.boxParams.getParsedDims(&rbw, &rbh);
            cout << "Parsed box dimensions: width=" <<rbw<< " height=" <<rbh<< "\n";
        }
    }    


	lineRasterizer = new LineRasterizer(x0, y0, pix_x_size, pix_y_size);
	lineRasterizer->setObserver(this);
	
	memset(&summary, 0, sizeof(summary));
	
	// assuming biggest data type we assign enough memory:
	bandValues_buffer = new double[globalInfo.bands.size()];

	// for polygon rasterization:
	pixelProportion_times_pix_abs_area = globalOptions.pix_prop * pix_abs_area;

    globalInfo.layer = layer;
    
	//
	// notify observers about initialization of process
	//
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ ) {
		(*obs)->init(globalInfo);
	}

	OGRFeature* feature;
	
	//
	// Was a specific FID given?
	//
	if ( desired_FID >= 0 ) {
		feature = layer->GetFeature(desired_FID);
		if ( !feature ) {
			cerr<< "FID " <<desired_FID<< " not found in " <<vect->getName()<< endl;
			exit(1);
		}
		process_feature(feature);
		delete feature;
	}
	//
	// Was a specific field name/value given?
	//
	else if ( desired_fieldName.size() > 0 ) {
		//
		// search for corresponding feature in vector datasource:
		//
		bool finished = false;
		bool found = false;
		while( !finished && (feature = layer->GetNextFeature()) != NULL ) {
			const int i = feature->GetFieldIndex(desired_fieldName.c_str());
			if ( i < 0 ) {
				finished = true;
			}
			else {
				const char* str = feature->GetFieldAsString(i);
				assert(str);
				if ( desired_fieldValue == string(str) ) {
					process_feature(feature);
					finished = true;
					found = true;
				}
			}
			delete feature;
		}
		// cout<< "   found=" <<found << endl;
	}
	//
	// else: process each feature in vector datasource:
	//
	else {
		
		if ( debug_no_spatial_filter ) {
			cout<< "*** Spatial filtering disabled ***" <<endl;
		}
		else {
			double minX = raster_env.MinX;
			double minY = raster_env.MinY; 
			double maxX = raster_env.MaxX; 
			double maxY = raster_env.MaxY; 
			layer->SetSpatialFilterRect(minX, minY, maxX, maxY);
		}
		
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
	}
	
	//
	// notify observers about finalization of process
	//
	for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ ) {
		(*obs)->end();
	}
    
    if ( releaseLayer ) {
        OGRDataSource *poDS = vect->getDataSource();
        poDS->ReleaseResultSet(layer);
    }

	delete[] bandValues_buffer;
	bandValues_buffer = 0;
	delete lineRasterizer;
	lineRasterizer = 0;
}


void Traverser::reportSummary() {
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
