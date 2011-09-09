//
// STARSpan project
// Carlos A. Rueda
// starspan_update_csv - update a csv 
// $Id: starspan_update_csv.cc,v 1.4 2007-12-15 05:49:58 crueda Exp $
//

#include "starspan.h"           
#include "Csv.h"       
#include <fstream>       

#include <cstdlib>
#include <cassert>


/**
  * implementation
  */
int starspan_update_csv(
	const char* in_csv_filename,
	vector<const char*> raster_filenames,
	const char* out_csv_filename
) {
	// the delimiter is assumed on the input file
	const string delimiter = globalOptions.delimiter;
	
	// open input file
	ifstream in_file(in_csv_filename);
	if ( !in_file ) {
		cerr<< "Couldn't open " <<in_csv_filename<< endl;
		return 1;
	}
	Csv csv(in_file, delimiter);
	string line;
	if ( !csv.getline(line) ) {
		in_file.close();
		cerr<< "Couldn't get header line from " <<in_csv_filename<< endl;
		return 1;
	}
	
	// first try is to use x,y fields
	bool use_xy = true;

	// check fields x,y in csv file:
	int x_field_index = -1;
	int y_field_index = -1;
	int col_field_index = -1;
	int row_field_index = -1;
	const unsigned num_existing_fields = csv.getnfield();
	for ( unsigned i = 0; i < num_existing_fields; i++ ) {
		string field = csv.getfield(i);
		if ( field == "x" )
			x_field_index = i;
		else if ( field == "y" )
			y_field_index = i;
		else if ( field == "col" )
			col_field_index = i;
		else if ( field == "row" )
			row_field_index = i;
	}
	
	if ( x_field_index < 0 || y_field_index < 0 ) {
		cout<< "Warning: No fields 'x' and/or 'y' are present in " <<in_csv_filename<<endl;
		cout<< "         Will try with col,row fields ...\n";
		use_xy = false;
		if ( col_field_index < 0 || row_field_index < 0 ) {
			in_file.close();
			cout<< "No fields 'col' and/or 'row' are present in " <<in_csv_filename<<endl;
			return 1;
		}
	}

	// create output file
	ofstream out_file(out_csv_filename, ios::out);
	if ( !out_file ) {
		in_file.close();
		cerr<< "Couldn't create " <<out_csv_filename<< endl;
		return 1;
	}

	//
	// copy existing field definitions
	//
	for ( unsigned i = 0; i < num_existing_fields; i++ ) {
		string field = csv.getfield(i);
		cout << "Creating field: " << field << endl;
		if ( i > 0 ) {
			out_file << delimiter;
		}
		out_file << field;
	}


	// create new band field definitions in csv:	
	// open raster datasets:
	vector<Raster*> rasts;
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
		rasts.push_back(new Raster(raster_filenames[i]));
	}
	int next_field_index = num_existing_fields;
	for ( unsigned r = 0; r < raster_filenames.size(); r++ ) {
		const char* rast_name = raster_filenames[r];
		Raster* rast = rasts[r];
		int bands;
		rast->getSize(NULL, NULL, &bands);
		for ( int b = 0; b < bands; b++ ) {
			char field_name[1024];
			sprintf(field_name, "Band_%d_%s", b+1, rast_name);
			cout<< "Creating field: " <<field_name<<endl;
			out_file << delimiter << field_name;
			next_field_index++;
		}
	}
	// end column headers
	out_file << endl;
	
	
	//
	// main body of processing
	//
	
	// for each point point in input csv...
	cout<< "processing records...\n";
	for ( int record = 0; csv.getline(line); record++ ) {
		cout<< "record " <<record<< "  ";
		//
		// copy existing field values
		//
		for ( unsigned i = 0; i < num_existing_fields; i++ ) {
			string field = csv.getfield(i);
			if ( i > 0 ) {
				out_file << delimiter;
			}
			out_file << field;
		}
		
		
		//
		// now extract desired pixel from given rasters
		//
		
		double x = 0, y = 0;
		int col = 0, row = 0;
		
		if ( use_xy ) {
			x = atof(csv.getfield(x_field_index).c_str());
			y = atof(csv.getfield(y_field_index).c_str());
			cout<< "x , y = " <<x<< " , " <<y<< endl;
		}
		else {
			col = atoi(csv.getfield(col_field_index).c_str());
			row = atoi(csv.getfield(row_field_index).c_str());
			cout<< "col , row = " <<col<< " , " <<row<< endl;
			// make col and row 0-based:
			--col;
			--row;
		}
		
		
		// for each raster...
		next_field_index = num_existing_fields;
		for ( unsigned r = 0; r < rasts.size(); r++ ) {
			Raster* rast = rasts[r];
			int bands;
			rast->getSize(NULL, NULL, &bands);
			GDALDataset* dataset = rast->getDataset();

			if ( use_xy ) {
				// convert from (x,y) to (col,row) in this rast
				rast->toColRow(x, y, &col, &row);
				//cout<< "x,y = " <<x<< " , " <<y<< endl;
			}
			// else: (col,row) already given above.
			
			
			//
			// extract pixel at (col,row) from rast
			char* ptr = (char*) rast->getBandValuesForPixel(col, row);
			if ( ptr ) {
				// add these bands to csv
				for ( int b = 0; b < bands; b++ ) {
					GDALRasterBand* band = dataset->GetRasterBand(b+1);
					GDALDataType bandType = band->GetRasterDataType();

					double val = starspan_extract_double_value(bandType, ptr);
					out_file << delimiter << val;

					int bandTypeSize = GDALGetDataTypeSize(bandType) >> 3;
					ptr += bandTypeSize;
				}
			}
		}
		// end record
		out_file << endl;
	}

	// close files:
	for ( unsigned r = 0; r < rasts.size(); r++ ) {
		delete rasts[r];
	}
	in_file.close();
	out_file.close();

	cout<< "finished.\n";
	return 0;
}
		


