//
// STARSpan project
// Carlos A. Rueda
// starspan_dump - dump geometries (testing routine)
// $Id: starspan_dump.cc,v 1.3 2008-04-11 19:15:12 crueda Exp $
//

#include "starspan.h"           
#include "traverser.h"       
#include <iostream>
#include <cstdlib>

#include "unistd.h"   // unlink           
#include <string>
#include <stdlib.h>
#include <stdio.h>


using namespace std;

static void processPoint(OGRPoint* point, FILE* file) {
	fprintf(file, "DataSet: Point\n");
	fprintf(file, "%10.3f , %10.3f\n", point->getX(), point->getY());
}
static void processMultiPoint(OGRMultiPoint* pp, FILE* file) {
	for ( int i = 0; i < pp->getNumGeometries(); i++ ) {
		OGRPoint* point = (OGRPoint*) pp->getGeometryRef(i);
		processPoint(point, file);
	}
}
static void processLineString(OGRLineString* linstr, FILE* file) {
	fprintf(file, "DataSet: LineString\n");
	int num_points = linstr->getNumPoints();
	for ( int i = 0; i < num_points; i++ ) {
		OGRPoint point;
		linstr->getPoint(i, &point);
		fprintf(file, "%10.3f , %10.3f\n", point.getX(), point.getY());
	}
}
static void processMultiLineString(OGRMultiLineString* coll, FILE* file) {
	for ( int i = 0; i < coll->getNumGeometries(); i++ ) {
		OGRLineString* linstr = (OGRLineString*) coll->getGeometryRef(i);
		processLineString(linstr, file);
	}
}
static void processPolygon(OGRPolygon* poly, FILE* file) {
	OGRLinearRing* ring = poly->getExteriorRing();
	processLineString(ring, file);
}


static void dumpFeature(OGRFeature* feature, FILE* file) {
	fprintf(file, "DataSet: FID=%ld\n", feature->GetFID());

	OGRGeometry* geometry = feature->GetGeometryRef();
	OGRwkbGeometryType type = geometry->getGeometryType();
	switch ( type ) {
		case wkbPoint:
		case wkbPoint25D:
			processPoint((OGRPoint*) geometry, file);
			break;
	
		case wkbMultiPoint:
		case wkbMultiPoint25D:
			processMultiPoint((OGRMultiPoint*) geometry, file);
			break;
	
		case wkbLineString:
		case wkbLineString25D:
			processLineString((OGRLineString*) geometry, file);
			break;
	
		case wkbMultiLineString:
		case wkbMultiLineString25D:
			processMultiLineString((OGRMultiLineString*) geometry, file);
			break;
			
		case wkbPolygon:
		case wkbPolygon25D:
			processPolygon((OGRPolygon*) geometry, file);
			break;
			
		default:
			cerr<< "dump: " 
				<< OGRGeometryTypeToName(type)
				<< ": intersection type not considered\n"
			;
	}
}





/**
  * Writes geometries and grid to a ptplot file.
  */
class DumpObserver : public Observer {
public:
	OGRLayer* layer;
	Raster* rast;
	GlobalInfo* global_info;
	FILE* file;
	long pixel_count;
	double pix_x_size, pix_y_size;

	/**
	  * Creates a dump object
	  */
	DumpObserver(OGRLayer* layer, Raster* rast, FILE* f)
	: layer(layer), rast(rast), file(f)
	{
		global_info = 0;
		pixel_count = 0;
	}
	
	/**
	  * simply calls end()
	  */
	~DumpObserver() {
		end();
	}
	
	/**
	  * finalizes current feature if any; closes the file
	  */
	void end() {
		if ( file ) {
			fclose(file);
			cout<< "dump: finished.\n";
			file = 0;
		}
	}
	
	/**
	  * returns true.
	  */
	bool isSimple() { 
		return true; 
	}

