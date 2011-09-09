//
// Traverse
// Carlos A. Rueda
// $Id: traverser.h,v 1.29 2008-05-09 00:50:36 crueda Exp $
//

#ifndef traverser_h
#define traverser_h

#include "common.h"
#include "Raster.h"
#include "Vector.h"
#include "rasterizers.h"
#include "Progress.h"

#include <geos/version.h>
#if GEOS_VERSION_MAJOR < 3
	#include <geos.h>
	#include <geos/io.h>
#else
	#include <geos/util.h>

	#include <geos/geom/CoordinateArraySequence.h>
	#include <geos/geom/CoordinateSequence.h>
	#include <geos/geom/CoordinateSequenceFactory.h>
	#include <geos/geom/Envelope.h>
	#include <geos/geom/Geometry.h>
	#include <geos/geom/GeometryCollection.h>
	#include <geos/geom/GeometryFactory.h>
	#include <geos/geom/LineString.h>
	#include <geos/geom/LinearRing.h>
	#include <geos/geom/MultiLineString.h>
	#include <geos/geom/MultiPoint.h>
	#include <geos/geom/MultiPolygon.h>
	#include <geos/geom/Point.h>
	#include <geos/geom/Polygon.h>

	#include <geos/io/WKTWriter.h>
#endif





#include <set>
#include <list>
#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <cstdio>

using namespace std;

#if GEOS_VERSION_MAJOR < 3
	using namespace geos;
	
	#define EXC_STRING(ex) (ex)->toString()
#else
	using namespace geos;
	using namespace geos::util;
	using namespace geos::geom;
	using namespace geos::io;
	
	#define EXC_STRING(ex) (ex)->what()
#endif


/**
  * Pixel location.
  * Used as element for set of visited pixels
  */
class EPixel {
	public:
	int col, row;
	EPixel(int col, int row) : col(col), row(row) {}
	EPixel(const EPixel& p) : col(p.col), row(p.row) {}
	bool operator<(EPixel const &right) const {
		if ( col < right.col )
			return true;
		else if ( col == right.col )
			return row < right.row;
		else
			return false;
	}
};


/**
  * Set of visited pixels in feature currently being processed
  */
class PixSet {
	set<EPixel> _set;

public:
	class Iterator {
		friend class PixSet;
		
		PixSet* ps;
		set<EPixel>::iterator colrow;
		
		Iterator(PixSet* ps);
	public:
		~Iterator();
		bool hasNext();
		void next(int *col, int *row);
	};

	PixSet();
	~PixSet();
	
	void insert(int col, int row);
	int size();
	bool contains(int col, int row);
	void clear();
	Iterator* iterator();
};


/**
  * Info passed in observer#init(info)
  */
struct GlobalInfo {
	
	/** info about all bands in given rasters */
	vector<GDALRasterBand*> bands;
	                           
	/** A rectangle covering the raster extension. */
	OGRPolygon rasterPoly;
    
    /** Dimensions of the raster */
    int width, height;
    
    /** The layer being traversed */
    OGRLayer* layer;
};


// forward declaration
class Traverser;


/**
  * Info passed in observer#intersection{Found,End} methods.
  */
struct IntersectionInfo {
    /** the traverser sending the notification */
    Traverser* trv;
    
    /** Intersecting feature */
    OGRFeature* feature;
    
    /** The actual geometry used in intersection with raster envelope.
     * Unless some pre-processing operation has been applied (eg., buffer, box),
     * this will be equal to the original feature's geometry, ie., 
     * feature->GetGeometryRef().
     */
    OGRGeometry* geometryToIntersect;
    
    /** Initial intersection */
    OGRGeometry* intersection_geometry;
};


/**
  * Event sent to traversal observers every time an intersecting
  * pixel is found.
  * Note that all pixel locations are 0-based, with [0,0] denoting
  * the upper left corner pixel.
  */
struct TraversalEvent {
	/**
	  * Info about the location of pixels.
	  */
	struct {
		/** 0-based [col,row] location relative to global grid.  */
		int col;
		int row;
		
		/** corresponding geographic location in global grid. */
		double x;
		double y;
	} pixel;
	
	
	/**
	  * data from pixel in scanned raster.
	  * Only assigned if observer#isSimple() is false.
	  */
	void* bandValues;
	
	TraversalEvent(int col, int row, double x, double y) {
		pixel.col = col;
		pixel.row = row;
		pixel.x = x;
		pixel.y = y;
	}
};

/**
  * Any object interested in doing some task as geometries are
  * traversed must implement this interface.
  */
class Observer {
public:
	virtual ~Observer() {}
	
