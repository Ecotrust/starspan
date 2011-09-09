//
// STARSpan project
// Carlos A. Rueda
// starspan_minirasters - generate mini rasters including strip
// $Id: starspan_minirasters.cc,v 1.24 2008-07-08 07:43:20 crueda Exp $
//

#include "starspan.h"           
#include "traverser.h"       
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <vector>

#include <unistd.h>   // unlink           
#include <string>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

using namespace std;


/**
  * Creates a miniraster for each traversed feature.
  */
class MiniRasterObserver : public Observer {
public:
	GlobalInfo* global_info;
	string prefix;
	const char* pszOutputSRS;
	GDALDatasetH hOutDS;

	// to help in determination of extrema col,row coordinates
	bool first;
	int mini_col0, mini_row0, mini_col1, mini_row1;
	
	// If not null, basic info is added for each created miniraster
	vector<MRBasicInfo>* mrbi_list;
    
	/**
	  * Creates the observer for this operation. 
	  */
	MiniRasterObserver(string aprefix, const char* pszOutputSRS)
	: prefix(aprefix), pszOutputSRS(pszOutputSRS)
	{
		global_info = 0;
		hOutDS = 0;
		mrbi_list = 0;
	}
	
	
	
	/**
	  * simply calls end()
	  */
	~MiniRasterObserver() {
		end();
	}
    
    
	/**
	  * Nothing done in this class.
	  */
	virtual void end() {
	}
	
	/**
	  * returns true:  we subset the raster directly for
	  *  each intersecting feature in intersectionEnd()
	  */
	bool isSimple() { 
		return true; 
	}

	/**
	  * 
	  */
	virtual void init(GlobalInfo& info) {
		global_info = &info;
	}

	/**
	  * Just sets first = true.
	  */
	void intersectionFound(IntersectionInfo& intersInfo) {
		first = true;
	}

	/**
	  * used to keep track of the extrema
	  */
	void addPixel(TraversalEvent& ev) {
		int col = ev.pixel.col;
		int row = ev.pixel.row;
		if ( first ) {
			first = false;
			mini_col0 = mini_col1 = col;
			mini_row0 = mini_row1 = row;
		}
		else {
			if ( mini_col0 > col )
				mini_col0 = col;
			if ( mini_row0 > row )
				mini_row0 = row;
			if ( mini_col1 < col )
				mini_col1 = col;
			if ( mini_row1 < row )
				mini_row1 = row;
		}
	}

	/** aux to create image filename */
	static string create_filename(string prefix, long FID) {
		ostringstream ostr;
		ostr << prefix << setfill('0') << setw(4) << FID << ".img";
		return ostr.str();
	}
	static string create_filename_hdr(string prefix, long FID) {
		ostringstream ostr;
		ostr << prefix << setfill('0') << setw(4) << FID << ".hdr";
		return ostr.str();
	}

