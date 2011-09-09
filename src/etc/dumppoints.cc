//
// dumppoints
// Carlos A. Rueda
// $Id: dumppoints.cc,v 1.1 2005-10-21 23:00:54 crueda Exp $
// :tabSize=4:indentSize=4:noTabs=false:
// :folding=indent:collapseFolds=2:
//

#include "ogrsf_frmts.h"
#include "cpl_conv.h"
#include "cpl_string.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>


#define VERSION "0.1"


using namespace std;


static OGRDataSource* ds;
static OGRLayer* layer;

                             
	
static void _init(const char* pszDataSource) {
	OGRRegisterAll();
	
	ds = OGRSFDriverRegistrar::Open(pszDataSource);
    if( ds == NULL ) {
        fprintf(stderr, "Unable to open datasource `%s'\n", pszDataSource);
		exit(0);
	};

	layer = ds->GetLayer(0);
	if ( !layer ) {
		cerr<< "Couldn't get layer from " <<pszDataSource<< endl;
		exit(1);
	}
	layer->ResetReading();
	
	if ( layer->GetSpatialRef() == NULL ) {
		fprintf(stdout, "# WARNING: UNKNOWN SPATIAL REFERENCE IN VECTOR FILE!\n");
	}
}

static void _end() {
	delete ds;
	CPLPopErrorHandler();
}



static void process_feature(OGRFeature* feature) {
	OGRGeometry* feature_geometry = feature->GetGeometryRef();
	OGRwkbGeometryType geometry_type = feature_geometry->getGeometryType();

	switch ( geometry_type ) {
		case wkbPoint:
		case wkbPoint25D:
			/////////    OK    //////////////
			break;
	
		default:
			cerr<< OGRGeometryTypeToName(geometry_type)<< " : geometry type not considered"<< endl;
			exit(1);
	}
	
	
	OGRPoint* point = (OGRPoint*) feature_geometry;
	double x = point->getX();
	double y = point->getY();
	fprintf(stdout, "%d direct x=%g  y=%g  isnormal(x)=%d", feature->GetFID(), x, y, isnormal(x));
	long* ptr = (long*) &x;
	fprintf(stdout, " Hx=%x %x \n", ptr[0], ptr[1] );
	ptr = (long*) &y;
	fprintf(stdout, " Hy=%x %x \n", ptr[0], ptr[1] );

	
	//point->dumpReadable(stdout);
	//char        *pszWkt = NULL;
	//point->exportToWkt( &pszWkt );
	////fprintf(stdout, " wkt=%s\n", pszWkt);
	//sscanf(pszWkt, "POINT (%lf %lf", &x, &y);
	//CPLFree( pszWkt );
	//
	//fprintf(stdout, "%d viawkt x=%.5f  y=%.5f\n", feature->GetFID(), x, y);
}


///////////////////////////////////////////////////////////////
// main program
int main(int argc, char ** argv) {
	if ( argc == 1 ) {
		fprintf(stderr, "dumppoints vectorfile\n");
		return 1;
	}
	const char* vector_filename = argv[1];

	_init(vector_filename);		
	
	OGRFeature*	feature;
	//long psize = layer->GetFeatureCount();	
	while( (feature = layer->GetNextFeature()) != NULL ) {
		process_feature(feature);
		delete feature;
	}
	
	_end();
	
	
	return 0;
}

