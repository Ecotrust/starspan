//
// Stats - basic stats calculator
// Carlos A. Rueda
// $Id: Stats.h,v 1.3 2006-03-24 17:10:49 perrygeo Exp $
//

#ifndef Stats_h
#define Stats_h

#include <vector>
#include <map>

using namespace std;


/** indices into stats.result arrays: */
enum {
	SUM,     // cumulation
	MIN,     // minimum
	MAX,     // maximum
	AVG,     // average
	
	// require second pass:
	
	VAR,     // sample variance
	STDEV,   // std deviation = sqrt(VAR)
	MODE,    // mode
	MEDIAN,  // median
	NULLS,   // number of nodata values
	
	TOT_RESULTS  // do not use
};


/**
  * Basic statistics calculator
  */
class Stats {
public:
	/** assign true to your desired stats before you call compute(). */
	bool include[TOT_RESULTS];
	
	/** If include[s] == true, then result[s] will contain the result
	  * for s after calling compute().
	  */
	double result[TOT_RESULTS];

	/** creates a stats object. All stats will be calculated by default */
	Stats() {
		for ( int i = 0; i < TOT_RESULTS; i++ ) {
			include[i] = true;
		}
	}
	
	/**
	  * compute those stats s where include[s] == true.
	  */
	void compute(vector<int>& values, int nodata); 
	
	/**
	  * compute those stats s where include[s] == true.
	  */
	void compute(vector<double>& values, double nodata); 
	
	/**
	  * Utility method to count the number of occurrences of each value.
	  * Results are updated in the given map.
	  */
	static void computeCounts(vector<int>& values, map<int,int>& count); 
	
};


#endif