	/**
	  * Proper creation of mini-raster with info gathered during traversal
	  * of the given feature.
	  */
	virtual void intersectionEnd(IntersectionInfo& intersInfo) {
        OGRFeature* feature = intersInfo.feature;
        OGRGeometry* intersection_geometry = intersInfo.intersection_geometry;
        
		if ( intersInfo.trv->getPixelSetSize() == 0 )
			return;
		
        
        // (width,height) of proper mini raster according to feature geometry
		int mini_width = mini_col1 - mini_col0 + 1;  
		int mini_height = mini_row1 - mini_row0 + 1;
		
		// create mini raster
		long FID = feature->GetFID();
		string mini_filename = create_filename(prefix, FID);
		
		// for parity; by default no parity adjustments:
		int xsize_incr = 0; 
		int ysize_incr = 0;
		
		if ( globalOptions.mini_raster_parity.length() > 0 ) {
            //
            // make necessary parity adjustments (on xsize_incr and/or ysize_incr) 
            //
			const char* mini_raster_parity = 0;
			if ( globalOptions.mini_raster_parity[0] == '@' ) {
				const char* attr = globalOptions.mini_raster_parity.c_str() + 1;
				int index = feature->GetFieldIndex(attr);
				if ( index < 0 ) {
					cerr<< "\n\tField `" <<attr<< "' not found";
					cerr<< "\n\tParity not performed\n";
				}
				else {
					mini_raster_parity = feature->GetFieldAsString(index);
				}
			}
			else {
				// just take the given parameter
				mini_raster_parity = globalOptions.mini_raster_parity.c_str();
			}

			int parity = 0;
			if ( 0 == strcmp(mini_raster_parity, "odd") )
				parity = 1;
			else if ( 0 == strcmp(mini_raster_parity, "even") )
				parity = 2;
			else {
				cerr<< "\n\tunrecognized value `" <<mini_raster_parity<< "' for parity";
				cerr<< "\n\tParity not performed\n";
			}
			
			if ( (parity == 1 && mini_width % 2 == 0)
			||   (parity == 2 && mini_width % 2 == 1) ) {
				xsize_incr = 1;
			}
			if ( (parity == 1 && mini_height % 2 == 0)
			||   (parity == 2 && mini_height % 2 == 1) ) {
				ysize_incr = 1;
			}
		}
        
        Raster* rastr = intersInfo.trv->getRaster(0);
        
		GDALDatasetH hOutDS = starspan_subset_raster(
			rastr->getDataset(),
			mini_col0, mini_row0, mini_width, mini_height,
			mini_filename.c_str(),
			pszOutputSRS,
			xsize_incr, ysize_incr,
			NULL // nodata -- PENDING
		);
		
		if ( globalOptions.only_in_feature ) {
			double nodata = globalOptions.nodata;
			
			if ( globalOptions.verbose )
				cout<< "nullifying pixels...\n";

			const int nBandCount = GDALGetRasterCount(hOutDS);
			
			int num_points = 0;			
			for ( int row = mini_row0; row <= mini_row1; row++ ) {
				for ( int col = mini_col0; col <= mini_col1 ; col++ ) {
					if ( intersInfo.trv->pixelVisited(col, row) ) {
						num_points++;
					}
					else {   // nullify:
						for(int i = 0; i < nBandCount; i++ ) {
							GDALRasterBand* band = ((GDALDataset *) hOutDS)->GetRasterBand(i+1);
							band->RasterIO(
								GF_Write,
								col - mini_col0, 
								row - mini_row0,
								1, 1,              // nXSize, nYSize
								&nodata,           // pData
								1, 1,              // nBufXSize, nBufYSize
								GDT_Float64,       // eBufType
								0, 0               // nPixelSpace, nLineSpace
							);
							
						}
					}
				}
			}
			if ( globalOptions.verbose )
				cout<< " " <<num_points<< " points retained\n";
		}

		if ( mrbi_list ) {
            int next_row = 0;   // will remain zero if mrbi_list is empty
            if ( mrbi_list->size() > 0 ) {
                // get last inserted mrbi:
                MRBasicInfo mrbi = mrbi_list->back();
                next_row = mrbi.getNextRow() + globalOptions.mini_raster_separation;
            }
            
			mrbi_list->push_back(MRBasicInfo(FID, mini_filename, mini_width, mini_height, next_row));
		}
		
		GDALClose(hOutDS);
		cout<< endl;
	}
};


Observer* starspan_getMiniRasterObserver(
	const char* prefix,
	const char* pszOutputSRS 
) {	
	return new MiniRasterObserver(prefix, pszOutputSRS);
}


/////////////////////////////////////////////////////////////////////
/////////// miniraster strip:
/////////////////////////////////////////////////////////////////////

static void translateGeometry(OGRGeometry* geometry, double x0, double y0);


/**
  * Extends MiniRasterObserver to override a couple of methods,
  * use mrbi_list, and create the strip output files.
  */
class MiniRasterStripObserver : public MiniRasterObserver {
	string basefilename;
    
    OGRLayer* inLayer;  // layer being traversed;
    
    // set when output vector should be created:
    Vector* outVector;
    OGRLayer* outLayer;
    bool ownOutVector;   // outVector and outLayer are mine?
    
    // updated in intersectionEnd; also used in end()
    Raster* rastr;
    
    // should we create strip at end of traversal?  true by default
    bool createStrip;    
    
