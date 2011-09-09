//
// STARSpan project
// Carlos A. Rueda
// starspan_csv_raster_field - generate a CSV file from rasters given in a
// vector field
// $Id: starspan_csv_raster_field.cc,v 1.3 2006-11-10 22:53:08 perrygeo Exp $
//

#include "starspan.h"           
#include "traverser.h"       

#include <stdlib.h>
#include <assert.h>

static const char* raster_field_name;
static const char* raster_directory;
static string raster_filename;
static vector<const char*>* select_fields;
static const char* output_filename;
static FILE* output_file;
static bool new_file = false;
static bool write_column_headers = false;
static OGRFeature* currentFeature;
static Raster* raster;
static bool RID_already_included = false;
static OGRLayer* layer;
	



static void open_output_file() {
	// if file exists, append new rows. Otherwise create file.
	output_file = fopen(output_filename, "r+");
	if ( output_file ) {
		fseek(output_file, 0, SEEK_END);
		if ( globalOptions.verbose )
			fprintf(stdout, "Appending to existing file %s\n", output_filename);
	}
	else {
		// create output file
		output_file = fopen(output_filename, "w");
		if ( !output_file) {
			fprintf(stderr, "Cannot create %s\n", output_filename);
			exit(1);
		}
		new_file = true;
	}
}

static void write_column_headers_if_necessary() {
	if ( write_column_headers ) {
		write_column_headers = false;

		// Create FID field
		fprintf(output_file, "FID");
		
		// Create fields:
		if ( select_fields ) {
			for ( vector<const char*>::const_iterator fname = select_fields->begin(); fname != select_fields->end(); fname++ ) {
				fprintf(output_file, ",%s", *fname);
				if ( 0 == strcmp(raster_field_name, *fname) ) {
					RID_already_included = true;
				}
			}
		}
		else {
			// all fields from layer definition
			OGRFeatureDefn* poDefn = layer->GetLayerDefn();
			int feature_field_count = poDefn->GetFieldCount();
			
			for ( int i = 0; i < feature_field_count; i++ ) {
				OGRFieldDefn* poField = poDefn->GetFieldDefn(i);
				const char* pfield_name = poField->GetNameRef();
				fprintf(output_file, ",%s", pfield_name);
			}
			RID_already_included = true;
		}
		
		if ( ! RID_already_included ) {
			// add RID field
			fprintf(output_file, ",%s", raster_field_name);
		}
		
		// Create (col,row) fields, if so indicated
		if ( !globalOptions.noColRow ) {
			fprintf(output_file, ",col");
			fprintf(output_file, ",row");
		}
		
		// Create (x,y) fields, if so indicated
		if ( !globalOptions.noXY ) {
			fprintf(output_file, ",x");
			fprintf(output_file, ",y");
		}
		
		// Create fields for bands
		GDALDataset* dataset = raster->getDataset();
		for ( int i = 0; i < dataset->GetRasterCount(); i++ ) {
			fprintf(output_file, ",Band%d", i+1);
		}
		
		fprintf(output_file, "\n");
		
	}
}

static void processPoint(OGRPoint* point) {
	double x = point->getX();
	double y = point->getY();
	int col, row;
	raster->toColRow(x, y, &col, &row);
	
	void* band_values = raster->getBandValuesForPixel(col, row);
	if ( !band_values ) {
		// means NO intersection.
		return;
	}
	
	
	write_column_headers_if_necessary();
	
	//
	// Add field values to new record:
	//

	// Add FID value:
	fprintf(output_file, "%ld", currentFeature->GetFID());
	
	// add attribute fields from source currentFeature to record:
	if ( select_fields ) {
		for ( vector<const char*>::const_iterator fname = select_fields->begin(); fname != select_fields->end(); fname++ ) {
			const int i = currentFeature->GetFieldIndex(*fname);
			if ( i < 0 ) {
				fprintf(stderr, "\n\tField `%s' not found\n", *fname);
				exit(1);
			}
			
			const char* str = currentFeature->GetFieldAsString(i);
			if ( strchr(str, ',') )
				fprintf(output_file, ",\"%s\"", str);   // quote string
			else
				fprintf(output_file, ",%s", str);
		}
	}
	else {
		// all fields
		int feature_field_count = currentFeature->GetFieldCount();
		for ( int i = 0; i < feature_field_count; i++ ) {
			const char* str = currentFeature->GetFieldAsString(i);
			if ( strchr(str, ',') )
				fprintf(output_file, ",\"%s\"", str);   // quote string
			else
				fprintf(output_file, ",%s", str);
		}
	}

	if ( ! RID_already_included ) {
		// add RID field
		fprintf(output_file, ",%s", raster_filename.c_str());
	}
	
	
	// add (col,row) fields
	if ( !globalOptions.noColRow ) {
		fprintf(output_file, ",%d,%d", col, row);
	}
	
	// add (x,y) fields
	if ( !globalOptions.noXY ) {
		fprintf(output_file, ",%.3f,%.3f", x, y);
	}
	
	// add band values to record:
	char* ptr = (char*) band_values;
	char value[1024];
	GDALDataset* dataset = raster->getDataset();
	for ( int i = 0; i < dataset->GetRasterCount(); i++ ) {
		GDALRasterBand* band = dataset->GetRasterBand(i+1);
		GDALDataType bandType = band->GetRasterDataType();
		int typeSize = GDALGetDataTypeSize(bandType) >> 3;
		starspan_extract_string_value(bandType, ptr, value);
		fprintf(output_file, ",%s", value);
		
		// move to next piece of data in buffer:
		ptr += typeSize;
	}
	fprintf(output_file, "\n");
	
}


