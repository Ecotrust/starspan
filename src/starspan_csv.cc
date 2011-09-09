//
// STARSpan project
// Carlos A. Rueda
// starspan_csv - generate a CSV file from multiple rasters
// $Id: starspan_csv.cc,v 1.13 2008-05-09 02:11:17 crueda Exp $
//

#include "starspan.h"
#include "traverser.h"
#include "Csv.h"

#include <stdlib.h>
#include <assert.h>


/**
  * This observer extracts from ONE raster
  */
class CSVObserver : public Observer {
public:
	GlobalInfo* global_info;
	Vector* vect;
	OGRLayer* poLayer;
	OGRFeature* currentFeature;
	vector<const char*>* select_fields;
	const char* raster_filename;
	string RID_value;  //  will be used only if globalOptions.RID != "none".
	bool write_header;
	FILE* file;
	int layernum;
	CsvOutput csvOut;
	
	/**
	  * Creates a csv creator
	  */
	CSVObserver(Vector* vect, vector<const char*>* select_fields, FILE* f, int layernum)
	: vect(vect), select_fields(select_fields), file(f), layernum(layernum)
	{
		global_info = 0;
		poLayer = vect->getLayer(layernum);
		if ( !poLayer ) {
			fprintf(stderr, "Couldn't fetch layer %d\n", layernum);
			exit(1);
		}
	}
	
	/**
	  * If write_header is true, it creates first line with 
	  * column headers:
	  *    FID, fields-from-feature, [col,row,] [x,y,] [RID,] bands-from-raster
	  */
	void init(GlobalInfo& info) {
		global_info = &info;
		
		csvOut.setFile(file);
		csvOut.setSeparator(globalOptions.delimiter);
		csvOut.startLine();

		if ( write_header ) {
			//		
			// write column headers:
			//
	
			// Create FID field
			csvOut.addString("FID");
			
			// Create fields:
			if ( select_fields ) {
				for ( vector<const char*>::const_iterator fname = select_fields->begin(); fname != select_fields->end(); fname++ ) {
					csvOut.addString(*fname);
				}
			}
			else {
				// all fields from layer definition
				OGRFeatureDefn* poDefn = poLayer->GetLayerDefn();
				int feature_field_count = poDefn->GetFieldCount();
				
				for ( int i = 0; i < feature_field_count; i++ ) {
					OGRFieldDefn* poField = poDefn->GetFieldDefn(i);
					const char* pfield_name = poField->GetNameRef();
					csvOut.addString(pfield_name);
				}
			}
			
			// RID column, if to be included
			if ( globalOptions.RID != "none" ) {
				csvOut.addString(RID_colName);
			}
			
			// Create (col,row) fields, if so indicated
			if ( !globalOptions.noColRow ) {
				csvOut.addString("col").addString("row");
			}
			
			// Create (x,y) fields, if so indicated
			if ( !globalOptions.noXY ) {
				csvOut.addString("x").addString("y");
			}
			
			// Create fields for bands
			for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
				csvOut.addField("Band%d", i+1);
			}
			
			csvOut.endLine();
		}
		