	/**
	  * Returns true if this observer is only interested in the location of
	  * intersecting pixels, in which case traversal event objects will
	  * be filled with only info about the pixel location.
      * This base class returns false, so events in addPixel notifications
      * will contain band values as well.
	  */
	virtual bool isSimple(void) { return false; }
	
	/**
	  * Called only once at the beginning of a traversal processing.
	  */
	virtual void init(GlobalInfo& info) {}
	
	/**
	  * A new intersecting feature has been found.
      * @param intersInfo
	  */
	virtual void intersectionFound(IntersectionInfo& intersInfo) {}
	
	/**
	  * Processing of intersecting feature has finished.  
      * @param intersInfo
	  */
	virtual void intersectionEnd(IntersectionInfo& intersInfo) {}
	
	/**
	  * A new pixel location has been computed.
	  * @param ev Associated event. Pixel location is always provided
	  * but raster bands are given only if isSimple() returns false.
	  */
	virtual void addPixel(TraversalEvent& ev) {}

	/**
	  * Called only once at the end of a traversal processing.
	  */
	virtual void end(void) {}
	
};


extern GeometryFactory* global_factory;
extern const CoordinateSequenceFactory* global_cs_factory;

// create pixel polygon
inline Polygon* create_pix_poly(double x0, double y0, double x1, double y1) {
	CoordinateSequence *cl = new DefaultCoordinateSequence();
	cl->add(Coordinate(x0, y0));
	cl->add(Coordinate(x1, y0));
	cl->add(Coordinate(x1, y1));
	cl->add(Coordinate(x0, y1));
	cl->add(Coordinate(x0, y0));
	LinearRing* pixLR = global_factory->createLinearRing(cl);
	vector<Geometry *>* holes = NULL;
	Polygon *poly = global_factory->createPolygon(pixLR, holes);
	return poly;
}


	



/**
  * A Traverser intersects every geometry feature in a vector datasource
  * with a raster dataset. Observers should be registered for the
  * actual work to be done.
  *
  * synopsis of usage:
  *
  * <pre>
  *		// create traverser:
  *		Traverser tr;
  *
  *		// give inputs:
  *		tr.setVector(v);
  *		tr.addRaster(r1);
  *		tr.addRaster(r2);
  *		...
  *		// add observers:  
  *		tr.addObserver(observer1);
  *		tr.addObserver(observer2);
  *		...
  *
  *		// run processing:
  *		tr.traverse();
  * </pre>
  */
class Traverser : LineRasterizerObserver {
public:

	/**
	  * Creates a traverser.
	  */
	Traverser(void);
	
	~Traverser();
	
	/**
	  * Sets the vector datasource.
	  * (eventually this would become addVector)
	  * @param v vector datadource
	  */
	void setVector(Vector* vector);

	/**
	  * Sets the vector datasource.
	  * (eventually this would become addVector)
	  * @param v vector datadource
	  */
	void setLayerNum(int vector_layernum);

	/**
	  * Gets the vector associated to this traverser.
	  */
	Vector* getVector(void) { return vect; }
	
	/**
	  * Gets the layernumber for the vector associated to this traverser.
	  */
	int getLayerNum(void) { return layernum; }
	
	/**
	  * Adds a raster.
	  * @param r the raster dataset
	  */
	void addRaster(Raster* raster);
	
	/**
	  * Gets the number of rasters.
	  */
	int getNumRasters(void) { return rasts.size(); }

	/**
	  * Gets a raster from the list of rasters.
	  */
	Raster* getRaster(int i) { return rasts[i]; }
	
	
	/** 
	  * Clear the list of rasters.
	  */
	void removeRasters(void);
	
	/**
	  * Only the given FID will be processed.
	  *
	  * @param FID  a FID.
	  */
	void setDesiredFID(long FID);

	/**
	  * Only the feature whose given field is equal to the given value
	  * FID will be processed.
	  * This method will have no effect if setDesiredFID(FID) is called with
	  * a valid FID.
	  *
	  * @param field_name Name of field
	  * @param field_value Value of the field
	  */
	void setDesiredFeatureByField(const char* field_name, const char* field_value);
	
	
	/**
	  * Adds an observer to this traverser.
	  */
	void addObserver(Observer* aObserver);

	/**
	  * Gets the number of observers associated to this traverser.
	  */
	unsigned getNumObservers(void) { return observers.size(); } 

	/**
	  * convenience method to delete the observers associated to this traverser.
	  */
	void releaseObservers(void);

	
	/**
	  * Gets the (minimum) size in bytes for a buffer to store 
	  * all band values. The result of this call varies as more
	  * rasters are added to this traverser.
	  *
	  * @return size in bytes.
	  */
	size_t getBandBufferSize(void) {
		return minimumBandBufferSize;
	}
	