//
// only Point type is processed now
//
static void processGeometry(OGRGeometry* geometry) {
	OGRwkbGeometryType type = geometry->getGeometryType();
	switch ( type ) {
		case wkbPoint:
		case wkbPoint25D:
			processPoint((OGRPoint*) geometry);
			break;
			
		default:
			cerr<< (string(OGRGeometryTypeToName(type))
			    + ": intersection type not considered."
			);
	}
}



static void extract_pixels() {
	OGRGeometry* feature_geometry = currentFeature->GetGeometryRef();
	processGeometry(feature_geometry);
}



static void process_feature() {
	if ( globalOptions.verbose ) {
		fprintf(stdout, "\nFID: %ld", currentFeature->GetFID());
	}
	
	const int i = currentFeature->GetFieldIndex(raster_field_name);
	if ( i < 0 ) {
		fprintf(stderr, "\n\tField `%s' not found\n", raster_field_name);
		exit(1);
	}
	raster_filename = raster_directory;
	raster_filename += "/";
	raster_filename += currentFeature->GetFieldAsString(i);
	
	raster = new Raster(raster_filename.c_str());
	
	extract_pixels();
	
	delete raster;
}





////////////////////////////////////////////////////////////////////////////////

int starspan_csv_raster_field(
	Vector*              vect,
	const char*          _raster_field_name,
	const char*          _raster_directory,
	vector<const char*>* _select_fields,
	const char*          _output_filename,
	int                  layernum
) {
	raster_field_name = _raster_field_name;
	raster_directory =  _raster_directory;
	select_fields =     _select_fields;
	output_filename =   _output_filename;


	//if ( vect->getLayerCount() > 1 ) {
	//	cerr<< "Vector datasource with more than one layer: "
	//	    << vect->getName()
	//		<< "\nOnly one layer expected.\n"
	//	;
	//	return 1;
	//}
	
	layer = vect->getLayer(layernum);
	if ( !layer ) {
		cerr<< "Couldn't get layer from " << vect->getName()<< endl;
		return 1;
	}
	layer->ResetReading();

	open_output_file();
	
	write_column_headers = new_file;
	
	//
	// Was a specific FID given?
	//
	if ( globalOptions.FID >= 0 ) {
		currentFeature = layer->GetFeature(globalOptions.FID);
		if ( !currentFeature ) {
			cerr<< "FID " <<globalOptions.FID<< " not found in " <<vect->getName()<< endl;
			exit(1);
		}
		process_feature();
		delete currentFeature;
	}
	//
	// else: process each feature in vector datasource:
	//
	else {
		Progress* progress = 0;
		if ( globalOptions.progress ) {
			long psize = layer->GetFeatureCount();
			if ( psize >= 0 ) {
				progress = new Progress(psize, globalOptions.progress_perc);
			}
			else {
				progress = new Progress((long)globalOptions.progress_perc);
			}
			cout << "\t";
			progress->start();
		}
		while( (currentFeature = layer->GetNextFeature()) != NULL ) {
			process_feature();
			delete currentFeature;
			if ( progress )
				progress->update();
		}
		if ( progress ) {
			progress->complete();
			delete progress;
			progress = 0;
			cout << endl;
		}
	}	
	
	return 0;
}

