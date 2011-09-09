//
// STARSpan project
// Carlos A. Rueda
// starspan_stats - some stats calculation
// $Id: starspan_stats.cc,v 1.11 2008-05-09 02:11:17 crueda Exp $
//

#include "starspan.h"           
#include "traverser.h"       
#include "Stats.h"       
#include "Csv.h"

#include <iostream>
#include <cstdlib>
#include <math.h>
#include <cassert>

using namespace std;


/**
  * Creates fields and populates the table.
  */
class StatsObserver : public Observer {
public:
	Traverser& tr;
	GlobalInfo* global_info;
	Vector* vect;
	FILE* file;
	vector<const char*> select_stats;
	vector<const char*>* select_fields;
	const char* raster_filename;
	string RID;  //  will be used only if globalOptions.RID != "none".
	
	// if all bands are of integral type, then tr.getPixelIntegerValuesInBand
	// is used; else tr.getPixelDoubleValuesInBand is used.
	bool get_integer;
	
	Stats stats;
	double* result_stats[TOT_RESULTS];
	
	bool write_header;
	bool closeFile;
	bool releaseStats;

	// NULL if no pending feature to be processed
	OGRFeature* last_feature;	
		
	// last processed FID (this value "survives" last_feature)
	long last_FID;
	
	CsvOutput csvOut;


	/**
	  * Creates a stats calculator
	  */
	StatsObserver(Traverser& tr, FILE* f, vector<const char*> select_stats,
		vector<const char*>* select_fields
	) : tr(tr), file(f), select_stats(select_stats), select_fields(select_fields)
	{
		vect = tr.getVector();
		global_info = 0;
		for ( unsigned i = 0; i < TOT_RESULTS; i++ ) {
			result_stats[i] = 0;
			stats.include[i] = false;
		}
		
		// by default, write the header in init()
		write_header = true;
		
		// by default, close the file in end()
		closeFile = true;

		// by default, result_stats arrays (to be allocated in init())
		// get released in end():
		releaseStats = true;

		last_FID = -1;
		last_feature = 0;
		
		for ( vector<const char*>::const_iterator stat = select_stats.begin(); stat != select_stats.end(); stat++ ) {
			if ( 0 == strcmp(*stat, "avg") )
				stats.include[AVG] = true;
			else if ( 0 == strcmp(*stat, "mode") )
				stats.include[MODE] = true;
			else if ( 0 == strcmp(*stat, "stdev") )
				stats.include[STDEV] = true;
			else if ( 0 == strcmp(*stat, "min") )
				stats.include[MIN] = true;
			else if ( 0 == strcmp(*stat, "max") )
				stats.include[MAX] = true;
			else if ( 0 == strcmp(*stat, "sum") )
				stats.include[SUM] = true;
			else if ( 0 == strcmp(*stat, "median") )
				stats.include[MEDIAN] = true;
			else if ( 0 == strcmp(*stat, "nulls") )
				stats.include[NULLS] = true;
			else {
				cerr<< "Unrecognized stats " << *stat<< endl;
				exit(1);
			}
		}
	}
	
	/**
	  * simply calls end()
	  */
	~StatsObserver() {
		end();
	}
	
	/**
	  * finalizes current feature if any; 
	  * closes the file if closeFile is true;
	  * releases the result_stats arrays if releaseStats is true
	  */
	void end() {
		finalizePreviousFeatureIfAny();
		if ( closeFile && file ) {
			fclose(file);
			cout<< "Stats: finished" << endl;
			file = 0;
		}
		
		if ( releaseStats ) {
			for ( unsigned i = 0; i < TOT_RESULTS; i++ ) {
				if ( result_stats[i] ) {
					delete[] result_stats[i];
					result_stats[i] = 0;
				}
			}
		}
	}


	/**
	  * returns true. We ask traverser to get values for
	  * visited pixels in each feature.
	  */
	bool isSimple() { 
		return true; 
	}

