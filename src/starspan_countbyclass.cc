//
// STARSpan project
// Carlos A. Rueda
// starspan_countbyclass - count by class
// $Id: starspan_countbyclass.cc,v 1.7 2008-04-11 19:15:12 crueda Exp $
//

#include "starspan.h"           
#include "traverser.h"       
#include "Stats.h"       
#include "Csv.h"       

#include <iostream>
#include <cstdlib>
#include <math.h>
#include <cassert>

using namespace std;

// my prefix for verbose output:
static const char* vprefix = "  [count-by-class]";

/**
  * Creates fields and populates the table.
  */
class CountByClassObserver : public Observer {
public:
	Traverser& tr;
	GlobalInfo* global_info;
	Vector* vect;
	FILE* outfile;
	bool OK;
		
	CsvOutput csvOut;

	/**
	  * Creates a counter by class.
	  */
	CountByClassObserver(Traverser& tr, FILE* f) : tr(tr), outfile(f) {
		vect = tr.getVector();
		global_info = 0;
		OK = false;
		assert(outfile);
	}
	
	/**
	  * simply calls end()
	  */
	~CountByClassObserver() {
		end();
	}
	
	/**
	  * closes the file
	  */
	void end() {
		if ( outfile ) {
			fclose(outfile);
			cout<< "CountByClass: finished" << endl;
			outfile = 0;
		}
	}


	/**
	  * returns true. We ask traverser to get values for
	  * visited pixels in each feature.
	  */
	bool isSimple() { 
		return true; 
	}

	/**
	  * Creates first line with column headers:
	  *    FID, class, count
	  */
	void init(GlobalInfo& info) {
		global_info = &info;

		if ( globalOptions.verbose ) {
			cout<< vprefix<< " init\n";
		}

		OK = false;   // but let's see ...
		
                int layernum = tr.getLayerNum();
		OGRLayer* poLayer = vect->getLayer(layernum);
		if ( !poLayer ) {
			cerr<< "Couldn't fetch layer" << endl;
			return;
		}
		
		const unsigned num_bands = global_info->bands.size();
		
		if ( num_bands > 1 ) {
			cerr<< "CountByClass: warning: multiband raster data;" <<endl;
			cerr<< "              Only the first band will be processed." <<endl;
		}
		else if ( num_bands == 0 ) {
			cerr<< "CountByClass: warning: no bands in raster data;" <<endl;
			return;
		}

		// check first band is of integral type:
		GDALDataType bandType = global_info->bands[0]->GetRasterDataType();
		if ( bandType == GDT_Float64 || bandType == GDT_Float32 ) {
			cerr<< "CountByClass: warning: first band in raster data is not of integral type" <<endl;
			return;
		}

		csvOut.setFile(outfile);
		csvOut.setSeparator(globalOptions.delimiter);
		csvOut.startLine();

		//		
		// write column headers:
		//
		csvOut.addString("FID").addString("class").addString("count");
		csvOut.endLine();
		//fprintf(outfile, "FID,class,count\n");
		
		// now, all seems OK to continue processing		
		OK = true;
	}
	
	/**
	  * gets the counts and writes news records accordingly.
	  */
	void intersectionEnd(IntersectionInfo& intersInfo) {
		if ( !OK )
			return;

		const long FID = intersInfo.feature->GetFID();
		
		// get values of pixels corresponding to first band:		
		vector<int> values;
		tr.getPixelIntegerValuesInBand(1, values);
		
		// get the counts:
		map<int,int> counters;
		Stats::computeCounts(values, counters);

		// report the counts:
		if ( globalOptions.verbose ) {
			cout<< vprefix<< " FID=" <<FID<< " pixels=" <<tr.getPixelSetSize()<< ":\n";
		}
		for ( map<int, int>::iterator it = counters.begin(); it != counters.end(); it++ ) {
			const int class_ = it->first;
			const int count = it->second;
			
			// add record to outfile:
			csvOut.startLine();
			csvOut.addField("%ld", FID).addField("%d", class_).addField("%d", count);
			csvOut.endLine();
			//fprintf(outfile, "%ld,%d,%d\n", FID, class_, count);

			if ( globalOptions.verbose ) {
				cout<< vprefix<< "   class=" <<class_<< " count=" <<count<< endl;
			}
		}
		fflush(outfile);
	}
};



/**
  * starspan_getCountByClassObserver: implementation
  */
Observer* starspan_getCountByClassObserver(
	Traverser& tr,
	const char* filename
) {
	// create output file
	FILE* outfile = fopen(filename, "w");
	if ( !outfile ) {
		cerr<< "Couldn't create "<< filename << endl;
		return 0;
	}

	return new CountByClassObserver(tr, outfile);	
}
		