	/**
	  * Creates grid according to raster
	  */
	void init(GlobalInfo& info) {
		global_info = &info;

		double x0, y0, x1, y1;
		rast->getCoordinates(&x0, &y0, &x1, &y1);
		rast->getPixelSize(&pix_x_size, &pix_y_size);
		if ( x0 > x1 ) {
			double tmp = x0;
			x0 = x1;
			x1 = tmp;
		}
		if ( y0 > y1 ) {
			double tmp = y0;
			y0 = y1;
			y1 = tmp;
		}
		double abs_pix_x_size = pix_x_size;
		double abs_pix_y_size = pix_y_size;
		if ( abs_pix_x_size < 0.0 )
			abs_pix_x_size = -abs_pix_x_size;
		if ( abs_pix_y_size < 0.0 )
			abs_pix_y_size = -abs_pix_y_size;
		
		fprintf(file, "XTicks:");
		for ( double x = x0; x <= x1; x += abs_pix_x_size )
			fprintf(file, " %10.3f %10.3f,", x, x);
		fprintf(file, "\n");
		fprintf(file, "YTicks:");
		for ( double y = y0; y <= y1; y += abs_pix_y_size )
			fprintf(file, " %10.3f %10.3f,", y, y);
		fprintf(file, "\n");


		fprintf(file, "DataSet: grid_envelope\n");
		fprintf(file, "%f , %f\n", x0, y0);
		fprintf(file, "%f , %f\n", x1, y0);
		fprintf(file, "%f , %f\n", x1, y1);
		fprintf(file, "%f , %f\n", x0, y1);
		fprintf(file, "%f , %f\n", x0, y0);
	}


	/**
	  * Inits creation of datasets corresponding to new feature
	  */
	void intersectionFound(IntersectionInfo& intersInfo) {
		dumpFeature(intersInfo.feature, file);
	}

	/**
	  * writes out the point in the given event.
	  */
	void addPixel(TraversalEvent& ev) {
		fprintf(file, "DataSet: Pixel\n");
		if ( globalOptions.use_pixpolys ) {
			fprintf(file, "%f , %f\n", ev.pixel.x, ev.pixel.y);
			fprintf(file, "%f , %f\n", ev.pixel.x + pix_x_size, ev.pixel.y);
			fprintf(file, "%f , %f\n", ev.pixel.x + pix_x_size, ev.pixel.y + pix_y_size);
			fprintf(file, "%f , %f\n", ev.pixel.x, ev.pixel.y + pix_y_size);
			fprintf(file, "%f , %f\n", ev.pixel.x, ev.pixel.y);
		}
		else {
			fprintf(file, "%f , %f\n", ev.pixel.x, ev.pixel.y);
		}
	}
};



Observer* starspan_dump(
	Traverser& tr,
	const char* filename
) {	
	if ( !tr.getVector() ) {
		cerr<< "vector datasource expected\n";
		return 0;
	}

	int layernum = tr.getLayerNum();
	OGRLayer* layer = tr.getVector()->getLayer(layernum);
	if ( !layer ) {
		cerr<< "warning: No layer 0 found\n";
		return 0;
	}

	if ( tr.getNumRasters() == 0 ) {
		cerr<< "raster expected\n";
		return 0;
	}
	Raster* rast = tr.getRaster(0);

	// create output file
	FILE* file = fopen(filename, "w");
	if ( !file ) {
		cerr << "Couldn't create " << filename << endl;
		return 0;
	}
	return new DumpObserver(layer, rast, file);
}


void dumpFeature(Vector* vector, long FID, const char* filename) {
	OGRLayer* layer = vector->getLayer(0);
	if ( !layer ) {
		cerr<< "warning: No layer 0 found\n";
		return;
	}
	OGRFeature* feature = layer->GetFeature(FID);
	if ( !feature ) {
		cerr<< " feature FID= " <<FID<< " not found\n";
		return;
	}
	FILE* file = fopen(filename, "w");
	if (!file) {
		cerr<< "cannot create " << filename<< endl;
		return;
	}
	
	dumpFeature(feature, file);
	fclose(file);
}