	/**
	  * If write_header is true, it creates first line with 
	  * column headers:
	  *    FID, {vect-attrs}, RID, numPixels, S1_Band1, S1_Band2 ..., S2_Band1, S2_Band2 ...
	  * where S# is each desired statistics
	  */
	void init(GlobalInfo& info) {
		global_info = &info;

		int layernum = tr.getLayerNum();
		OGRLayer* poLayer = vect->getLayer(layernum);
		if ( !poLayer ) {
			cerr<< "Couldn't fetch layer " <<layernum<<  endl;
			exit(1);
		}

		csvOut.setFile(file);
		csvOut.setSeparator(globalOptions.delimiter);
		csvOut.startLine();
		
		//		
		// write column headers:
		//
		if ( write_header && file ) {
			// Create FID field
			csvOut.addString("FID");
			//fprintf(file, "FID");
	
			// Create fields:
			if ( select_fields ) {
				for ( vector<const char*>::const_iterator fname = select_fields->begin(); fname != select_fields->end(); fname++ ) {
					csvOut.addString(*fname);
					//fprintf(file, ",%s", *fname);
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
					//fprintf(file, ",%s", pfield_name);
				}
			}
			
			
			// RID column, if to be included
			if ( globalOptions.RID != "none" ) {
				csvOut.addString(RID_colName);
				//fprintf(file, ",RID");
			}
			
			// Create numPixels field
			csvOut.addString("numPixels");
			//fprintf(file, ",numPixels");
			
			// Create fields for bands
			for ( vector<const char*>::const_iterator stat = select_stats.begin(); stat != select_stats.end(); stat++ ) {
				for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
					csvOut.addField("%s_Band%d", *stat, i+1);
					//fprintf(file, ",%s_Band%d", *stat, i+1);
				}
			}	
			csvOut.endLine();
			//fprintf(file, "\n");
		}
		
		// allocate space for all possible results
		for ( unsigned i = 0; i < TOT_RESULTS; i++ ) {
			result_stats[i] = new double[global_info->bands.size()];
		}
		
		// assume integer bands:
		get_integer = true;
		for ( unsigned i = 0; i < global_info->bands.size(); i++ ) {
			GDALDataType bandType = global_info->bands[i]->GetRasterDataType();
			if ( bandType == GDT_Float64 || bandType == GDT_Float32 ) {
				get_integer = false;
				break;
			}
		}		
		
