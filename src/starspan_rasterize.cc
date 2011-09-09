//
// StarSpan project
// Carlos A. Rueda
// starspan_rasterize - rasterizes and given vector
// $Id: starspan_rasterize.cc,v 1.1 2008-04-21 23:17:37 crueda Exp $
//

#include "starspan.h"
#include "traverser.h"

#include <stdlib.h>
#include <assert.h>


/**
  * This observer extracts from ONE raster
  */
class RasterizeObserver : public Observer {
public:
    RasterizeParams* rasterizeParams;
    
    GlobalInfo* global_info;
    OGRPolygon* inRasterPoly;
    
    // the buffer type and the output band type: 
    GDALDataType data_type;
    short* buffer;
    
    // output dataset:
    GDALDataset* ds;
    
	
	/**
	  * Creates a rasterizer
	  */
	RasterizeObserver(RasterizeParams* rasterizeParams)
    : rasterizeParams(rasterizeParams)
	{
	}
	
	/**
	  */
	void init(GlobalInfo& info) {
		global_info = &info;
		
        inRasterPoly = &info.rasterPoly;
        
        // start output raster:
		GDALDriver* hDriver = GetGDALDriverManager()->GetDriverByName(rasterizeParams->rastFormat);
		if( hDriver == NULL ) {
			cerr<< "Couldn't get driver " <<rasterizeParams->rastFormat<< " to create output rasters.\n";
			return;
		}
		// the dimensions for output raster:
		int width =  info.width ;		
		int height = info.height;
        
        /////////////////////////////////////////////////////////////////////
		// allocate transfer buffer with enough space for the longest row and type:	
		const long ints = (long) width ;
		if ( globalOptions.verbose ) {
			cout<< "Allocating " <<ints<< " short integers for buffer\n";
		}
		buffer = new short[ints];
		if ( !buffer ) {
			cerr<< " Cannot allocate " <<ints<< " elements for buffer\n";
			return;
		}
		
        //////////////////////////////////////////////
        // data_type: the buffer type and the output band type: 
        if ( rasterizeParams->rastValue <= 255 ) {
            data_type = GDT_Byte;
            unsigned char* bytes = (unsigned char*) buffer; 
            for ( int i = 0; i < ints; i++ ) {
                bytes[i] = (unsigned char) rasterizeParams->rastValue;
            }
        }
        else {
            data_type = GDT_Int16;
            short* shorts = (short*) buffer; 
            for ( int i = 0; i < ints; i++ ) {
                shorts[i] = (short) rasterizeParams->rastValue;
            }
        }        
        
        /////////////////////////////////////////////////////////////////////
		// create raster:
		ds = hDriver->Create(
			rasterizeParams->outRaster_filename, width, height, 1 /*bands*/, 
			data_type, 
			NULL   /*papszOptions*/ 
		);
		if ( !ds ) {
			delete buffer;
			cerr<< "Couldn't create " <<rasterizeParams->outRaster_filename<< endl;
			return;
		}
        
        if ( rasterizeParams->projection ) {
            ds->SetProjection(rasterizeParams->projection);
        }
        ds->SetGeoTransform(rasterizeParams->geoTransform);
        
        if ( rasterizeParams->fillNoData ) {
            // fill with globalOptions.nodata
            ds->GetRasterBand(1)->Fill(globalOptions.nodata);
        }
        
	}
	
    
    /** don't need pixel values */
    bool isSimple(void) { return true; }

	/**
	  * Puts a rastValue to the given pixel.
	  */
	void addPixel(TraversalEvent& ev) { 
		int col = ev.pixel.col;
		int row = ev.pixel.row;
		
        ds->RasterIO(GF_Write,
            col,               //nXOff,
            row,               //nYOff,
            1,                 //nXSize,
            1,                 //nYSize,
            buffer,            //pData,
            1,                 //nBufXSize,
            1,                 //nBufYSize,
            data_type,         //eBufType,
            1,                 //nBandCount,
            NULL,              //panBandMap,
            0,                 //nPixelSpace,
            0,                 //nLineSpace,
            0                  //nBandSpace
        );  	
	}

	/**
	  * Closes generated raster.
	  */
	void end() {
		if ( globalOptions.verbose ) {
			cout<< "Closing generated raster\n";
		}
        delete ds;
        cout<< "Done.\n";
    }

};



////////////////////////////////////////////////////////////////////////////////

Observer* starspan_getRasterizeObserver(RasterizeParams* rasterizeParams) {
    Observer* obs = new RasterizeObserver(rasterizeParams);
	return obs;
}

