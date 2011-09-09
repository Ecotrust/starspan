//
// StarSpan project
// starspan declarations
// Carlos A. Rueda
// $Id: starspan.h,v 1.38 2008-07-08 07:43:20 crueda Exp $
//

#ifndef starspan_h
#define starspan_h

#include "config.h"

#define STARSPAN_VERSION VERSION

#include "common.h"           
#include "Raster.h"           
#include "Vector.h"       
#include "traverser.h"
#include "Stats.h"       

#include <cstdio>

/////////////////////////////////////////////////////////////////////////////
// services:

/**
  * Gets statistics for a given feature in a raster.
  * RESULT[s][b] = statistic s on band b
  * where:
  *    SUM <= s < TOT_RESULTS 
  *    0 <= b < #bands in raster
  * @param FID
  * @param vect
  * @param rast
  * @param select_stats List of desired statistics (avg, mode, stdev, min, max)
  */
double** starspan_getFeatureStats(
	long FID, Vector* vect, Raster* rast,
	vector<const char*> select_stats
); 


/**
  * Gets statistics for a feature in a raster.
  * RESULT[s][b] = statistic s on band b
  * where:
  *    SUM <= s < TOT_RESULTS
  *    0 <= b < #bands in raster
  * @param field_name    Field name
  * @param field_value   Field value
  * @param vect
  * @param rast
  * @param select_stats List of desired statistics (avg, mode, stdev, min, max)
  * @param FID  Output: If not null, it'll have the corresponding FID.
  */
double** starspan_getFeatureStatsByField(
	const char* field_name, 
	const char* field_value, 
	Vector* vect, 
	Raster* rast,
	vector<const char*> select_stats,
	long *FID
); 



/////////////////////////////////////////////////////////////////////////////
// main operations:


/**
  * Generates a CSV file with the following columns:
  *     FID, link, RID, BandNumber, FieldBandValue, <s1>_ImageBandValue, <s2>_ImageBandValue, ...
  * where:
  *     FID:          (informative.  link is given by next field)
  *     link:         link field
  *     RID           raster filename
  *     BandNumber    1 .. N
  *     FieldBandValue  value from input speclib
  *     <s1>_ImageBandValue  
  *                   selected statistics for BandNumber in RID
  *                   example, avg_ImageBandValue.
  *
  * @param vect Vector datasource
  * @param raster_filenames rasters
  * @param speclib_filename spectral library file name
  * @param link_name Name of field to be used as link vector-speclib
  * @param select_stats List of desired statistics (avg, mode, stdev, min, max)
  * @param calbase_filename output file name
  *
  * @return 0 iff OK 
  */
int starspan_tuct_2(
	Vector* vect,
	vector<const char*> raster_filenames,
	const char* speclib_filename,
	const char* link_name,
	vector<const char*> select_stats,
	const char* calbase_filename
);



/** Stats calculation one multiple rasters.
  * Generates a CSV file with the following columns:
  *     FID, {vect-attrs}, RID, numPixels, S1_Band1, S1_Band2 ..., S2_Band1, S2_Band2 ...
  * where:
  *     FID:          feature ID as given by OGRFeature#GetFID()
  *     {vect-attrs}: attributes from vector
  *     RID:          Name of raster from which band values are extracted
  *     numPixels     Number of pixels in feature
  *     <s>_Band<b>   statistic s for band b 
  *
  * @param vect Vector datasource
  * @param raster_filenames rasters
  * @param select_stats List of desired statistics
  * @param select_fields desired fields from vector
  * @param csv_filename output file name
  * @param layernum layer number within the vector datasource
  *
  * @return 0 iff OK 
  */
int starspan_stats(
	Vector* vect,
	vector<const char*> raster_filenames,
	vector<const char*> select_stats,
	vector<const char*>* select_fields,
	const char* csv_filename,
	int layernum
);

/**
  * Gets an observer that computes statistics for each FID.
  *
  * @param tr Data traverser
  * @param select_stats List of desired statistics (avg, stdev, min, max)
  * @param select_fields desired fields
  * @param filename output file name
  *
  * @return observer to be added to traverser. 
  */
Observer* starspan_getStatsObserver(
	Traverser& tr,
	vector<const char*> select_stats,
	vector<const char*>* select_fields,
	const char* filename
);


/**
  * Gets an observer that computes counts per class from an
  * integral raster band. Only the first band from the raster
  * dataset will be processed as long as its type is integral.
  * The format of the output file is as follows:
  * <pre>
  *		FID,class,count
  * </pre>
  *
  * @param tr Data traverser
  * @param filename output file name
  *
  * @return observer to be added to traverser. 
  */