		// prepare RID
		if ( globalOptions.RID != "none" ) {
			RID = raster_filename;
			if ( globalOptions.RID == "file" ) {
				starspan_simplify_filename(RID);
			}
		}
	}
	

	/**
	  * compute all results from current list of pixels.
	  * Desired results are reported by finalizePreviousFeatureIfAny.
	  */
	void computeResults(void) {
		if ( get_integer ) {
			vector<int> values;
			for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
				values.clear();
				tr.getPixelIntegerValuesInBand(j+1, values);
				if (!globalOptions.nodata) {
					double nodata = global_info->bands[j]->GetNoDataValue();
					globalOptions.nodata = nodata;
				} 
				stats.compute(values, int(globalOptions.nodata));
				for ( int i = 0; i < TOT_RESULTS; i++ ) {
					result_stats[i][j] = stats.result[i]; 
				}
			}
		}
		else {
			vector<double> values;
			for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
				values.clear();
				tr.getPixelDoubleValuesInBand(j+1, values);
				if (!globalOptions.nodata) {
					double nodata = global_info->bands[j]->GetNoDataValue();
					globalOptions.nodata = nodata;
				} 
				stats.compute(values, globalOptions.nodata);
				for ( int i = 0; i < TOT_RESULTS; i++ ) {
					result_stats[i][j] = stats.result[i]; 
				}
			}
		}
	}

	/**
	  * dispatches finalization of previous feature
	  */
	void finalizePreviousFeatureIfAny(void) {
		if ( !last_feature )
			return;
		
		
		if ( 0 == tr.getPixelSetSize() ) {
			if ( globalOptions.verbose ) {
				cout<< "No intersecting pixels actually found for previous FID: " <<last_feature->GetFID()<< endl;
			}
			delete last_feature;
			last_feature = 0;
			return;
		}	
		
		computeResults();
 


		if ( file ) {
			// Add FID value:
			csvOut.addField("%ld", last_feature->GetFID());
			//fprintf(file, "%ld", last_feature->GetFID());
	
			// add attribute fields from source feature to record:
			if ( select_fields ) {
				for ( vector<const char*>::const_iterator fname = select_fields->begin(); fname != select_fields->end(); fname++ ) {
					const int i = last_feature->GetFieldIndex(*fname);
					if ( i < 0 ) {
						cerr<< endl << "\tField `" <<*fname<< "' not found" << endl;
						exit(1);
					}
					const char* str = last_feature->GetFieldAsString(i);
					csvOut.addString(str);
					//fprintf(file, ",%s", str);
				}
			}
			else {
				// all fields
				int last_feature_field_count = last_feature->GetFieldCount();
				for ( int i = 0; i < last_feature_field_count; i++ ) {
					const char* str = last_feature->GetFieldAsString(i);
					csvOut.addString(str);
					//fprintf(file, ",%s", str);
				}
			}
			
			// add RID field
			if ( globalOptions.RID != "none" ) {
				csvOut.addString(RID);
				//fprintf(file, ",%s", RID.c_str());
			}
			
			
			// Add numPixels value:
			csvOut.addField("%d", tr.getPixelSetSize());
			//fprintf(file, ",%d", tr.getPixelSetSize());
			
			// report desired results:
			// (desired list is traversed to keep order according to column headers)
			for ( vector<const char*>::const_iterator stat = select_stats.begin(); stat != select_stats.end(); stat++ ) {
				if ( 0 == strcmp(*stat, "avg") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						csvOut.addField("%f", result_stats[AVG][j]);
						//fprintf(file, ",%f", result_stats[AVG][j]);
					}
				}
				else if ( 0 == strcmp(*stat, "mode") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						csvOut.addField("%f", result_stats[MODE][j]);
						//fprintf(file, ",%f", result_stats[MODE][j]);
					}
				}
				else if ( 0 == strcmp(*stat, "stdev") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						csvOut.addField("%f", result_stats[STDEV][j]);
						//fprintf(file, ",%f", result_stats[STDEV][j]);
					}
				}
				else if ( 0 == strcmp(*stat, "min") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						csvOut.addField("%f", result_stats[MIN][j]);
						//fprintf(file, ",%f", result_stats[MIN][j]);
					}
				}
				else if ( 0 == strcmp(*stat, "max") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						csvOut.addField("%f", result_stats[MAX][j]);
						//fprintf(file, ",%f", result_stats[MAX][j]);
					}
				}
				else if ( 0 == strcmp(*stat, "sum") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						csvOut.addField("%f", result_stats[SUM][j]);
						//fprintf(file, ",%f", result_stats[SUM][j]);
					}
				}
				else if ( 0 == strcmp(*stat, "median") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						csvOut.addField("%f", result_stats[MEDIAN][j]);
						//fprintf(file, ",%f", result_stats[MEDIAN][j]);
					}
				}
				else if ( 0 == strcmp(*stat, "nulls") ) {
					for ( unsigned j = 0; j < global_info->bands.size(); j++ ) {
						csvOut.addField("%d", int(result_stats[NULLS][j]) );
						//fprintf(file, ",%d", int(result_stats[NULLS][j]) );
					}
				}
				else {
					cerr<< "Unrecognized stats "<< *stat << endl;
					exit(1);
				}
			}
			csvOut.endLine();
			//fprintf(file, "\n");
		}
		delete last_feature;
		last_feature = 0;
	}

	/**
	  * dispatches pending feature if any, and prepares for the next
	  */
	void intersectionFound(IntersectionInfo& intersInfo) {
		finalizePreviousFeatureIfAny();
		
		// keep track of last FID processed:
		last_FID = intersInfo.feature->GetFID();
		
		last_feature = intersInfo.feature->Clone();
	}
	
	
	/**
	  * Nothing needs to be done here. Traverser keeps track of
	  * pixels in current feature.
	  */
	void addPixel(TraversalEvent& ev) {
	}

};



