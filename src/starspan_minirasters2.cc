//
// STARSpan project
// Carlos A. Rueda
// minirasters2 - generate minirasters from multiple rasters with duplicate pixel handling
// $Id: starspan_minirasters2.cc,v 1.6 2008-05-09 00:50:36 crueda Exp $
//

#include "starspan.h"
#include "traverser.h"
#include "Csv.h"

#include <stdlib.h>
#include <assert.h>


static Vector* vect;
static vector<const char*>* select_fields;
static int layernum;

static const char*  mini_prefix;
static const char*  mini_srs;



static void extractFunction(ExtractionItem* item) {
    
	// strategy:
	// - Set globalOptions.FID = feature->GetFID();
	// - Set list of rasters to only the one given
	// - Create and initialize a traverser
	// - Create and register MiniRasterObserver
    // - traverse
	
	// - Set globalOptions.FID = feature->GetFID();
	globalOptions.FID = item->feature->GetFID();
	
	// - Set list of rasters to only the one given
	vector<const char*> raster_filenames;
	raster_filenames.push_back(item->rasterFilename);
	
	// - Create and initialize a traverser
	bool prevResetReading = Traverser::_resetReading;
	Traverser::_resetReading = false;
    
	Traverser tr;
    
	tr.setVector(vect);
	tr.setLayerNum(layernum);

    Raster* raster = new Raster(item->rasterFilename);  
    tr.addRaster(raster);

    tr.setDesiredFID(globalOptions.FID);
    
    // - Create and register MiniRasterObserver
    Observer* obs = starspan_getMiniRasterObserver(mini_prefix, mini_srs);
	tr.addObserver(obs);

    // - traverse
    tr.traverse();
    
    tr.releaseObservers();
    
	Traverser::_resetReading = prevResetReading;
    
    delete raster;
	
	if ( globalOptions.verbose ) {
		cout<< "--starspan_miniraster2: completed." << endl;
	}
}

//
//
int starspan_miniraster2(
	Vector* _vect,
	vector<const char*> raster_filenames,
	vector<const char*> *mask_filenames,
	vector<const char*>* _select_fields,
	int _layernum,
	vector<DupPixelMode>& dupPixelModes,
    const char*  _mini_prefix,
    const char*  _mini_srs
) {
    vect = _vect;
    select_fields = _select_fields;
    layernum = _layernum;
    
    mini_prefix = _mini_prefix;
    mini_srs    = _mini_srs;
    
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

