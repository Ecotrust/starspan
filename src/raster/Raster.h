/*
	raster - raster interface
	$Id: Raster.h,v 1.4 2008-04-22 20:59:47 crueda Exp $
*/
#ifndef Raster_h
#define Raster_h

#include "gdal.h"           
#include "gdal_priv.h"
#include "ogr_srs_api.h"

#include <vector>
#include <cstdio>

using namespace std;

/** Location of pixel in 0-based (col,row) coordinates.
  */
struct CRPixel {
	int col;
	int row;
	CRPixel(int col, int row) : col(col), row(row) {}
};


/** Represents a raster. */
class Raster {
public:
	// initializes this module
	static int init(void);
	
	// finishes this module
	static int end(void);
	
	/**
     * Creates a raster object representing an existing file. 
	 * Returns null if error 
     */
	static Raster* open(const char* filename);
	
	// Creates a raster object representing an existing file.
    // Deprecated: use Raster::open
	Raster(const char* filename);
	
	// Creates a raster object representing a new raster file.
	Raster(const char* filename, int width, int height, int bands, GDALDataType type);
	
	// Creates a raster object representing a new raster file of type GDT_Byte.
	Raster(const char* filename, int width, int height, int bands);
	
	
	GDALDataset* getDataset(void) { return hDataset; }
	
	// gets raster size in pixels and number of bands
	void getSize(int *width, int *height, int *bands);
	
	// gets raster coordinates
	void getCoordinates(double *x0, double *y0, double *x1, double *y1);
	
	// gets the pixel size as reported by the geo transform associated to
	// the dataset.
	void getPixelSize(double *pix_x_size, double *pix_y_size);

	/**
	  * Returns the size in byte of the internal buffer used to store
	  * the values obtained by getBandValuesForPixel.
	  */
	unsigned getBandValuesBufferSize();

	/**
	  * Returns a pointer to an internal buffer containing the values of all
	  * bands of this raster at the specified location.
	  * NULL is returned if (col,row) is invalid.
	  */
	void* getBandValuesForPixel(int col, int row);

	/**
	  * Reads band values for a given pixel.
	  * Returns a pointer to the GIVEN buffer containing the values of all
	  * bands of this raster at the specified location.
	  * NULL is returned if (col,row) is invalid.
	  * buffer MUST BE big enough according to the desired output type.
	  */
	void* getBandValuesForPixel(int col, int row, GDALDataType bufferType, void* buffer);

	/**
	  * Writes band values for a given pixel.
	  * Returns a pointer to the GIVEN buffer containing the values of all
	  * bands of this raster at the specified location.
	  * NULL is returned if (col,row) is invalid.
	  * buffer MUST BE big enough according to the desired output type.
	  */
	void* setBandValuesForPixel(int col, int row, GDALDataType bufferType, void* buffer);

	/**
	  * Gets the values corresponding to a given list of pixel
	  * locations from a given band.
	  * @param band_index Desired band. Note that 1 corresponds to the first band
	  *              (to keep consistency with GDAL).
	  * @param colrows Desired locations.
	  * @param list Where values are to be added.
	  *             Note that a 0.0 will be added where (col,row) is not valid.
	  * @return 0 iff OK.
	  */
	int getPixelDoubleValuesInBand(unsigned band_index, vector<CRPixel>* colrows, vector<double>& list);
	
	/**
	  * (x,y) to (col,row) conversion.
	  * Returned location (col,row) could be outside this raster extension.
	  */
	void toColRow(double x, double y, int *col, int *row);

	/**
	  * (x,y) to (col,row) conversion, where the upper left corner of (col,row)
	  * is closest to the given (x,y).
	  * Returned location (col,row) could be outside this raster extension.
	  */
	void toClosestColRow(double x, double y, int *col, int *row);

	// closes this raster.
	~Raster();
	
	// for debugging
	void report(FILE* file);
	
private:
    Raster(GDALDataset* hDataset);
    
	GDALDataset* hDataset;
    const char* pszProjection;
    double adfGeoTransform[6];
	bool geoTransfOK;
	double* bandValues_buffer;
	
	void report_corner(FILE* file,const char*,int,int);
	void _create(const char* filename, int width, int height, int bands, GDALDataType type);
};

#endif
