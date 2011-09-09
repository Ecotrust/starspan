/*
	Vector - vector interface implementation
	$Id: Vector_ogr.cc,v 1.4 2008-04-10 20:26:37 crueda Exp $
	See Vector.h for public doc.
*/

#include "Vector.h"


int Vector::init() {
    OGRRegisterAll();
	return 0;
}

int Vector::end() {
    delete OGRSFDriverRegistrar::GetRegistrar();
    CPLFinderClean();
	return 0;
}



static void ReportOnLayer(int iLayer, FILE* file, OGRLayer* poLayer) {
    OGRFeatureDefn* poDefn = poLayer->GetLayerDefn();

    fprintf(file, "\nLayer %d: Name: %s\n", iLayer, poDefn->GetName() );
	fprintf(file, "Geometry: %s\n", OGRGeometryTypeToName(poDefn->GetGeomType()));
	fprintf(file, "Feature Count: %d\n", poLayer->GetFeatureCount());
	
	OGREnvelope oExt;
	if (poLayer->GetExtent(&oExt, TRUE) == OGRERR_NONE) {
		fprintf(file, "Extent: (%f, %f) - (%f, %f)\n", 
			   oExt.MinX, oExt.MinY, oExt.MaxX, oExt.MaxY);
	}

	char* pszWKT;
	if( poLayer->GetSpatialRef() == NULL )
		pszWKT = CPLStrdup( "(unknown)" );
	else {
		poLayer->GetSpatialRef()->exportToPrettyWkt( &pszWKT );
	}            

	fprintf(file, "Layer SRS WKT:\n%s\n", pszWKT);
	CPLFree(pszWKT);

	fprintf(file, "Field definitions:\n");
	for( int iAttr = 0; iAttr < poDefn->GetFieldCount(); iAttr++ ) {
		OGRFieldDefn* poField = poDefn->GetFieldDefn( iAttr );
		fprintf(file, "\t%s: %s (%d.%d)\n",
			poField->GetNameRef(),
			poField->GetFieldTypeName( poField->GetType() ),
			poField->GetWidth(),
			poField->GetPrecision() 
		);
	}

	/*
    OGRFeature* poFeature;
	int nFetchFID = OGRNullFID;
	int bSummaryOnly = FALSE;	
	
    if( nFetchFID == OGRNullFID && !bSummaryOnly ) {
        while( (poFeature = poLayer->GetNextFeature()) != NULL ) {
            poFeature->DumpReadable(file);
            delete poFeature;
        }
    }
    else if( nFetchFID != OGRNullFID ) {
        poFeature = poLayer->GetFeature( nFetchFID );
        if( poFeature == NULL ) {
            fprintf(file, "Unable to locate feature id %d on this layer.\n", nFetchFID);
        }
        else {
            poFeature->DumpReadable(file);
            delete poFeature;
        }
    }
	*/
}


Vector* Vector::open(const char* pszDataSource) {
	OGRDataSource* ds = OGRSFDriverRegistrar::Open(pszDataSource);
    if( ds == NULL ) {
        fprintf(stderr, "Vector::open: Unable to open datasource `%s'\n", pszDataSource);
		return 0;
	};
	
	return new Vector(ds);
}
	
Vector::Vector(OGRDataSource* ds) {
	poDS = ds;
}

Vector::~Vector() {
    delete poDS;
}

const char* Vector::getName() {
	return poDS->GetName();
}

OGRLayer* Vector::getLayer(int layer_num) {
	OGRLayer* layer = poDS->GetLayer(layer_num);
	if( layer == NULL ) {
		fprintf(stderr, "Vector::getLayer: Couldn't fetch layer %d\n", layer_num);
		return NULL;
	}
	return layer;
}

void Vector::report(FILE* file) {
	fprintf(file, "%s\n", poDS->GetName());
	fprintf(file, "Layers = %d\n", poDS->GetLayerCount());
	for( int iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++ ) {
		OGRLayer* poLayer = poDS->GetLayer(iLayer);
		if( poLayer == NULL ) {
			fprintf(file, "FAILURE: Couldn't fetch advertised layer %d!\n", iLayer);
			exit(1);
		}
		ReportOnLayer(iLayer+1, file, poLayer);
	}
}

void Vector::showFields(FILE* file) {
	for( int iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++ ) {
		OGRLayer* poLayer = poDS->GetLayer(iLayer);
		if( poLayer == NULL ) {
			fprintf(file, "FAILURE: Couldn't fetch advertised layer %d!\n", iLayer);
			exit(1);
		}
		OGRFeatureDefn* poDefn = poLayer->GetLayerDefn();
		for( int iAttr = 0; iAttr < poDefn->GetFieldCount(); iAttr++ ) {
			OGRFieldDefn* poField = poDefn->GetFieldDefn( iAttr );
			fprintf(file, "%s\n",
				poField->GetNameRef()
			);
		}
	}
}


Vector* Vector::create(const char* pszDataSource) {
    const char *pszDriverName = "ESRI Shapefile";
    OGRSFDriver *poDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(
                pszDriverName 
    );
    if ( poDriver == NULL ) {
        fprintf(stderr, "%s: driver not available.\n", pszDriverName);
        return NULL;
    }

    OGRDataSource *poDS = poDriver->CreateDataSource(pszDataSource, NULL);
    if ( poDS == NULL ) {
        fprintf(stderr, "Creation of output file failed.\n");
        return NULL;
    }
	
	return new Vector(poDS);
}
	