	/**
	  * Gets the values in integer type corresponding to the set of
	  * visited pixels in current traversed feature.
	  * @param band_index Desired band. Note that 1 corresponds to the first band
	  *              (to keep consistency with GDAL).
	  * @param list Where values are to be added.
	  *             Note that a 0 will be added where (col,row) is not valid.
	  * @return 0 iff OK.
	  */
	int getPixelIntegerValuesInBand(unsigned band_index, vector<int>& list);
	
	/**
	  * Gets the values in double type corresponding to the set of
	  * visited pixels in current traversed feature.
	  * @param band_index Desired band. Note that 1 corresponds to the first band
	  *              (to keep consistency with GDAL).
	  * @param list Where values are to be added.
	  *             Note that a 0.0 will be added where (col,row) is not valid.
	  * @return 0 iff OK.
	  */
	int getPixelDoubleValuesInBand(unsigned band_index, vector<double>& list);
	
	/**
	  * Reads in values from all bands (all given rasters) at pixel in (col,row).
	  * Values are stored in bandValues_buffer.
	  *
	  * @param buffer where values are copied.  
	  *      Assumed to have at least getBandBufferSize() bytes allocated.
	  * @return buffer
	  */
	void* getBandValuesForPixel(int col, int row, void* buffer);
	
	/**
	  * Sets the output to write progress info.
	  */
	void setProgress(double perc, ostream& out) { 
		progress_perc = perc;
		progress_out = &out;
	}
	
	/**
	  * Sets log output
	  */
	void setLog(ostream& log) { 
		logstream = &log;
	}
	
	
	/**
	  * Executes the traversal.
	  * This traverser should not be modified while
	  * the traversal is performed. Otherwise unexpected
	  * behaviour may occur.
	  */
	void traverse(void);

	/**
	  * Returns the number of visited pixels
	  * in the current feature.
	  */
	inline unsigned getPixelSetSize(void) {
		return pixset.size();
	}
	
	/**                                
	  * True if pixel at [col,row] has been already visited
	  * according to current feature.
	  */
	inline bool pixelVisited(int col, int row) {
		return pixset.contains(col, row);
	}
	
	/** summary results for each traversal */
	struct {
		int num_intersecting_features;
		int num_point_features;
		int num_multipoint_features;
		int num_linestring_features;
		int num_multilinestring_features;
		int num_polygon_features;
		int num_multipolygon_features;
		int num_geometrycollection_features;
		int num_invalid_polys;
		int num_polys_with_internal_ring;
		int num_polys_exploded;
		int num_sub_polys;
		long num_processed_pixels;
		
	} summary;
	
	/** reports a summary of intersection to std output. */
	void reportSummary(void);
	
	
	/**
	 * By default, traverse() starts by calling ResetReading on the layer
	 * (_resetReading == true).
	 * However, this is in general not appropriate because it precludes the
	 * traversal from being used as a subtask. 
	 * For example, in starspan_csv_dup_pixel, all features are processed
	 * but the extraction from each one is done with a corresponding traversal.
	 * As a quick fix to allow for this beahaviuor at least for specific commands,
	 * this global (static) flag can be used to instruct the traversal not
	 * to call ResetReading on the layer. 
	 */
	static bool _resetReading;
	
private:
	
	
	struct _Rect {
		Traverser* tr;
		
		double x, y;     // origin in grid coordinates
		int cols, rows;  // size in pixels
		
		_Rect(Traverser* tr, double x, double y, int cols, int rows): 
		tr(tr), x(x), y(y), cols(cols), rows(rows)
		{}
		
		inline double area() { 
			return fabs(cols * tr->pix_x_size * rows * tr->pix_y_size);
		}
		
		inline double x2() { 
			return x + cols * tr->pix_x_size;
		}
		
		inline double y2() { 
			return y + rows * tr->pix_y_size;
		}
		
		inline bool empty() { 
			return cols <= 0 || rows <= 0;
		}
		
		inline _Rect upperLeft() { 
			int cols2 = (cols >> 1);
			int rows2 = (rows >> 1);
			return _Rect(tr, x, y, cols2, rows2);
		}
		
		inline _Rect upperRight() {
			int cols2 = (cols >> 1);
			int rows2 = (rows >> 1);
			double x2 = x + cols2 * tr->pix_x_size; 
			return _Rect(tr, x2, y, cols - cols2, rows2);
		}
		
		inline _Rect lowerLeft() {
			int cols2 = (cols >> 1);
			int rows2 = (rows >> 1);
			double y2 = y + rows2 * tr->pix_y_size; 
			return _Rect(tr, x, y2, cols2, rows - rows2);
		}
		