Observer* starspan_getCountByClassObserver(
	Traverser& tr,
	const char* filename
);




/** Extraction from rasters specified in vector field.
  * Generates a CSV file with the following columns:
  *     FID, {vect-attrs}, [col,row,] [x,y,] {rast-bands}
  * where:
  *     FID:          feature ID as given by OGRFeature#GetFID()
  *     {vect-attrs}: attributes from vector, including the specified RID field
  *     col,row:      pixel location relative to [0,0] in raster
  *     x,y:          pixel location in geographic coordinates
  *     {rast-bands}: bands from raster RID at corresponding location
  *
  * @param vect Vector datasource
  * @param raster_field_name Name of field containing the raster filename
  * @param raster_directory raster filenames are relative to this directory
  * @param select_fields desired fields from vector
  * @param csv_filename output file name
  * @param layernum layer number within the vector datasource
  *
  * @return 0 iff OK 
  */
int starspan_csv_raster_field(
	Vector* vect,
	const char* raster_field_name,
	const char* raster_directory,
	vector<const char*>* select_fields,
	const char* csv_filename,
	int layernum
);


/** Extraction from multiple rasters.
  * Generates a CSV file with the following columns:
  *     FID, {vect-attrs}, RID, [col,row,] [x,y,] {rast-bands}
  * where:
  *     FID:          feature ID as given by OGRFeature#GetFID()
  *     {vect-attrs}: attributes from vector
  *     RID:          Name of raster from which band values are extracted
  *     col,row:      pixel location relative to [0,0] in raster
  *     x,y:          pixel location in geographic coordinates
  *     {rast-bands}: bands from raster RID at corresponding location
  *
  * @param vect Vector datasource
  * @param raster_filenames rasters
  * @param select_fields desired fields from vector
  * @param csv_filename output file name
  * @param layernum layer number within the vector datasource
  *
  * @return 0 iff OK 
  */
int starspan_csv(
	Vector* vect,
	vector<const char*> raster_filenames,
	vector<const char*>* select_fields,
	const char* csv_filename,
	int layernum
);



/**
 * FR 200337 Duplicate pixel handling.
 *
 * @param dupPixelModes modes
 */
int starspan_csv_dup_pixel(
	Vector* vect,
	vector<const char*> raster_filenames,
	vector<const char*> *mask_filenames,     //  FR 200341
	vector<const char*>* select_fields,
	const char* csv_filename,
	int layernum,
	vector<DupPixelMode>& dupPixelModes
);


/** Generates ENVI output
  * @param tr Data traverser
  * @param select_fields desired fields
  * @param envisl_filename output file name
  * @param envi_image  true for image, false for spectral library
  *
  * @return observer to be added to traverser. 
  */
Observer* starspan_gen_envisl(
	Traverser& tr,
	vector<const char*>* select_fields,
	const char* envisl_name,
	bool envi_image
);


/**
  * Updates a CSV
  * DOC ME!
  */
int starspan_update_csv(
	const char* in_csv_filename,
	vector<const char*> raster_filenames,
	const char* out_csv_filename
);


/**
  * Creates a ptplot-readable file with geometries and
  * grid.
  *
  * @param tr Data traverser
  * @param filename output file name
  *
  * @return observer to be added to traverser. 
  */
Observer* starspan_dump(
	Traverser& tr,
	const char* filename
);

/** this allows to dump one specific feature. */
void dumpFeature(Vector* vector, long FID, const char* filename);



/**
  * Generate a JTS test.
  *
  * @param tr Data traverser
  * @param jtstest_filename output file name
  *
  * @return observer to be added to traverser. 
  */
Observer* starspan_jtstest(
	Traverser& tr,
	const char* jtstest_filename
);

/**
  * Creates mini-rasters.
  *
  * @param prefix
  * @param pszOutputSRS 
  *		see gdal_translate option -a_srs 
  *		If NULL, projection is taken from input dataset
  *
  * @return observer to be added to traverser. 
  */
Observer* starspan_getMiniRasterObserver(
	const char* prefix,
	const char* pszOutputSRS
);



/**
  * Creates a strip of mini-rasters.
  *
  * @param tr Data traverser
  * @param basefilename  Base name used to create output files
  * @param shpfilename If non-null, name to create output shapefile
  *
  * @return observer to be added to traverser. 
  */
Observer* starspan_getMiniRasterStripObserver(
	Traverser& tr,
	string basefilename,
    string shpfilename
);



