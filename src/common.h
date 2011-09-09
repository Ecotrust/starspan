//
// starspan common declarations
// Carlos A. Rueda
// $Id: common.h,v 1.15 2008-05-15 20:44:28 crueda Exp $
//

#ifndef starspan_common_h
#define starspan_common_h

#include <string>
#include <sstream>
#include <vector>
#include <cassert>

using namespace std;


// TODO: parameterize this name
#define RID_colName "RID" 


/** buffer parameters.
 * These parameters are applied on geometry features
 * before computing the intersection.
 * By default, no buffer operation will be applied.
  */
struct BufferParams {
	/** were given? */
	bool given;
	
	/** Distance to be applied.
	  * If it starts with '@' then it indicates the name of the attribute 
	  * from which this value will be obtained.
	  */
	string distance;

	/** Number of segments to use to approximate a quadrant of a circle.
	  * If it starts with '@' then it indicates the name of the attribute 
	  * from which this value will be obtained.
	  */
	string quadrantSegments;
};



/**
 * Parses a string for a size.
 * @param sizeStr the input string which may contain a suffix ("px") 
 * @param pix_size pixel size in case suffix "px" is given
 * @param size where the parsed size will be stored
 * @return 0 iff OK
 */ 
int parseSize(const char* sizeStr, double pix_size, double *size);


/** 
 * Box parameters.
 * Sets a fixed box centered according to bounding box
 * of the geometry features before computing the intersection.
 * By default, no box is used.
 */
struct BoxParams {
	/** were given? */
	bool given;
	
    /** given width and height */
	string width;
	string height;
    
    BoxParams() : 
        given(false), width(""), height(""), 
        parsed(false), pwidth(0), pheight(0) 
    {}
    
	
    /**
     * Parses the given width and height.
     * getParsedDims() can be called after this.
     * @param pix_x_size pixel size in x in case suffix "px" is given
     * @param pix_y_size pixel size in y in case suffix "px" is given
     * @return 0 iff OK
     */ 
    int parse(double pix_x_size, double pix_y_size) {
        int res = 0;
        if ( (res = parseSize(width.c_str(),  pix_x_size, &pwidth))
        ||   (res = parseSize(height.c_str(), pix_y_size, &pheight)) ) {
            return res;
        }
        parsed = true;
        return res;
    }

    /** Get the parsed dimensiones.
     * parse() must be called before this. 
     */
    void getParsedDims(double *pw, double *ph) {
        assert( parsed ) ;
        *pw = pwidth;
        *ph = pheight;
    }
    
private:
    bool parsed;
    double pwidth;
    double pheight;
    
};


/** Class for duplicate pixel modes.  */
struct DupPixelMode {
	string code;
	string param1;
	double arg;
	
	DupPixelMode(string code, double arg) :
		code(code), param1(""), arg(arg), withArg(true) {
	}
	
	DupPixelMode(string code) :
		code(code), param1(""), withArg(false) {
	}
	
	DupPixelMode(string code, string param1) :
		code(code), param1(param1), withArg(false) {
	}
	
	DupPixelMode(string code, string param1, int arg) :
		code(code), param1(param1), arg(arg), withArg(true) {
	}
	
	string toString() {
		ostringstream ostr;
		ostr << code;
        if ( param1.size() > 0 ) {
            ostr << " " << param1;
        }
        if ( withArg ) {
            ostr << " " << arg;
        }
		return ostr.str();
	}
    
    private:
        bool withArg;
};


/** 
 * vector selection parameters
 */
struct VectorSelectionParams {
    /** sql statement */
    string sql;
    
    /** where statement */
    string where;
    
    /** dialect */
    string dialect;
    
    VectorSelectionParams() : sql(""), where(""), dialect("") {}
    
};


/** Options that might be used by different services.
  */
struct GlobalOptions {
	bool use_pixpolys;
    
    /** should invalid polygons be skipped?
	 * By default all polygons are processed. 
     */
	bool skip_invalid_polys;

	/**
	 * The proportion of intersected area required for a pixel to be included.
	 * This parameter is only used during processing of polygons.
     * Value assumed to be in [0.0, 1.0].
     */
	double pix_prop;
	
	/** vector selection parameters */
	VectorSelectionParams vSelParams;
	
	/** If non-negative, it will be the desired FID to be extracted */
	long FID;
	
	bool verbose;
	
	bool progress;
	double progress_perc;

	/** param noColRow if true, no col,row fields will be included */
	bool noColRow;
	
	/** if true, no x,y fields will be included */
  	bool noXY;

	bool only_in_feature;
	
	/** Style for RID values in output.
	  * "file" : the simple filename without path 
	  * "path" : exactly as obtained from command line or field in vector 
	  * "none" : no RID is included in output
	  */
	string RID;
	
	bool report_summary;
	
	/** value used as nodata */
	double nodata;  
	
	/** buffer parameters */
	BufferParams bufferParams;
	
	/** box parameters */
	BoxParams boxParams;
	
	/** miniraster parity */
	string mini_raster_parity;
	
	/** separation in pixels between minirasters in strip */
	int mini_raster_separation;
	
    
	/** separator for CSV files */
	string delimiter;
	
	
	/** Given duplicate pixel modes. Empty if not given. */
	vector<DupPixelMode> dupPixelModes;
};

extern GlobalOptions globalOptions;


#endif

