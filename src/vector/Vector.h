/*
	vector - vector interface
	$Id: Vector.h,v 1.5 2008-04-10 20:26:37 crueda Exp $
*/
#ifndef Vector_h
#define Vector_h

#include "ogrsf_frmts.h"
#include "cpl_conv.h"
#include "cpl_string.h"

#include <stdio.h>  // FILE

/**
 * Represents a vector dataset.
 * This is implemented on top of the OGR library.
 */
class Vector {
public:
	/** initializes this module */
	static int init(void);
	
	/** finishes this module */
	static int end(void);
	
	/**
     * Creates a vector object representing an existing file. 
	 * Returns null if error 
     */
	static Vector* open(const char* filename);
	
	/** destroys this vector. */
	~Vector();
	
	/** gets the name of this object */
	const char* getName();
	
	/** gets the number of layers */
	int getLayerCount(void) { return poDS->GetLayerCount(); } 
	
	/** gets a layer */
	OGRLayer* getLayer(int layer_num); 
	
	/** general report */
	void report(FILE* file);
	
	/** report */
	void showFields(FILE* file);
	
	/** gets the OGRDataSource */
	OGRDataSource* getDataSource(void) { return poDS; } 
	
	/** 
     * Creates a vector object representing an new datasource.
     * Currently, the only format for created datasources is shapefile.
	 * Returns null if error 
     */
	static Vector* create(const char* filename);
	
private:
	Vector(OGRDataSource* ds);
	OGRDataSource* poDS;
};

#endif

