//
// STARSpan project
// Carlos A. Rueda
// starspan_util - misc utilities
// $Id: starspan_util.cc,v 1.19 2008-07-08 07:43:20 crueda Exp $
//

#include "starspan.h"           
#include "vrtdataset.h"
#include "jts.h"       

#include <stdlib.h>
#include <iomanip>

// aux routine for reporting 
void starspan_report(Traverser& tr) {
	Vector* vect = tr.getVector();
	if ( vect ) {
		fprintf(stdout, "\n----------------VECTOR--------------\n");
		vect->report(stdout);
	}

	for ( int i = 0; i < tr.getNumRasters(); i++ ) {
		Raster* rast = tr.getRaster(i);
		fprintf(stdout, "\n----------------RASTER--------------\n");
		rast->report(stdout);
	}
}

// aux routine for reporting 
void starspan_report_vector(Vector* vect) {
	if ( vect ) {
		fprintf(stdout, "\n----------------VECTOR--------------\n");
		vect->report(stdout);
	}
}

// aux routine for reporting 
void starspan_report_raster(Raster* rast) {
	if ( rast ) {
		fprintf(stdout, "\n----------------RASTER--------------\n");
		rast->report(stdout);
	}
}





void starspan_print_envelope(FILE* file, const char* msg, OGREnvelope& env) {
	fprintf(file, "%s %.3f %.3f %.3f %.3f\n", msg, env.MinX, env.MinY, env.MaxX, env.MaxY);
}