/**
  * starspan_getStatsObserver: implementation
  */
Observer* starspan_getStatsObserver(
	Traverser& tr,
	vector<const char*> select_stats,
	vector<const char*>* select_fields,
	const char* filename
) {
	// create output file
	FILE* file = fopen(filename, "w");
	if ( !file ) {
		cerr<< "Couldn't create "<< filename << endl;
		return 0;
	}

	return new StatsObserver(tr, file, select_stats, select_fields);	
}
		

/**
  * starspan_getFeatureStats: implementation
  */
double** starspan_getFeatureStats(
	long FID, Vector* vect, Raster* rast,
	vector<const char*> select_stats
) {
	Traverser tr;
	tr.setVector(vect);
	tr.addRaster(rast);
	tr.setDesiredFID(FID);

    double** result_stats = 0;
    
	FILE* file = 0;
	vector<const char*> select_fields;
	StatsObserver* statsObs = new StatsObserver(tr, file, select_stats, &select_fields);
	if ( statsObs ) {
        // I want to keep the resulting stats arrays for the client
        statsObs->releaseStats = false;
        
        tr.addObserver(statsObs);
        tr.traverse();
        
        // take results:
        double** result_stats = statsObs->result_stats;
        
        tr.releaseObservers();
	}
    
	return result_stats;
}


double** starspan_getFeatureStatsByField(
	const char* field_name, 
	const char* field_value, 
	Vector* vect, Raster* rast,
	vector<const char*> select_stats,
	long *FID
) {
	Traverser tr;
    
	tr.setVector(vect);
	tr.addRaster(rast);
	tr.setDesiredFeatureByField(field_name, field_value);

	FILE* file = 0;
	vector<const char*> select_fields; // empty means don't select any fields.
	StatsObserver* statsObs = new StatsObserver(tr, file, select_stats, &select_fields);
	if ( !statsObs )
		return 0;
	
	// I want to keep the resulting stats arrays for the client
	statsObs->releaseStats = false;
	
	tr.addObserver(statsObs);
	tr.traverse();
	
	// take results:
	double** result_stats = statsObs->result_stats;
	if ( FID )
		*FID = statsObs->last_FID;
	
	tr.releaseObservers();
	
	return result_stats;
}


////////////////////////////////////////////////////////////////////////////////

//
// Each raster is processed independently
//
int starspan_stats(
	Vector* vect,
	vector<const char*> raster_filenames,
	vector<const char*> select_stats,
	vector<const char*>* select_fields,
	const char* csv_filename,
	int layernum
) {
	FILE* file;
	bool new_file = false;
	
	// if file exists, append new rows. Otherwise create file.
	file = fopen(csv_filename, "r+");
	if ( file ) {
		if ( globalOptions.verbose )
			fprintf(stdout, "Appending to existing file %s\n", csv_filename);

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

	Traverser tr;
	tr.setVector(vect);
	tr.setLayerNum(layernum);

	if ( globalOptions.FID >= 0 )
		tr.setDesiredFID(globalOptions.FID);
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

	StatsObserver obs(tr, file, select_stats, select_fields);
	obs.closeFile = false;
	tr.addObserver(&obs);


	Raster* rasters[raster_filenames.size()];
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		rasters[i] = new Raster(raster_filenames[i]);
	}
	
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		fprintf(stdout, "%3u: Extracting from %s\n", i+1, raster_filenames[i]);
		obs.raster_filename = raster_filenames[i];
		obs.write_header = new_file && i == 0;
		tr.removeRasters();
		tr.addRaster(rasters[i]);
		
		tr.traverse();

		if ( globalOptions.report_summary ) {
			tr.reportSummary();
		}
	}
	
	fclose(file);
	
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		delete rasters[i];
	}
	
	return 0;
}

