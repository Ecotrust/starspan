//
// StarSpan project
// Carlos A. Rueda
// minirasterstrip2 - similar to minirasters2 but to generate a miniraster strip
//                    from multiple rasters with duplicate pixel handling
// $Id: starspan_minirasterstrip2.cc,v 1.6 2008-07-08 07:43:20 crueda Exp $
//

#include "starspan.h"
#include "traverser.h"

#include <stdlib.h>
#include <assert.h>


static Vector* vect;
static vector<const char*>* select_fields;
static int layernum;
                    
// the common observer while all features are traversed:
static Observer* obs;



static void mrs_extractFunction(ExtractionItem* item) {
    
	// strategy:
	// - Set globalOptions.FID = feature->GetFID();
	// - Set list of rasters to only the one given
	// - Create and initialize a traverser
	// - Register the common observer
    // - traverse
	
	// - Set globalOptions.FID = feature->GetFID();
	globalOptions.FID = item->feature->GetFID();
	
	// - Set list of rasters to only the one given
	vector<const char*> raster_filenames;
	raster_filenames.push_back(item->rasterFilename);
	
	// - Create and initialize a traverser
	Traverser tr;
    
	tr.setVector(vect);
	tr.setLayerNum(layernum);

    Raster raster(item->rasterFilename);  
    tr.addRaster(&raster);

    tr.setDesiredFID(globalOptions.FID);
    
    // - Register the common observer
	tr.addObserver(obs);

    // - traverse
    tr.traverse();
}

//
//
int starspan_minirasterstrip2(
	Vector* _vect,
	vector<const char*> raster_filenames,
	vector<const char*> *mask_filenames,
	vector<const char*>* _select_fields,
	int _layernum,
	vector<DupPixelMode>& dupPixelModes,
    string mrst_img_filename,
    string mrst_shp_filename,
    string mrst_fid_filename,
    string mrst_glt_filename
) {
    vect = _vect;
    select_fields = _select_fields;
    layernum = _layernum;

    
    Vector* outVector = 0;
    OGRLayer* outLayer = 0;
    
    // <shp>
    if ( mrst_shp_filename.length() ) {
        // prepare outVector and outLayer:
        
        if ( globalOptions.verbose ) {
            cout<< "starspan_minirasterstrip2: starting creation of output vector " <<mrst_shp_filename<< " ...\n";
        }
    
        outVector = Vector::create(mrst_shp_filename.c_str()); 
        if ( !outVector ) {
            // errors should have been written
            return 0;
        }                
    
        // create layer in output vector:
        outLayer = starspan_createLayer(
            vect->getLayer(layernum),
            outVector,
            "mini_raster_strip"
        );
        
        if ( outLayer == NULL ) {
            delete outVector;
            outVector = NULL;
            return 0;
        }
        
        // add RID column, if to be included
        if ( globalOptions.RID != "none" ) {
            OGRFieldDefn outField(RID_colName, OFTString);
            if ( outLayer->CreateField(&outField) != OGRERR_NONE ) {
                delete outVector;
                outVector = NULL;
                cerr<< "Creation of field failed: " <<RID_colName<< "\n";
                return 0;
            }
        }        
    }
    // </shp>
    
    
    string prefix = mrst_img_filename;
    prefix += "_TMP_PRFX_";
    
    // our own list to be updated and used later to create final strip:
    vector<MRBasicInfo> mrbi_list;

    // this observer will update outVector/outLayer if any, but won't
    // create any strip -- we will do it after the scan of features below
    // and with the help ow our own list of MRBasicInfo elements:
    obs = starspan_getMiniRasterStripObserver2(
        mrst_img_filename,
        prefix.c_str(),
        outVector,
        outLayer,
        &mrbi_list
    );
    
    
	bool prevResetReading = Traverser::_resetReading;
	Traverser::_resetReading = false;

    int res = starspan_dup_pixel(
        vect,
        raster_filenames,
        mask_filenames,
        select_fields,
        layernum,
        dupPixelModes,
        mrs_extractFunction
    );
    
	Traverser::_resetReading = prevResetReading;
    
    if ( res ) {
        // problems: messages should have been generated.
    }
    else {
        if ( mrbi_list.size() == 0 ) {
            cout<< "\nstarspan_minirasterstrip2: NOTE: No minirasters were generated.\n"; 
        }
        else {
            // OK: create the strip:
            // use the first raster in the list to get #band and band type:
            Raster rastr(raster_filenames[0]);                     
            int strip_bands;
            rastr.getSize(NULL, NULL, &strip_bands);
            GDALDataType strip_band_type = rastr.getDataset()->GetRasterBand(1)->GetRasterDataType();
            
            starspan_create_strip(
                strip_band_type,
                strip_bands,
                prefix,
                &mrbi_list,
                mrst_img_filename,
                mrst_fid_filename,
                mrst_glt_filename
            );
        }
    }
    
    // release dynamic objects:
    if ( outVector ) {
        delete outVector;
    }
    delete obs;
    
    
    // Done:
    return res;
}