//////////////////////////////////////////////////////////////////////////////
// Adapted from gdal_translate.cpp
//
GDALDatasetH starspan_subset_raster(
	GDALDatasetH hDataset,        // input dataset
	int          xoff,
	int          yoff,            // xoff,yoff being the ulx, uly in pixel/line
	int          xsize, 
	int          ysize,
	
	const char*  pszDest,         // output dataset name
	const char*  pszOutputSRS,    // see -a_srs option for gdal_translate
	                              // If NULL, projection is taken from input dataset

	int          xsize_incr,      // used to create the raster 
	int          ysize_incr,      // used to create the raster
	double		 *nodata          // used if not null
) {
	// input vars:
    int*              panBandList = NULL;
	int               nBandCount = 0;
    double		      adfGeoTransform[6];
    GDALProgressFunc  pfnProgress = GDALTermProgress;
    int               bStrict = TRUE;

	// output vars:
    const char*     pszFormat = "ENVI";
    GDALDriverH		hDriver;
	GDALDatasetH	hOutDS;
	int             anSrcWin[4] = { xoff, yoff, xsize, ysize };
	int             nOXSize = anSrcWin[2];
	int             nOYSize = anSrcWin[3];
	char**          papszCreateOptions = NULL;

	
	
/* -------------------------------------------------------------------- */
/*      Find the output driver.                                         */
/* -------------------------------------------------------------------- */
    hDriver = GDALGetDriverByName(pszFormat);
    if( hDriver == NULL ) {
        fprintf(stderr, "Output driver `%s' not recognised.\n", pszFormat);
		return NULL;
	}



/* -------------------------------------------------------------------- */
/*	Build band list to translate					*/
/* -------------------------------------------------------------------- */
	nBandCount = GDALGetRasterCount( hDataset );
	panBandList = (int *) CPLMalloc(sizeof(int)*nBandCount);
	for(int i = 0; i < nBandCount; i++ )
		panBandList[i] = i+1;
	
	
	
/* ==================================================================== */
/*      Create a virtual dataset.                                       */
/* ==================================================================== */
    VRTDataset *poVDS;
        
/* -------------------------------------------------------------------- */
/*      Make a virtual clone.                                           */
/* -------------------------------------------------------------------- */
    poVDS = new VRTDataset( nOXSize + xsize_incr, nOYSize + ysize_incr );

	// set projection:
	if ( pszOutputSRS ) {
		OGRSpatialReference oOutputSRS;

		if( oOutputSRS.SetFromUserInput(pszOutputSRS) != OGRERR_NONE )
		{
			fprintf( stderr, "Failed to process SRS definition: %s\n", 
					 pszOutputSRS);
			GDALDestroyDriverManager();
			exit( 1 );
		}

		fprintf(stdout, "---------exportToWkt\n"); fflush(stdout);
		char* wkt;
		oOutputSRS.exportToWkt(&wkt);
		fprintf(stdout, "---------Setting given projection (wkt)\n");
		//fprintf(stdout, "%s\n", wkt);
		poVDS->SetProjection(wkt);
		CPLFree(wkt);
	}
	else {
		const char* pszProjection = GDALGetProjectionRef( hDataset );
		if( pszProjection != NULL && strlen(pszProjection) > 0 ) {
			fprintf(stdout, "---------Setting projection from input dataset\n");
			//fprintf(stdout, "%s\n", pszProjection);
			poVDS->SetProjection(pszProjection);
		}
	}
	
	
    if( GDALGetGeoTransform( hDataset, adfGeoTransform ) == CE_None ) {
        adfGeoTransform[0] += anSrcWin[0] * adfGeoTransform[1]
            + anSrcWin[1] * adfGeoTransform[2];
        adfGeoTransform[3] += anSrcWin[0] * adfGeoTransform[4]
            + anSrcWin[1] * adfGeoTransform[5];
        
        adfGeoTransform[1] *= anSrcWin[2] / (double) nOXSize;
        adfGeoTransform[2] *= anSrcWin[3] / (double) nOYSize;
        adfGeoTransform[4] *= anSrcWin[2] / (double) nOXSize;
        adfGeoTransform[5] *= anSrcWin[3] / (double) nOYSize;
        
        poVDS->SetGeoTransform( adfGeoTransform );
    }
	
    poVDS->SetMetadata( ((GDALDataset*)hDataset)->GetMetadata() );
	
	
    for(int i = 0; i < nBandCount; i++ )
    {
        VRTSourcedRasterBand   *poVRTBand;
        GDALRasterBand  *poSrcBand;
        GDALDataType    eBandType;

        poSrcBand = ((GDALDataset *) hDataset)->GetRasterBand(panBandList[i]);
		
/* -------------------------------------------------------------------- */
/*      Select output data type to match source.                        */
/* -------------------------------------------------------------------- */
		eBandType = poSrcBand->GetRasterDataType();
		
		
/* -------------------------------------------------------------------- */
/*      Create this band.                                               */
/* -------------------------------------------------------------------- */
        poVDS->AddBand( eBandType, NULL );
        poVRTBand = (VRTSourcedRasterBand *) poVDS->GetRasterBand( i+1 );
            
/* -------------------------------------------------------------------- */
/*      Create a simple data source depending on the         */
/*      translation type required.                                      */
/* -------------------------------------------------------------------- */
		poVRTBand->AddSimpleSource( poSrcBand,
									anSrcWin[0], anSrcWin[1], 
									anSrcWin[2], anSrcWin[3], 
									0, 0, nOXSize, nOYSize );
		
/* -------------------------------------------------------------------- */
/*      copy over some other information of interest.                   */
/* -------------------------------------------------------------------- */
        poVRTBand->SetMetadata( poSrcBand->GetMetadata() );
        poVRTBand->SetColorTable( poSrcBand->GetColorTable() );
        poVRTBand->SetColorInterpretation(
            poSrcBand->GetColorInterpretation());
        if( strlen(poSrcBand->GetDescription()) > 0 )
            poVRTBand->SetDescription( poSrcBand->GetDescription() );
		
		if ( nodata ) 
            poVRTBand->SetNoDataValue(*nodata);
		else {
			int bSuccess;
			double dfNoData = poSrcBand->GetNoDataValue( &bSuccess );
			if ( bSuccess )
				poVRTBand->SetNoDataValue( dfNoData );
		}
    }

/* -------------------------------------------------------------------- */
/*      Write to the output file using CopyCreate().                    */
/* -------------------------------------------------------------------- */
    hOutDS = GDALCreateCopy( hDriver, pszDest, (GDALDatasetH) poVDS,
                             bStrict, papszCreateOptions, 
                             pfnProgress, NULL );

    GDALClose((GDALDatasetH) poVDS);
	CPLFree( panBandList );
  	
	
	return hOutDS;
}


/** 
 * Creates a layer for a given vector.
 * It copies the definition of all fields from input layer.
 *
 * @return The created layer.
 */