/////////////////////////////////////////////////////////////////////////////
// misc and supporting utilities:

/** aux routines for reporting */
void starspan_report_vector(Vector* vect);
void starspan_report_raster(Raster* rast);
void starspan_report(Traverser& tr);

void starspan_print_envelope(FILE* file, const char* msg, OGREnvelope& env);


/** 
  * Creates a raster by subsetting a given raster
  */
GDALDatasetH starspan_subset_raster(
	GDALDatasetH hDataset,        // input dataset
	int          xoff,
	int          yoff,            // xoff,yoff being the ulx, uly in pixel/line
	int          xsize, 
	int          ysize,
	
	const char*  pszDest,         // output dataset name
	const char*  pszOutputSRS,    // see -a_srs option for gdal_translate.
	                              // If NULL, projection is taken from input dataset
								  
	int          xsize_incr,      // used to create the raster 
	int          ysize_incr,      // used to create the raster

	double		 *nodata          // used if not null
);


/**
  * Extracts a value from a buffer according to a type and returns it as a double.
  */
inline double starspan_extract_double_value(GDALDataType bandType, void* ptr) {
	double value;
	switch(bandType) {
		case GDT_Byte:
			value = (double) *( (char*) ptr );
			break;
		case GDT_UInt16:
			value = (double) *( (unsigned short*) ptr );
			break;
		case GDT_Int16:
			value = (double) *( (short*) ptr );
			break;
		case GDT_UInt32: 
			value = (double) *( (unsigned int*) ptr );
			break;
		case GDT_Int32:
			value = (double) *( (int*) ptr );
			break;
		case GDT_Float32:
			value = (double) *( (float*) ptr );
			break;
		case GDT_Float64:
			value = (double) *( (double*) ptr );
			break;
		default:
			fprintf(stderr, "Unexpected GDALDataType: %s\n", GDALGetDataTypeName(bandType));
			exit(1);
	}
	return value;
}

/**
  * Gets a value from a band as a string.
  */
inline void starspan_extract_string_value(GDALDataType bandType, char* ptr, char* value) {
	switch(bandType) {
		case GDT_Byte:
			sprintf(value, "%d", (int) *( (char*) ptr ));
			break;
		case GDT_UInt16:
			sprintf(value, "%u", *( (unsigned short*) ptr ));
			break;
		case GDT_Int16:
			sprintf(value, "%d", *( (short*) ptr ));
			break;
		case GDT_UInt32:
			sprintf(value, "%u", *( (unsigned int*) ptr ));
			break;
		case GDT_Int32:
			sprintf(value, "%u", *( (int*) ptr ));
			break;
		case GDT_Float32:
			sprintf(value, "%f", *( (float*) ptr ));
			break;
		case GDT_Float64:
			sprintf(value, "%f", *( (double*) ptr ));
			break;
		default:
			fprintf(stderr, "Unexpected GDALDataType: %s\n", GDALGetDataTypeName(bandType));
			exit(1);
	}
}


/**
  * Extracts a value from a buffer according to a type and returns it as an integer.
  */
inline int starspan_extract_int_value(GDALDataType bandType, void* ptr) {
	int value;
	switch(bandType) {
		case GDT_Byte:
			value = (int) *( (char*) ptr );
			break;
		case GDT_UInt16:
			value = (int) *( (unsigned short*) ptr );
			break;
		case GDT_Int16:
			value = (int) *( (short*) ptr );
			break;
		case GDT_UInt32: 
			value = (int) *( (unsigned int*) ptr );
			break;
		case GDT_Int32:
			value = (int) *( (int*) ptr );
			break;
		case GDT_Float32:
			value = (int) *( (float*) ptr );
			break;
		case GDT_Float64:
			value = (int) *( (double*) ptr );
			break;
		default:
			fprintf(stderr, "Unexpected GDALDataType: %s\n", GDALGetDataTypeName(bandType));
			exit(1);
	}
	return value;
}

/** gets rid of a possible path in the name. */
inline void starspan_simplify_filename(string& filename) {
	size_t idx = filename.find_last_of("/:\\");
	if ( idx != filename.npos ) {
		filename.erase(0, idx+1);
	}
}



////////////////////////////////////////////////////////////////////////////////
// general raster selection processing

/**
 * A pair (feature,raster) indicating the selected raster to extract
 * data from for and feature.
 */
struct ExtractionItem {
    OGRFeature* feature;
    const char* rasterFilename;
};


