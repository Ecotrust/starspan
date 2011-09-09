/*
	LayerRasterizer
    Preliminary approach to refactor traverser mechanism.
    Started: 2008-04-17
	$Id: LayerRasterizer.h,v 1.1 2008-04-18 07:46:56 crueda Exp $
*/
#ifndef LayerRasterizer_h
#define LayerRasterizer_h

#include "common.h"
#include "ogrsf_frmts.h"
#include "cpl_conv.h"
#include "cpl_string.h"

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
		
		Iterator(PixSet* ps) : ps(ps) { 
            colrow = ps->_set.begin();
        }
        
	public:
        ~Iterator() {}
		bool hasNext(){
            return colrow != ps->_set.end();
        }
		void next(int *col, int *row) {
            *col = colrow->col;
            *row = colrow->row;
            colrow++;
        }
	};

	PixSet() {}
	~PixSet() {}
	
	void insert(int col, int row) {
        EPixel colrow(col, row);
        _set.insert(colrow);
    }
    
	int size() { return _set.size(); }
    
	bool contains(int col, int row) {
        return _set.find(EPixel(col, row)) != _set.end() ;
    }
    
	void clear() { _set.clear(); }
    
	Iterator* iterator() {
        return new PixSet::Iterator(this);
    }
};




/** ---- From: RastEvent
  * Event sent to observers every time an intersecting pixel is found.
  */
struct RastEvent {
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
	
	
	RastEvent(int col, int row, double x, double y) {
		pixel.col = col;
		pixel.row = row;
		pixel.x = x;
		pixel.y = y;
	}
};



/** ---- From GlobalInfo
  * Info passed in observer#init(info)
  */
struct RastInfo {
	
	/** A rectangle covering the raster extension. */
	OGRPolygon rasterPoly;
    
    /** The layer being traversed */
    OGRLayer* layer;
};


/** ---- from IntersectionInfo
  * Info passed in observer#intersection{Found,End} methods.
  */
struct RastIntersectionInfo {
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
  * Any object interested in doing some task as pixels are
  * found must implement this interface.
  */
class LayerRasterizerObserver {
public:
	virtual ~LayerRasterizerObserver() {}
	
	/**
	  * Called only once at the beginning of a traversal processing.
	  */
	virtual void init(RastInfo& info) {}
	
	/**
	  * A new intersecting feature has been found.
      * @param intersInfo
	  */
	virtual void intersectionFound(RastIntersectionInfo& intersInfo) {}
	
	/**
	  * Processing of intersecting feature has finished.  
      * @param intersInfo
	  */
	virtual void intersectionEnd(RastIntersectionInfo& intersInfo) {}
	
	/**
	  * Called when a new pixel location is computed as part
	  * of a rasterization.
	  */
	virtual void addPixel(RastEvent& ev) {}

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
 * Rasterizes the features in a given OGRLayer.
 */
class LayerRasterizer : LineRasterizerObserver {
public:
	/**
     * Creates a LayerRasterizer. 
     */
	LayerRasterizer(
        OGRLayer* layer,
        int width, int height,
        double x0, double y0, double x1, double y1,
        double pix_x_size, double pix_y_size
    );   
	
	/** destroys this object. */
	~LayerRasterizer();
	
	/** Gets the layer given in constructor. */
	OGRLayer* getLayer() { return layer; } 
	
	/**
	  * Sets the proportion of intersected area required for a pixel to be included.
	  * This parameter is only used during processing of polygons.
	  *
	  * @param pixprop A value assumed to be in [0.0, 1.0].
	  */
    void setPixelProportion(double pixprop) {
        pixelProportion = pixprop; 
    }

	/**
	  * Adds an observer to this rasterizer.
	  */
    void addObserver(LayerRasterizerObserver* aObserver) {
        observers.push_back(aObserver);
    }

	/**
	  * Gets the number of observers associated to this traverser.
	  */
	unsigned getNumObservers(void) { return observers.size(); } 

	/**
	  * convenience method to delete the observers associated with this traverser.
	  */
    void releaseObservers(void) {
        for ( vector<LayerRasterizerObserver*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
            delete *obs;
    }

	/**
	  * Sets the output to write progress info.
	  */
	void setProgress(double perc, ostream& out) { 
		progress_perc = perc;
		progress_out = &out;
	}
	
	/**
	  * Sets verbose flag
	  */
	void setVerbose(bool v) { 
		verbose = v;
	}
	
	/**
	  * Sets log output
	  */
	void setLog(ostream& log) { 
		logstream = &log;
	}
	
	/**
	  * Sets if invalid polygons should be skipped.
	  * By default all polygons are processed.
	  */
	void setSkipInvalidPolygons(bool b) { 
		skip_invalid_polys = b;
	}
	
	/**
	  * Executes the rasterization.
	  */
	void rasterize(void);

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
	
	
	
private:
	
	
	struct _Rect {
		LayerRasterizer* tr;
		
		double x, y;     // origin in grid coordinates
		int cols, rows;  // size in pixels
		
		_Rect(LayerRasterizer* tr, double x, double y, int cols, int rows): 
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
	
    OGRLayer* layer;
	vector<LayerRasterizerObserver*> observers;

	double pixelProportion;
	RastInfo globalInfo;
	int width, height;
	double x0, y0, x1, y1;
	double pix_x_size, pix_y_size;
	double pix_abs_area;
	double pixelProportion_times_pix_abs_area;
	OGREnvelope raster_env;
	LineRasterizer* lineRasterizer;
	void notifyObservers(void);

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
		
		RastEvent event(col, row, x, y);
		summary.num_processed_pixels++;
		
		// notify observers:
		for ( vector<LayerRasterizerObserver*>::const_iterator obs = observers.begin(); obs != observers.end(); obs++ )
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
	bool verbose;
	ostream* logstream;
	bool debug_dump_polys;
	bool skip_invalid_polys;
	
	WKTWriter wktWriter;
};

#endif