OGRLayer* starspan_createLayer(
    OGRLayer* inLayer,           // source layer
    Vector* outVector,           // vector where layer will be created
    const char *outLayerName     // name for created layer
) {
    if ( globalOptions.verbose ) {
        cout<< "starspan_createLayer: starting creation of output layer for vector " <<outVector->getName()<< " ...\n";
    }

    // get datasource to create layer:
    OGRDataSource *poDS = outVector->getDataSource();
    
    // get layer definition from the input layer:
    OGRFeatureDefn* inDefn = inLayer->GetLayerDefn();

    // create layer in output vector:
    OGRLayer* outLayer = poDS->CreateLayer(
            outLayerName, // "mini_raster_strip", 
            inLayer->GetSpatialRef(),
            inDefn->GetGeomType(),
            NULL
    );
    if ( !outLayer ) {
        cerr<< "Layer creation failed.\n";
        return 0;
    }
    
    // create field definitions from originating layer:
    for( int iAttr = 0; iAttr < inDefn->GetFieldCount(); iAttr++ ) {
        OGRFieldDefn* inField = inDefn->GetFieldDefn(iAttr);
        
        OGRFieldDefn outField(inField->GetNameRef(), inField->GetType());
        outField.SetWidth(inField->GetWidth());
        outField.SetPrecision(inField->GetPrecision());
        if ( outLayer->CreateField(&outField) != OGRERR_NONE ) {
            cerr<< "Creation of field failed: " <<inField->GetNameRef()<< "\n";
            return 0;
        }
    }
    
    return outLayer;
}


///////////////////////////////////////////////////
/** 
 * Validates the mutual consistency of the given list of elements.
 * <p>
 * Consistency requires that all given elements have:
 * <ul>
 *  <li> the same projection
 *  <li> the same pixel size in the case of rasters
 * </ul>
 *
 * @return 0 iff OK
 */
