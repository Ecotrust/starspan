//
// STARSpan project
// Carlos A. Rueda
// starspan_csv2 - generate a CSV file from multiple rasters with duplicate pixel handling
// $Id: starspan_csv2.cc,v 1.1 2008-02-04 23:13:21 crueda Exp $
//

#include "starspan.h"
#include "traverser.h"
#include "Csv.h"

#include <stdlib.h>
#include <assert.h>


static Vector* vect;
static vector<const char*>* select_fields;
static int layernum;

static const char* csv_filename;



static void extractFunction(ExtractionItem* item) {
    
	// strategy:
	// - Set globalOptions.FID = feature->GetFID();
	// - Set list of rasters to only the one given
	// - Call starspan_csv() 
	
	// - Set globalOptions.FID = feature->GetFID();
	globalOptions.FID = item->feature->GetFID();
	
	// - Set list of rasters to only the one given
	vector<const char*> raster_filenames;
	raster_filenames.push_back(item->rasterFilename);
	
	// - Call starspan_csv()
	bool prevResetReading = Traverser::_resetReading;
	Traverser::_resetReading = false;
	int ret = starspan_csv(
		vect,
		raster_filenames,
		select_fields,
		csv_filename,
		layernum
	);
	Traverser::_resetReading = prevResetReading;
	
	if ( globalOptions.verbose ) {
		cout<< "--starspan_csv2: do_extraction: starspan_csv returned: " <<ret<< endl;
	}
}

//
// FR 200337: Duplicate pixel handling.
// It uses starspan_dup_pixel
//
int starspan_csv2(
	Vector* _vect,
	vector<const char*> raster_filenames,
	vector<const char*> *mask_filenames,
	vector<const char*>* _select_fields,
	int _layernum,
	vector<DupPixelMode>& dupPixelModes,
	const char* _csv_filename
) {
    vect = _vect;
    select_fields = _select_fields;
    layernum = _layernum;
    csv_filename = _csv_filename;
    
    return starspan_dup_pixel(
        vect,
        raster_filenames,
        mask_filenames,
        select_fields,
        layernum,
        dupPixelModes,
        extractFunction
    );
}