/**
 * Performs extraction on a per feature basis: for each feature, it selects
 * a raster from the given list of raster and calls the provided 
 * extractionFunction with the corresponding extraction item.
 *
 * @param vect List of features
 * @param raster_filenames List of rasters
 * @param mask_filenames List of corresponding masks for the rasters
 * @param select_fields List of desired fields
 * @param layernum
 * @param dupPixelModes Modes for duplicate pixel handling
 * @param extractionFunction Function to call for each extraction item.
 *
 * @return 0 iff successful.
 */
int starspan_dup_pixel(
	Vector* vect,
	vector<const char*> raster_filenames,
	vector<const char*> *mask_filenames,
	vector<const char*>* select_fields,
	int layernum,
	vector<DupPixelMode>& dupPixelModes,
	void (*extractionFunction)(ExtractionItem* item)
);


int starspan_csv2(
	Vector* _vect,
	vector<const char*> raster_filenames,
	vector<const char*> *mask_filenames,
	vector<const char*>* _select_fields,
	int _layernum,
	vector<DupPixelMode>& dupPixelModes,
	const char* _csv_filename
);


int starspan_miniraster2(
	Vector* _vect,
	vector<const char*> raster_filenames,
	vector<const char*> *mask_filenames,
	vector<const char*>* _select_fields,
	int _layernum,
	vector<DupPixelMode>& dupPixelModes,
    const char*  _mini_prefix,
    const char*  _mini_srs
);


//////////////////////////////////////////////////////////////////////
// <GRASS>

/**
 * Determines if the GRASS interface should be used.
 * This is the case if the following conditions are met:
 *   - The program has been compiled with GRASS support, and
 *   - The program is running within the GRASS environment, and
 *   - The first argument is not "S" (for standard interface).
 * Updates the argument info if necessary.
 */
bool use_grass(int *argc, char ** argv);

/** The GRASS interface */
int starspan_grass(int argc, char ** argv);

// </GRASS>
//////////////////////////////////////////////////////////////////////


struct RasterizeParams {
    const char* outRaster_filename; 
    int rastValue;
    bool fillNoData;
    const char* rastFormat;
    const char* projection;
    double* geoTransform;
    
    RasterizeParams() : 
        outRaster_filename(0), 
        rastValue(1), 
        fillNoData(true),
        rastFormat("ENVI"),
        projection(0),
        geoTransform(new double[6])
    {
        for ( int i = 0; i < 6; i++ ) {
            geoTransform[i] = 0;
        }
        geoTransform[1] = geoTransform[5] = 1;
    }
    
    ~RasterizeParams() {
        delete geoTransform;
    }
};
Observer* starspan_getRasterizeObserver(RasterizeParams* rasterizeParams);



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
);


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
);


///////////////////////////////////////////////////
// mini raster basic information; A list of these elements
// is gathered by the main miniraster generator and then used by
// the strip generator to properly locate the minirasters within the strip.
//
struct MRBasicInfo {
	// corresponding FID from which the miniraster was extracted
	long FID;
    
    string mini_filename;
	
	// dimensions of this miniraster
	int width;
	int height;
    
    // row to locate this miniraster in strip:
    int mrs_row;
	
	MRBasicInfo(long FID, string& mini_filename, int width, int height, int mrs_row) : 
		FID(FID), mini_filename(mini_filename), width(width), height(height), mrs_row(mrs_row)
	{}
    
    /** Returns the next row just below this miniraster */
    int getNextRow() {
        return mrs_row + height;
    }
};


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
);


/**
 * Called by starspan_minirasterstrip2()
 * Creates a MiniRasterStripObserver with NO ownership over the
 * given output vector and layer, which could be null.
 * Ownership won't be taken over the mrbi_list either.
 * This observer won't call starspan_create_strip() at the end of each 
 * traversal-- the caller will presumably do that.
 */
Observer* starspan_getMiniRasterStripObserver2(
	string basefilename,
    string prefix,
    Vector* outVector,
    OGRLayer* outLayer,
    vector<MRBasicInfo>* mrbi_list
);


/**
 * generates a miniraster strip from multiple rasters with duplicate pixel handling
 */
int starspan_minirasterstrip2(
	Vector* _vect,
	vector<const char*> raster_filenames,
	vector<const char*> *mask_filenames,
	vector<const char*>* _select_fields,
	int _layernum,
	vector<DupPixelMode>& dupPixelModes,
    string basefilename,
    string shpfilename,
    string mrst_fid_filename,
    string mrst_glt_filename
);


#endif