int starspan_validate_input_elements(
    Vector* vect,
    int vector_layernum,
	vector<const char*> raster_filenames,
	vector<const char*> *mask_filenames
) {
    // Either no masks are given, or the number of rasters and masks are the same:
    assert( mask_filenames == 0 || mask_filenames->size() == raster_filenames.size() );
    
    // base projection for comparison
    OGRSpatialReference* spatRef0 = 0;
    
    if ( vect ) {
        // get spatRef0 from vector's layer:
        OGRLayer* layer = vect->getLayer(vector_layernum);
        spatRef0 = layer->GetSpatialRef();
        // Note: spatRef0 should NOT be directly released in this case
    }
    
    
    // pixel size of first raster:
    double pix_x_size0 = 0, pix_y_size0 = 0;
    
	int res = 0;
	for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
        // open the raster momentarily to get required info:
		Raster rastr(raster_filenames[i]);
        
        if ( i == 0 ) {
            if ( spatRef0 ) {
                // compare first raster against vector's spatial reference:
                //
                // check projection:
                //
                const char* projection = rastr.getDataset()->GetProjectionRef();    
                OGRSpatialReference spatRef(projection);
                
                if ( !spatRef0->IsSame(&spatRef) ) {
                    cerr<< "Error: VECTOR and RASTER:\n"
                        << "   " <<vect->getName()<< "\n"
                        << "   " <<raster_filenames[0]<< "\n"
                    ;
                    char* wkt0;
                    char* wkt;
                    spatRef0->exportToProj4(&wkt0); 
                    spatRef.exportToProj4(&wkt); 
                    cerr<< "have different spatial references:\n" 
                        << "   " <<wkt0<< "\n"
                        << " --- vs. ---\n" 
                        << "   " <<wkt<< "\n"
                    ;
                    CPLFree(wkt0);
                    CPLFree(wkt);
                    res = 1;
                    break;
                }
            }
            else {
                // get spatRef0 from this first raster:
                const char* projection = rastr.getDataset()->GetProjectionRef();    
                spatRef0 = new OGRSpatialReference(projection);
                // Note: in this case, spatRef0 should be released, of course. 
            }
            rastr.getPixelSize(&pix_x_size0, &pix_y_size0);
        }
        else {
            //
            // check projection:
            //
            const char* projection = rastr.getDataset()->GetProjectionRef();    
            OGRSpatialReference spatRef(projection);
            
            if ( !spatRef0->IsSame(&spatRef) ) {
                cerr<< "Error: RASTERS:\n"
                    << "   " <<raster_filenames[0]<< "\n"
                    << "   " <<raster_filenames[i]<< "\n"
                ;
                char* wkt0;
                char* wkt;
                spatRef0->exportToProj4(&wkt0); 
                spatRef.exportToProj4(&wkt); 
                cerr<< "have different spatial references:\n" 
                    << "   " <<wkt0<< "\n"
                    << " --- vs. ---\n" 
                    << "   " <<wkt<< "\n"
                ;
                CPLFree(wkt0);
                CPLFree(wkt);
                res = 1;
                break;
            }
            
            //
            // check pixel size:
            //
            double pix_x_size, pix_y_size;
            rastr.getPixelSize(&pix_x_size, &pix_y_size);
            if ( pix_x_size0 != pix_x_size || pix_y_size0 != pix_y_size ) {
                cerr<< "Error: RASTERS:\n"
                    << "   " <<raster_filenames[0]<< "\n"
                    << "   " <<raster_filenames[i]<< "\n"
                    << "have different pixel sizes: " 
                    << pix_x_size0<< "x" <<pix_y_size0<< " vs. " 
                    << pix_x_size<< "x" <<pix_y_size<< "\n"
                ;
                res = 1;
                break;
            }
        }

        if ( mask_filenames ) {
            const char* mask_filename = (*mask_filenames)[i];
            Raster mask(mask_filename);

            //
            // check projection:
            //
            const char* projection = mask.getDataset()->GetProjectionRef();    
            OGRSpatialReference spatRef(projection);
            
            if ( !spatRef0->IsSame(&spatRef) ) {
                cerr<< "Error: RASTER and MASK:\n"
                    << "   " <<raster_filenames[0]<< "\n"
                    << "   " <<mask_filename      << "\n"
                ;
                char* wkt0;
                char* wkt;
                spatRef0->exportToProj4(&wkt0); 
                spatRef.exportToProj4(&wkt); 
                cerr<< "have different spatial references:\n" 
                    << "   " <<wkt0<< "\n"
                    << " --- vs. ---\n" 
                    << "   " <<wkt<< "\n"
                ;
                CPLFree(wkt0);
                CPLFree(wkt);
                res = 1;
                break;
            }
            
            //
            // check pixel size:
            //
            double pix_x_size, pix_y_size;
            mask.getPixelSize(&pix_x_size, &pix_y_size);
            if ( pix_x_size0 != pix_x_size || pix_y_size0 != pix_y_size ) {
                cerr<< "Error: RASTER and MASK:\n"
                    << "   " <<raster_filenames[0]<< "\n"
                    << "   " <<mask_filename      << "\n"
                    << "have different pixel sizes: " 
                    << pix_x_size0<< "x" <<pix_y_size0<< " vs. " 
                    << pix_x_size<< "x" <<pix_y_size<< "\n"
                ;
                res = 1;
                break;
            }
        }
	}
    
    if ( !vect && spatRef0 ) {
        // spatRef0 was not obtained from vector, so release it:
        delete spatRef0;
    }

    if ( !res && globalOptions.verbose ) {
        cout<< "starspan_validate_input_elements: OK\n";
        if ( vect ) {
            cout<< "  Vector: " <<vect->getName()<< ", layer " <<vector_layernum<< "\n";
        }
        else {
            cout<< "  Vector: not given\n";
        }
        cout<< "  Rasters: " <<raster_filenames.size();
        if ( mask_filenames ) {
            cout<< "  Masks: " <<mask_filenames->size()<< "\n";
        }
        else {
            cout<< " (no masks given)\n";
        }
    }
    
    return res;
}



///////////////////////////////////////////////////
// mini raster strip creation

static string create_filename_hdr(string prefix, long FID) {
    ostringstream ostr;
    ostr << prefix << setfill('0') << setw(4) << FID << ".hdr";
    return ostr.str();
}

/**
  * Creates output strips according to the minirasters registered in mrbi_list.
  */