		inline _Rect lowerRight() {
			if ( cols <= 1 && rows <= 1 )
				return _Rect(tr, x, y, 0, 0);  // empty
			
			int cols2 = (cols >> 1);
			int rows2 = (rows >> 1);
			double x2 = x + cols2 * tr->pix_x_size; 
			double y2 = y + rows2 * tr->pix_y_size;
			return _Rect(tr, x2, y2, cols - cols2, rows - rows2);
		}
		
		inline Geometry* intersect(Polygon* p) {
			if ( empty() )
				return 0;
			
			Polygon* poly = create_pix_poly(x, y, x2(), y2());
			Geometry* inters = 0;
			try {
				inters = p->intersection(poly);
			}
			catch(TopologyException* ex) {
				cerr<< "TopologyException: " << EXC_STRING(ex) << endl;
				if ( tr->debug_dump_polys ) {
					cerr<< "pix_poly = " << tr->wktWriter.write(poly) << endl;
					cerr<< "geos_poly = " << tr->wktWriter.write(p) << endl;
				}
			}
			catch(GEOSException* ex) {
				cerr<< "GEOSException: " << EXC_STRING(ex) << endl;
				if ( tr->debug_dump_polys ) {
					WKTWriter wktWriter;
					cerr<< "pix_poly = " << tr->wktWriter.write(poly) << endl;
					cerr<< "geos_poly = " << tr->wktWriter.write(p) << endl;
				}
			}
			delete poly;
			return inters;
		}
	};
	
	Vector* vect;
	int layernum;
	vector<Raster*> rasts;
	vector<Observer*> observers;
	bool notSimpleObserver;

	long desired_FID;
	string desired_fieldName;
	string desired_fieldValue;
	
	GlobalInfo globalInfo;
	int width, height;
	double x0, y0, x1, y1;
	double pix_x_size, pix_y_size;
	double pix_abs_area;
	double pixelProportion_times_pix_abs_area;
	OGREnvelope raster_env;
	size_t minimumBandBufferSize;
	double* bandValues_buffer;
	LineRasterizer* lineRasterizer;
	void notifyObservers(void);
	void getBandValuesForPixel(int col, int row);
	
	/** (x,y) to (col,row) conversion */
	inline void toColRow(double x, double y, int *col, int *row) {
		*col = (int) floor( (x - x0) / pix_x_size );
		*row = (int) floor( (y - y0) / pix_y_size );
	}
	
	/** (col,row) to (x,y) conversion */
	inline void toGridXY(int col, int row, double *x, double *y) {
		*x = x0 + col * pix_x_size;
		*y = y0 + row * pix_y_size;
	}

	/** arbitrary (x,y) to grid (x,y) conversion */
	inline void xyToGridXY(double x, double y, double *gx, double *gy) {
		int col = (int) floor( (x - x0) / pix_x_size );
		int row = (int) floor( (y - y0) / pix_y_size );
		*gx = x0 + col * pix_x_size;
		*gy = y0 + row * pix_y_size;
	}
	
	// Does not check for duplication.
	// Return:
	//   -1: [col,row] out of raster extension
	//   0:  [col,row] dispached and added to pixset
	inline int dispatchPixel(int col, int row, double x, double y) {
		if ( col < 0 || col >= width  ||  row < 0 || row >= height ) {
			return -1;
		}
		
		TraversalEvent event(col, row, x, y);
		summary.num_processed_pixels++;
		
		// if at least one observer is not simple...
		if ( notSimpleObserver ) {
			// get also band values
			getBandValuesForPixel(col, row);
			event.bandValues = bandValues_buffer;
		}
		
		// notify observers:
		for ( vector<Observer*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
			(*obs)->addPixel(event);
		
		// keep track of processed pixels
		pixset.insert(col, row);
		return 0;
	}
	
	void processPoint(OGRPoint*);
	void processMultiPoint(OGRMultiPoint*);
	void processLineString(OGRLineString* linstr);
	void processMultiLineString(OGRMultiLineString* coll);
	void processValidPolygon(Polygon* geos_poly);
	void processValidPolygon_QT(Polygon* geos_poly);
	void rasterize_poly_QT(_Rect& env, Polygon* poly);
	void rasterize_geometry_QT(_Rect& env, Geometry* geom);
	void dispatchRect_QT(_Rect& r);
	void processPolygon(OGRPolygon* poly);
	void processMultiPolygon(OGRMultiPolygon* mpoly);
	void processGeometryCollection(OGRGeometryCollection* coll);
	void processGeometry(OGRGeometry* intersection_geometry, bool count);

	void process_feature(OGRFeature* feature);
	
	// LineRasterizerObserver	
	void pixelFound(double x, double y);

	// set of visited pixels:
	PixSet pixset;
                                    
	ostream* progress_out;
	double progress_perc;
	ostream* logstream;
	bool debug_dump_polys;
	bool debug_no_spatial_filter;
	
	WKTWriter wktWriter;
};

#endif