		currentFeature = NULL;
		if ( globalOptions.RID != "none" ) {
			RID_value = raster_filename;
			if ( globalOptions.RID == "file" ) {
				starspan_simplify_filename(RID_value);
			}
		}
	}
	

	/**
	  * Used here to update currentFeature
	  */
	void intersectionFound(IntersectionInfo& intersInfo) {
		currentFeature = intersInfo.feature;
	}
	
	
	/**
	  * Adds a record to the output file.
	  */
	void addPixel(TraversalEvent& ev) { 
		int col = 1 + ev.pixel.col;
		int row = 1 + ev.pixel.row;
		void* band_values = ev.bandValues;
		
		//
		// Add field values to new record:
		//
		
		csvOut.startLine();

		// Add FID value:
		csvOut.addField("%ld", currentFeature->GetFID());
		
		// add attribute fields from source currentFeature to record:
		if ( select_fields ) {
			for ( vector<const char*>::const_iterator fname = select_fields->begin(); fname != select_fields->end(); fname++ ) {
				const int i = currentFeature->GetFieldIndex(*fname);
				if ( i < 0 ) {
					fprintf(stderr, "\n\tField `%s' not found\n", *fname);
					exit(1);
				}
				const char* str = currentFeature->GetFieldAsString(i);
				csvOut.addString(str);
			}
		}
		else {
			// all fields
			int feature_field_count = currentFeature->GetFieldCount();
			for ( int i = 0; i < feature_field_count; i++ ) {
				const char* str = currentFeature->GetFieldAsString(i);
				csvOut.addString(str);
			}
		}

		// add RID field
		if ( globalOptions.RID != "none" ) {
			csvOut.addString(RID_value);
		}
		
		
		// add (col,row) fields
		if ( !globalOptions.noColRow ) {
			csvOut.addField("%d", col).addField("%d", row);
		}
		
		// add (x,y) fields
		if ( !globalOptions.noXY ) {
			csvOut.addField("%.3f", ev.pixel.x).addField("%.3f", ev.pixel.y);
		}
		
		// add band values to record:
		char* ptr = (char*) band_values;
		char value[1024];
		for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
			GDALDataType bandType = global_info->bands[i]->GetRasterDataType();
			int typeSize = GDALGetDataTypeSize(bandType) >> 3;
			
			starspan_extract_string_value(bandType, ptr, value);
			csvOut.addString(value);
			
			// move to next piece of data in buffer:
			ptr += typeSize;
		}
		csvOut.endLine();
	}

	/**
	  * does nothing
	  */
	void end() {}
};



////////////////////////////////////////////////////////////////////////////////

//
// Each raster is processed independently
//
int starspan_csv(
	Vector* vect,
	vector<const char*> raster_filenames,
	vector<const char*>* select_fields,
	const char* csv_filename,
	int layernum
) {
	FILE* file;
	bool new_file = false;
	
	// if file exists, append new rows. Otherwise create file.
	file = fopen(csv_filename, "r+");
	if ( file ) {
		if ( globalOptions.verbose ) {
			fprintf(stdout, "starspan_csv: Appending to existing file %s\n", csv_filename);
        }

		fseek(file, 0, SEEK_END);

		// check that new data will start in a new line:
		// if last character is not '\n', then write a '\n':
		// (This check will make the output more robust in case
		// the previous information is not properly aligned, eg.
		// when the previous generation was killed for some reason.)
		long endpos = ftell(file);
		if ( endpos > 0 ) {
			fseek(file, endpos -1, SEEK_SET);
			char c;
			if ( 1 == fread(&c, sizeof(c), 1, file) ) {
				if ( c != '\n' )
					fputc('\n', file);    // add a new line
			}
		}
	}
	else {
		// create output file
		file = fopen(csv_filename, "w");
		if ( !file) {
			fprintf(stderr, "Cannot create %s\n", csv_filename);
			return 1;
		}
		new_file = true;
	}

	CSVObserver obs(vect, select_fields, file, layernum);

	Traverser tr;
	tr.addObserver(&obs);

	tr.setVector(vect);
	tr.setLayerNum(layernum);
    
	if ( globalOptions.progress ) {
		tr.setProgress(globalOptions.progress_perc, cout);
		cout << "Number of features: ";
		long psize = vect->getLayer(layernum)->GetFeatureCount();
		if ( psize >= 0 )
			cout << psize;
		else
			cout << "(not known in advance)";
		cout<< endl;
	}
	
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		fprintf(stdout, "starspan_csv: %3u: Extracting from %s\n", i+1, raster_filenames[i]);
		obs.raster_filename = raster_filenames[i];
		obs.write_header = new_file && i == 0;
		tr.removeRasters();

		Raster* raster = new Raster(raster_filenames[i]);
		tr.addRaster(raster);
		
		tr.traverse();

		if ( globalOptions.report_summary ) {
			tr.reportSummary();
		}

		delete raster;
	}
	
	fclose(file);
	
	return 0;
}