void starspan_create_strip(
    GDALDataType strip_band_type,
    int strip_bands,
    string prefix,
    vector<MRBasicInfo>* mrbi_list,
    string strip_filename,
    string fid_filename,
    string loc_filename
) {
    if ( globalOptions.verbose ) {
        cout<< "starspan_create_strip: Creating miniraster strip...\n";
    }

    ///////////////////////
    // get ENVI driver
    const char * const pszFormat = "ENVI";
    GDALDriver* hDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if( hDriver == NULL ) {
        cerr<< "Couldn't get driver " <<pszFormat<< " to create output rasters.\n";
        return;
    }
    
    
    
    ////////////////////////////////////////////////////////
    // get dimensions for output strip images:
    
    // strip_width will be the maximum miniraster width:
    int strip_width = 0;		
    for ( vector<MRBasicInfo>::const_iterator mrbi = mrbi_list->begin(); mrbi != mrbi_list->end(); mrbi++ ) {
        if ( strip_width < mrbi->width ) {
            strip_width = mrbi->width;
        }
    }
    
    // strip_height will be equal to the index of the getNextRow call on the last element:
    int strip_height = mrbi_list->back().getNextRow();
    
    
    
    if ( globalOptions.verbose ) {
        cout<< "strip: width x height x bands: " 
            <<strip_width<< " x " <<strip_height<< " x " <<strip_bands<< endl;
    }

    /////////////////////////////////////////////////////////////////////
    // allocate transfer buffer with enough space for the longest row	
    const long doubles = (long)strip_width * strip_bands;
    if ( globalOptions.verbose ) {
        cout<< "Allocating " <<doubles<< " doubles for buffer\n";
    }
    double* buffer = new double[doubles];
    if ( !buffer ) {
        cerr<< " Cannot allocate " <<doubles<< " doubles for buffer\n";
        return;
    }
    
    //////////////////////////////////////////////
    // the band types for the strips: 
    const GDALDataType fid_band_type = GDT_Int32; 
    const GDALDataType loc_band_type = GDT_Float32; 

    
    ////////////////////////////////////////////////////////////////
    // create output rasters
    ///////////////////////////////////////////////////////////////

    char **papszOptions = NULL;
    
    /////////////////////////////////////////////////////////////////////
    // create strip image:
    GDALDataset* strip_ds = hDriver->Create(
        strip_filename.c_str(), strip_width, strip_height, strip_bands, 
        strip_band_type, 
        papszOptions 
    );
    if ( !strip_ds ) {
        delete buffer;
        cerr<< "Couldn't create " <<strip_filename<< endl;
        return;
    }
    // fill with globalOptions.nodata
    for ( int k = 0; k < strip_bands; k++ ) {
        strip_ds->GetRasterBand(k+1)->Fill(globalOptions.nodata);
    }
    
    /////////////////////////////////////////////////////////////////////
    // create FID 1-band image:
    GDALDataset* fid_ds = hDriver->Create(
        fid_filename.c_str(), strip_width, strip_height, 1, 
        fid_band_type, 
        papszOptions 
    );
    if ( !fid_ds ) {
        delete strip_ds;
        hDriver->Delete(strip_filename.c_str());
        delete buffer;
        cerr<< "Couldn't create " <<fid_filename<< endl;
        return;
    }
    // fill with -1 as "background" value. 
    // -1 is chosen as normally FIDs starts from zero.
    fid_ds->GetRasterBand(1)->Fill(-1);
    
    /////////////////////////////////////////////////////////////////////
    // create 2-band loc image:
    GDALDataset* loc_ds = hDriver->Create(
        loc_filename.c_str(), strip_width, strip_height, 2, 
        loc_band_type, 
        papszOptions 
    );
    if ( !loc_ds ) {
        delete fid_ds;
        hDriver->Delete(fid_filename.c_str());
        delete strip_ds;
        hDriver->Delete(strip_filename.c_str());
        delete buffer;
        cerr<< "Couldn't create " <<loc_filename<< endl;
        return;
    }
    // fill with 0 as "background" value.
    // 0 is arbitrarely chosen.
    loc_ds->GetRasterBand(1)->Fill(0);
    loc_ds->GetRasterBand(2)->Fill(0);
    
    
    /////////////////////////////////////////////////////////////////////
    // transfer data, fid, and loc from minirasters to output strips
    /////////////////////////////////////////////////////////////////////
    
    int processed_minirasters = 0;
    
    /////////////////////////////////////////////////////////////////////
    // for each miniraster (according to mrbi_list):
    for ( vector<MRBasicInfo>::const_iterator mrbi = mrbi_list->begin(); 
    mrbi != mrbi_list->end(); mrbi++, processed_minirasters++ ) {
    
        if ( globalOptions.verbose ) {
            cout<< "  adding miniraster FID=" <<mrbi->FID<< " to strip...\n";
        }

        // get miniraster filename:
        string mini_filename = mrbi->mini_filename; //create_filename(prefix, mrbi->FID);
        
        ///////////////////////
        // open miniraster
        GDALDataset* mini_ds = (GDALDataset*) GDALOpen(mini_filename.c_str(), GA_ReadOnly);
        if ( !mini_ds ) {
            cerr<< " Unexpected: couldn't read " <<mini_filename<< endl;
            hDriver->Delete(mini_filename.c_str());
            unlink(create_filename_hdr(prefix, mrbi->FID).c_str()); // hack
            continue;
        }
        
        // row to position miniraster in strip:
        int next_row = mrbi->mrs_row;

        // column to position miniraster in strip:
        int next_col = 0;
        

        ///////////////////////////////////////////////////////////////
        // transfer data to image strip (row by row):			
        for ( int i = 0; i < mini_ds->GetRasterYSize(); i++ ) {
            
            // read data from raster into buffer
            // (note: reading is always (0,0)-relative in source miniraster)
            mini_ds->RasterIO(GF_Read,
                0,   	                     //nXOff,
                0 + i,  	                 //nYOff,
                mini_ds->GetRasterXSize(),   //nXSize,
                1,                           //nYSize,
                buffer,                      //pData,
                mini_ds->GetRasterXSize(),   //nBufXSize,
                1,                           //nBufYSize,
                strip_band_type,             //eBufType,
                strip_bands,                 //nBandCount,
                NULL,                        //panBandMap,
                0,                           //nPixelSpace,
                0,                           //nLineSpace,
                0                            //nBandSpace
            );  	
            
            // write buffer in strip image
            // (note: wiritn is (next_col,next_row)-based in destination strip)
            strip_ds->RasterIO(GF_Write,
                next_col,                    //nXOff,
                next_row + i,                //nYOff,
                mini_ds->GetRasterXSize(),   //nXSize,
                1,                           //nYSize,
                buffer,                      //pData,
                mini_ds->GetRasterXSize(),   //nBufXSize,
                1,                           //nBufYSize,
                strip_band_type,             //eBufType,
                strip_bands,                 //nBandCount,
                NULL,                        //panBandMap,
                0,                           //nPixelSpace,
                0,                           //nLineSpace,
                0                            //nBandSpace
            );  	
        }
        
        ///////////////////////////////////////////////////////////////
        // write FID chunck by replication
        long fid_datum = mrbi->FID;
        
        if ( false ) {  
            // ----- should work but seems to be a RasterIO bug  -----
            // NOTE: The arguments in the following call need to be reviewed
            // since I haven't paid much attention to keep it up to date.
            fid_ds->RasterIO(GF_Write,
                next_col,                  //nXOff,
                next_row,                  //nYOff,
                mini_ds->GetRasterXSize(), //nXSize,
                mini_ds->GetRasterYSize(), //nYSize,
                &fid_datum,                //pData,
                1,                         //nBufXSize,
                1,                         //nBufYSize,
                fid_band_type,             //eBufType,
                1,                         //nBandCount,
                NULL,                      //panBandMap,
                0,                         //nPixelSpace,
                0,                         //nLineSpace,
                0                          //nBandSpace
            );
        }
        else { 
           // ------- workaround  -------------
            // replicate FID in buffer
            int* fids = (int*) buffer;
            for ( int j = 0; j < mini_ds->GetRasterXSize(); j++ ) {
                fids[j] = (int) mrbi->FID;
            }
            for ( int i = 0; i < mini_ds->GetRasterYSize(); i++ ) {
                fid_ds->RasterIO(GF_Write,
                    next_col,                   //nXOff,
                    next_row + i,               //nYOff,
                    mini_ds->GetRasterXSize(),  //nXSize,
                    1,                          //nYSize,
                    fids,                       //pData,
                    mini_ds->GetRasterXSize(),  //nBufXSize,
                    1,                          //nBufYSize,
                    fid_band_type,              //eBufType,
                    1,                          //nBandCount,
                    NULL,                       //panBandMap,
                    0,                          //nPixelSpace,
                    0,                          //nLineSpace,
                    0                           //nBandSpace
                );
            }
        }
        
        ///////////////////////////////////////////////////////////////
        // write loc data
        double adfGeoTransform[6];
        mini_ds->GetGeoTransform(adfGeoTransform);
        float pix_x_size = (float) adfGeoTransform[1];
        float pix_y_size = (float) adfGeoTransform[5];
        float x0 = (float) adfGeoTransform[0];
        float y0 = (float) adfGeoTransform[3];
        float xy[2] = { x0, y0 };
        for ( int i = 0; i < mini_ds->GetRasterYSize(); i++, xy[1] += pix_y_size ) {
            xy[0] = x0;
            for ( int j = 0; j < mini_ds->GetRasterXSize(); j++, xy[0] += pix_x_size ) {
                loc_ds->RasterIO(GF_Write,
                    next_col + j,               //nXOff,
                    next_row + i,               //nYOff,
                    1,                          //nXSize,
                    1,                          //nYSize,
                    xy,                         //pData,
                    1,                          //nBufXSize,
                    1,                          //nBufYSize,
                    loc_band_type,              //eBufType,
                    2,                          //nBandCount,
                    NULL,                       //panBandMap,
                    0,                          //nPixelSpace,
                    0,                          //nLineSpace,
                    0                           //nBandSpace
                );
            }
        }
        
        ///////////////////////////////////////////////////////////////
        //
        // is this the first miniraster?
        //
        if ( processed_minirasters == 0 ) {
            //
            // then, set projection for generated strips using the info from
            // this (arbitrarely chosen) first miniraster:
            //
            const char* projection = mini_ds->GetProjectionRef();
            if ( projection && strlen(projection) > 0 ) {
                strip_ds->SetProjection(projection);
                fid_ds->  SetProjection(projection);                   
                loc_ds->  SetProjection(projection);
            }
            
            //
            // also, set the geotransform accordingly, that is, keep them from
            // the first miniraster but adjust origin point to be (0,0):
            //
            adfGeoTransform[0] = 0;   // top left x
            adfGeoTransform[3] = 0;   // top left y
            strip_ds->SetGeoTransform(adfGeoTransform);
            fid_ds->  SetGeoTransform(adfGeoTransform);                   
            loc_ds->  SetGeoTransform(adfGeoTransform);
        }
        
        ///////////////////////////////////////////////////////////////
        if ( false ) {  
            // GDAL bug: ENVIDataset::FlushCache is not idempotent
            // for header file (repeated items would be generated)
            strip_ds->FlushCache();
            fid_ds->FlushCache();
            loc_ds->FlushCache();
        }
        else {
            // workaround: don't call FlushCache; it seems
            // we don't need these calls anyway.
        }
        
        // close and delete miniraster
        delete mini_ds;
        hDriver->Delete(mini_filename.c_str());
        unlink(create_filename_hdr(prefix, mrbi->FID).c_str()); // hack
    }
    
    // close outputs
    delete strip_ds;
    delete fid_ds;
    delete loc_ds;
    
    // release buffer
    delete buffer;
}


/**
 * Parses a string for a size.
 * @param sizeStr the input string which may contain a suffix ("px") 
 * @param pix_size pixel size in case suffix "px" is given
 * @param size where the parsed size will be stored
 * @return 0 iff OK
 */ 
int parseSize(const char* sizeStr, double pix_size, double *size) {
    int readChars = 0;
    sscanf(sizeStr, "%lf%n", size, &readChars);
    if ( readChars > 0 ) {
        const char* suffix = sizeStr + readChars;
        if ( strcmp(suffix, "px") == 0 ) {
            (*size) *= pix_size;
        }
        else if ( strlen(suffix) > 0 ) {
            // invalid suffix
            return -1;
        }
        return 0;   // OK
    }
    else {
        // not even the value was scanned successfully:
        return -2;
    }
}