    // is mrbi_list mine?  true by default
    bool ownMrbiList;
	
public:
	MiniRasterStripObserver(string bfilename, string prefix)
	: MiniRasterObserver(prefix, 0),
      basefilename(bfilename), inLayer(0), 
      outVector(0), outLayer(0), 
      ownOutVector(true), // although there is no outVector, actually
      createStrip(true)
	{
		mrbi_list = new vector<MRBasicInfo>();
        ownMrbiList = true;
	}
    
    
    /**
     * Sets the createStrip flag. By default true.
     * If this flag is true, then end() will call starspan_create_strip().
     */
    void setCreateStrip(bool b) {
        createStrip = b;
    }
    
    /**
     * Sets the list to add MRBasicInfo elements in, but its ownership remains the caller's
     */
    void setMrbiList(vector<MRBasicInfo>* mrbiList) {
        if ( ownMrbiList && this->mrbi_list ) {
            // release previous object:
            delete this->mrbi_list;
        }
        this->mrbi_list = mrbiList;
        ownMrbiList = false;
    }
    
    /**
     * Sets the output vector/layer but its ownership remains the caller's
     */
    void setOutVector(Vector* vtr, OGRLayer* lyr) {
        if ( ownOutVector && this->outVector) {
            // release previous object:
            delete this->outVector;
        }
        this->outVector = vtr;
        this->outLayer = lyr;
        ownOutVector = false;
    }
    
    /**
     * Sets the output vector/layer including its ownership.
     */
    void setOutVectorDirectly(Vector* vtr, OGRLayer* lyr) {
        if ( ownOutVector && this->outVector) {
            // release previous object:
            delete this->outVector;
        }
        this->outVector = vtr;
        this->outLayer = lyr;
        ownOutVector = true;
    }
    
    /**
     * Takes a handle on the layer being traversed.
     */
    void init(GlobalInfo& info) {
        MiniRasterObserver::init(info);
        
        // remember layer being traversed:
        inLayer = info.layer;
    }
    
    
    /**
     * Calls super.intersectionEnd(); if output vector was indicated, then
     * it copies fields and translates the feature data to the output vector.
     */
	virtual void intersectionEnd(IntersectionInfo& intersInfo) {
        MiniRasterObserver::intersectionEnd(intersInfo);

        // update rastr
        rastr = intersInfo.trv->getRaster(0);

		if ( !outLayer ) {
			return;  // not output layer under creation
        }
        
		if (intersInfo.trv->getPixelSetSize() == 0 ) {
			return;   // no pixels were actually intersected
        }
        
        OGRFeature* feature = intersInfo.feature;
        OGRGeometry* geometryToIntersect = intersInfo.geometryToIntersect;
        OGRGeometry* intersection_geometry = intersInfo.intersection_geometry;

        // create feature in output vector:
        OGRFeature *outFeature = OGRFeature::CreateFeature(outLayer->GetLayerDefn());
        
        // copy everything from incoming feature:
        // (TODO handle selected fields as in other commands) 
        // (TODO do not copy geometry, but set the corresponding geometry)
        outFeature->SetFrom(feature);
        
        //  make sure we release the copied geometry:
        outFeature->SetGeometryDirectly(NULL);

        // handle RID column:
        if ( globalOptions.RID != "none" ) {
            // Add RID value if the field exists:
            int idx = outFeature->GetFieldIndex(RID_colName);
            if ( idx >= 0 ) {
                string RID_value = rastr->getDataset()->GetDescription();
                if ( globalOptions.RID == "file" ) {
                    starspan_simplify_filename(RID_value);
                }
                outFeature->SetField(idx, RID_value.c_str());
            }
        }
        
        // depending on the associated geometry (see below), these offsets
        // will help in locating the geometry in the strip:
        double offsetX = 0;
        double offsetY = 0;
        
        double pix_x_size, pix_y_size;
        rastr->getPixelSize(&pix_x_size, &pix_y_size);
        
        // Associate the required geometry.
        // This will depend on what geometry was actually used for interesection:
        OGRGeometry* feature_geometry = feature->GetGeometryRef();
        if ( geometryToIntersect == feature_geometry ) {
            // if it was original feature's geometry, 
            // then associate intersection_geometry:
            outFeature->SetGeometry(intersection_geometry);
            // note that (offsetX,offsetY) will remain == (0,0)
        }
        else {
            // else, associate the intersection between original feature's 
            // geometry and intersection_geometry:
            try {
                OGRGeometry* featInters = feature_geometry->Intersection(intersection_geometry);
                outFeature->SetGeometryDirectly(featInters);
                // (note: featInters is a new object so we can use SetGeometryDirectly)
                
                // In this case, we need to take into account a possible offset
                // between featInters and intersection_geometry:
                
                OGREnvelope env1, env2;
                featInters->getEnvelope(&env1);
                intersection_geometry->getEnvelope(&env2);
                
                // the sign of the pixel size will indicate the way to get the offset: 
                offsetX = -(pix_x_size < 0 ? env2.MaxX - env1.MaxX : env2.MinX - env1.MinX);
                offsetY = -(pix_y_size < 0 ? env2.MaxY - env1.MaxY : env2.MinY - env1.MinY);
            }
            catch(GEOSException* ex) {
                cerr<< "min_raster_strip: GEOSException: " << EXC_STRING(ex) << endl;
                
                // should not happen, but well, assign intersection_geometry:
                outFeature->SetGeometry(intersection_geometry);
            }
        }
        
        
        //  grab a ref to the associated geometry for modification:
        OGRGeometry* outGeometry = outFeature->GetGeometryRef();

        
        // Adjust outGeometry to be relative to the strip so it can be overlayed:
        
        // get mrbi just pushed:
        MRBasicInfo mrbi = mrbi_list->back();
        int next_row = mrbi.mrs_row;   // base row for this miniraster in strip:

        // origin of outGeometry relative to:
        //      * origin of miniraster (0,       pix_y_size*next_row) 
        //      * and offset above     (offsetX, offsetY)
        double x0 = 0                   + offsetX;
        double y0 = pix_y_size*next_row + offsetY;
        
        // translate outGeometry:
        translateGeometry(outGeometry, x0, y0);

        // add outFeature to outLayer:
        if ( outLayer->CreateFeature(outFeature) != OGRERR_NONE ) {
           cerr<< "*** WARNING: Failed to create feature in output layer.\n";
           return;
        }
        OGRFeature::DestroyFeature(outFeature);
    }

	/**
	  * If createStrip is true, calls starspan_create_strip() and releases 
      * mrbi_list.
      * Then releases outVector if there is one and we own it.
	  */
	virtual void end() {
		if ( createStrip && mrbi_list ) {
            int strip_bands;
            rastr->getSize(NULL, NULL, &strip_bands);
            GDALDataType strip_band_type = rastr->getDataset()->GetRasterBand(1)->GetRasterDataType();
            
            //
            // NOTE: in this case, we still use the hard-coded suffixes:
            // instead of the ones given by the --xxx-suffix options.
            // But this is not to be used normally.
            // See starspan_minirasterstrip2()
            //
            string strip_filename = string(basefilename) + "_mr.img";
            string fid_filename =   string(basefilename) + "_mrid.img";
            string loc_filename =   string(basefilename) + "_mrloc.glt";
            
            starspan_create_strip(
                strip_band_type,
                strip_bands,
                prefix,
                mrbi_list,
                strip_filename,
                fid_filename,
                loc_filename
            );
			delete mrbi_list;
			mrbi_list = 0;
		}
        if ( ownOutVector && outVector ) {
            delete outVector;
        }
	}
};

Observer* starspan_getMiniRasterStripObserver(
	Traverser& tr,
	string basefilename,
	string shpfilename
) {	
	if ( !tr.getVector() ) {
		cerr<< "vector datasource expected\n";
		return 0;
	}

	int layernum = tr.getLayerNum();
	OGRLayer* layer = tr.getVector()->getLayer(layernum);
	if ( !layer ) {
		cerr<< "warning: No layer " <<layernum<< " found\n";
		return 0;
	}

    Vector* outVector = 0;
    OGRLayer* outLayer = 0;
    
    // <shp>
    if ( shpfilename.length() ) {
        // prepare outVector and outLayer:
        
        if ( globalOptions.verbose ) {
            cout<< "starspan_getMiniRasterStripObserver: starting creation of output vector " <<shpfilename<< " ...\n";
        }
    
        outVector = Vector::create(shpfilename.c_str()); 
        if ( !outVector ) {
            // errors should have been written
            return 0;
        }                
    
        // create layer in output vector:
        outLayer = starspan_createLayer(
            layer,
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
    
    
    
    string prefix = basefilename;
    prefix += "_TMP_PRFX_";
    
	MiniRasterStripObserver* obs = new MiniRasterStripObserver(basefilename, prefix.c_str());
    obs->setOutVectorDirectly(outVector, outLayer);
    
	return obs;
}




/**
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
) {	
    assert( mrbi_list );
    
	MiniRasterStripObserver* obs = new MiniRasterStripObserver(basefilename, prefix);
    
    // observer won't own output vector
    obs->setOutVector(outVector, outLayer); 
    
    // observer won't create strips
    obs->setCreateStrip(false);    
    
    // observer won't own the passed list
    obs->setMrbiList(mrbi_list);
    
	return obs;
}



////////////////////////////////////////////
/// geometry translation routines

static void translateLineString(OGRLineString* gg, double deltaX, double deltaY) {
    int numPoints = gg->getNumPoints();
    for ( int i = 0; i < numPoints; i++ ) {
        OGRPoint pp;
        gg->getPoint(i, &pp);
        pp.setX(pp.getX() + deltaX);
        pp.setY(pp.getY() + deltaY);
        gg->setPoint(i, &pp);
    }
}

/** does the translation */
static void traverseTranslate(OGRGeometry* geometry, double deltaX, double deltaY) {
	OGRwkbGeometryType type = geometry->getGeometryType();
	switch ( type ) {
		case wkbPoint:
		case wkbPoint25D: {
			OGRPoint* gg = (OGRPoint*) geometry;
            gg->setX(gg->getX() + deltaX);
            gg->setY(gg->getY() + deltaY);
        }
			break;
	
		case wkbLineString:
		case wkbLineString25D: {
			OGRLineString* gg = (OGRLineString*) geometry;
            translateLineString(gg, deltaX, deltaY);
        }
			break;
	
		case wkbPolygon:
		case wkbPolygon25D: {
			OGRPolygon* gg = (OGRPolygon*) geometry;
            OGRLinearRing *ring = gg->getExteriorRing();
            translateLineString(ring, deltaX, deltaY);
           	for ( int i = 0; i < gg->getNumInteriorRings(); i++ ) {
           		ring = gg->getInteriorRing(i);
           		translateLineString(ring, deltaX, deltaY);
           	}
        }
			break;
			
		case wkbGeometryCollection:
		case wkbGeometryCollection25D: 
		case wkbMultiPolygon: 
		case wkbMultiPolygon25D: 
		case wkbMultiPoint: 
		case wkbMultiPoint25D: 
		case wkbMultiLineString: 
		case wkbMultiLineString25D: 
        {
			OGRGeometryCollection* gg = (OGRGeometryCollection*) geometry;
           	for ( int i = 0; i < gg->getNumGeometries(); i++ ) {
           		OGRGeometry* geo = (OGRGeometry*) gg->getGeometryRef(i);
           		traverseTranslate(geo, deltaX, deltaY);
           	}
        }            
			break;
			
		default:
			cerr<< "***WARNING: mini_raster_strip: " <<OGRGeometryTypeToName(type)
                << ": geometry type not considered.\n"
			;
	}
}


/**
 * Translates the geometry such that it's located relative to (x0,y0).
 */
static void translateGeometry(OGRGeometry* geometry, double x0, double y0) {
    OGREnvelope bbox;
    geometry->getEnvelope(&bbox);
    
    //
    // FIXME: What follows is still rather a hack
    //
    
    double baseX = //x0 <= 0 ? bbox.MaxX : bbox.MinX;
                   bbox.MinX;
    double baseY = y0 <= 0 ? bbox.MaxY : bbox.MinY;
    
    // deltas to adjust all points in geometry:
    // first term: to move to origin;
    // second term: to move to requested (x0,y0) origin:
    double deltaX = -baseX +x0;
    double deltaY = -baseY +y0;

    traverseTranslate(geometry, deltaX, deltaY);    
}


