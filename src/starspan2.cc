//
// StarSpan project
// Carlos A. Rueda
// starspan2 - main program 
// $Id: starspan2.cc,v 1.53 2008-07-26 21:35:46 crueda Exp $
//

#include <geos/version.h>
#if GEOS_VERSION_MAJOR < 3
	#include <geos.h>
#else
	#include <geos/unload.h>
	using namespace geos::io;   // for Unload
#endif


#include "starspan.h"           
#include "traverser.h"           

#include <cstdlib>
#include <ctime>


#define DEFAULT_TABLE_SUFFIX                NULL
        //"_table.csv"

#define DEFAULT_SUMMARY_SUFFIX              NULL              
        //"_summary.csv"

#define DEFAULT_CLASS_SUMMARY_SUFFIX        NULL
       //"_classsummary.csv"

#define DEFAULT_MINIRASTER_SUFFIX           "_mr"
                                            
#define DEFAULT_MRST_IMG_SUFFIX             "_mrst.img"
#define DEFAULT_MRST_SHP_SUFFIX             "_mrst.shp"
#define DEFAULT_MRST_FID_SUFFIX             "_mrstfid.img"
#define DEFAULT_MRST_GLT_SUFFIX             "_mrstglt.img"
                                            
#define DEFAULT_STAT                         "avg"


static bool show_dev_options = false;

GlobalOptions globalOptions;

// prints a help message
static void usage(const char* msg) {
	if ( msg ) { 
		fprintf(stderr, "starspan: %s\n", msg); 
		fprintf(stderr, "Type `starspan --help' for help\n");
		exit(1);
	}
	// header:
	fprintf(stdout, 
		"\n"
		"CSTARS starspan %s (%s %s)\n"
		"\n"
		, STARSPAN_VERSION, __DATE__, __TIME__
	);
	
	// developer options:
	if ( show_dev_options ) {
		fprintf(stdout, 
		"      --dump_geometries <filename>\n"
		"      --jtstest <filename>\n"
		"      --ppoly \n"
		"      --srs <srs>\n"
		);
	}
	else {	
		// main body:
		fprintf(stdout, 
		"USAGE:\n"
		"  starspan <inputs/commands/options>...\n"
		"\n"
		"      --vector <filename>                         --raster <filenames> ... \n"
		"      --layer <layername>                         --mask <filenames> ...   \n"
		"\n"
		"      --out-prefix <string>                       --out-type <type>\n"
		"      --table-suffix <string>\n"
		"      --summary-suffix <string>                   --stats <stat> <stat> ...\n"
        "      --class-summary-suffix <string> \n"
		"      --mr-img-suffix <string>                    --mini_raster_parity <parity> \n"
		"      --mrst-img-suffix <string>                  --mrst-shp-suffix <string>\n"
		"      --mrst-fid-suffix <string>                  --mrst-glt-suffix <string>\n"
		"\n"
		"      --duplicate <mode> <mode> ...               --validate_inputs\n"
		"      --in                                        --separation <num-pixels> \n"
		"      --fields <field1> ... <fieldn>              --pixprop <minimum-pixel-proportion>\n"
		"      --sql <statement>                           --noColRow \n"
		"      --where <condition>                         --noXY\n"
		"      --dialect <string>                          --skip_invalid_polys\n"
		"      --fid <FID>                                 --nodata <value>\n"
		"      --buffer <distance> [<segments>]            --box <width> [<height>] \n"
		"      --RID {file | path | none}                  --delimiter <separator>\n"
		"      --progress [<value>]                        --show-fields \n"
		"      --report                                    --verbose \n"
		"      --elapsed_time                              --version\n"
		);
	}
	
	// footer:
	fprintf(stdout, 
		"\n"
		"Additional information at http://starspan.casil.ucdavis.edu\n"
		"\n"
	);
	
	exit(0);
}

// prints a help message
static void usage_string(string& msg) {
	const char* m = msg.c_str();
	usage(m);
}


static bool raster_added_to_traverser = false;
static void add_rasters_to_traverser(vector<const char*>& raster_filenames, Traverser& traversr) {
    if (!raster_added_to_traverser) {
        for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {    
            traversr.addRaster(new Raster(raster_filenames[i]));
        }
        raster_added_to_traverser = true;
        //cout<< "add_rasters_to_traverser: added " <<traversr.getNumRasters()<< " rasters.\n";
    }
}


///////////////////////////////////////////////////////////////
// main program
int main(int argc, char ** argv) {
    
	globalOptions.use_pixpolys = false;
	globalOptions.skip_invalid_polys = false;
	globalOptions.pix_prop = 0.5;
	globalOptions.FID = -1;
	globalOptions.verbose = false;
	globalOptions.progress = false;
	globalOptions.progress_perc = 1;
	globalOptions.noColRow = false;
	globalOptions.noXY = false;
	globalOptions.only_in_feature = false;
	globalOptions.RID = "file";
	globalOptions.report_summary = true;
	globalOptions.nodata = 0.0;
	globalOptions.bufferParams.given = false;
	globalOptions.bufferParams.quadrantSegments = "1";
	globalOptions.boxParams.given = false;
	globalOptions.mini_raster_parity = "";
	globalOptions.mini_raster_separation = 0;
	globalOptions.delimiter = ",";
    

	if ( use_grass(&argc, argv) ) {
		return starspan_grass(argc, argv);
	}
	
	if ( argc == 1 ) {
		usage(NULL);
	}
	
    
    const char* rasterize_suffix = NULL;
    RasterizeParams rasterizeParams;
	

	bool validateInputs = false;
    
	bool report_elapsed_time = false;
	bool do_report = false;
	bool show_fields = false;
    
    
    const char*  outprefix = NULL;
    string outtype;
    
    
	vector<const char*>* select_fields = NULL;
    
    const char*  table_suffix = DEFAULT_TABLE_SUFFIX;
	string  csv_name;
    
	const char*  summary_suffix = DEFAULT_SUMMARY_SUFFIX;
	vector<const char*> select_stats;
    
    const char*  class_summary_suffix = DEFAULT_CLASS_SUMMARY_SUFFIX;
    
    // ####.img will be appended to this
    const char*  miniraster_suffix = DEFAULT_MINIRASTER_SUFFIX;
	const char*  mini_srs = NULL;
    
    
	const char* mrst_img_suffix = DEFAULT_MRST_IMG_SUFFIX;
	const char* mrst_shp_suffix = DEFAULT_MRST_SHP_SUFFIX;
	const char* mrst_fid_suffix = DEFAULT_MRST_FID_SUFFIX;
	const char* mrst_glt_suffix = DEFAULT_MRST_GLT_SUFFIX;
    
	const char* jtstest_filename = NULL;
	
	const char* vector_filename = 0;
	const char* vector_layername = 0;
	int vector_layernum = 0;
	vector<const char*> raster_filenames;
	
    
	vector<const char*> mask_filenames;
    vector<const char*> *masks = 0;

	const char* dump_geometries_filename = NULL;
	
    
    
    
	// ---------------------------------------------------------------------
	// collect arguments
	//
	for ( int i = 1; i < argc; i++ ) {
		
		//
		// INPUTS:
		//
		if ( 0==strcmp("--vector", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--vector: which vector dataset?");
			if ( vector_filename )
				usage("--vector specified twice");
			vector_filename = argv[i];
		}
		
		else if ( 0==strcmp("--layer", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--layer: which layer within the vector dataset?");
			if ( vector_layername )
				usage("--layer specified twice");
			vector_layername = argv[i];
		}
		
		else if ( 0==strcmp("--raster", argv[i]) ) {
			while ( ++i < argc && argv[i][0] != '-' ) {
				const char* raster_filename = argv[i];
				raster_filenames.push_back(raster_filename);
			}
			if ( raster_filenames.size() == 0 )
				usage("--raster: missing argument(s)");
			if ( i < argc && argv[i][0] == '-' ) 
				--i;
		}
        
		else if ( 0==strcmp("--mask", argv[i]) ) {
			while ( ++i < argc && argv[i][0] != '-' ) {
				const char* mask_filename = argv[i];
				mask_filenames.push_back(mask_filename);
				// check for field indication:
				if ( mask_filename[0] == '@' ) {
					usage("--mask: @field specification not implemented");
				}
			}
			if ( mask_filenames.size() == 0 ) {
				usage("--mask: missing argument(s)");
            }
			if ( i < argc && argv[i][0] == '-' ) { 
				--i;
            }
		}
        
        
        else if ( 0==strcmp("--validate_inputs", argv[i]) ) {
            validateInputs = true;
        }
        
        
		else if ( 0==strcmp("--duplicate", argv[i]) ) {
			while ( ++i < argc && argv[i][0] != '-' ) {
				string dup_code = argv[i];
				if ( dup_code == "direction" ) {
					// need angle argument:
                    double dup_arg = 0;
					if ( ++i < argc && argv[i][0] != '-' ) {
						dup_arg = atof(argv[i]);
					}
					else {
						usage("direction: missing angle parameter");
					}
                    globalOptions.dupPixelModes.push_back(DupPixelMode(dup_code, dup_arg));
				}
				else if ( dup_code == "distance" ) {
                    globalOptions.dupPixelModes.push_back(DupPixelMode(dup_code));
				}
				else if ( dup_code == "ignore_nodata" ) {
                    string band_param;
					if ( ++i < argc && argv[i][0] != '-' ) {
						band_param = argv[i];
                        if ( band_param == "all_bands"
                        ||   band_param == "any_band"
                        ||   band_param == "band" ) {
                            // OK
                            if ( band_param == "band" ) {
                                // need band argument:
                                int band = 0;
                                if ( ++i < argc && argv[i][0] != '-' ) {
                                    band = atoi(argv[i]);
                                    globalOptions.dupPixelModes.push_back(DupPixelMode(dup_code, band_param, band));
                                }
                                else {
                                    usage("--duplicate: ignore_nodata band: missing band argument");
                                }
                            }
                            else {
                                globalOptions.dupPixelModes.push_back(DupPixelMode(dup_code, band_param));
                            }
                        }
                        else {
                            string msg = string("--duplicate: ignore_nodata: invalid parameter: ") +band_param;
                            usage_string(msg);
                        }
					}
					else {
						usage("--duplicate: ignore_nodata: missing parameter");
					}
				}
				else {
					string msg = string("--duplicate: unrecognized mode: ") +dup_code;
					usage_string(msg);
				}
			}
			if ( globalOptions.dupPixelModes.size() == 0 ) {
				usage("--duplicate: specify at least one mode");
			}
			
			if ( i < argc && argv[i][0] == '-' ) { 
				--i;
			}
		}
		
		
		else if ( 0==strcmp("--out-prefix", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--out-prefix: ?");
			outprefix = argv[i];
        }
        
		else if ( 0==strcmp("--out-type", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--out-type: ?");
			outtype = argv[i];
        }
        
        
		else if ( 0==strcmp("--table-suffix", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--table-suffix: ?");
            table_suffix = argv[i];
		}
		
		else if ( 0==strcmp("--summary-suffix", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--summary-suffix: ?");
			summary_suffix = argv[i];
		}
		else if ( 0==strcmp("--stats", argv[i]) ) {
			while ( ++i < argc && argv[i][0] != '-' ) {
				select_stats.push_back(argv[i]);
			}
			if ( select_stats.size() == 0 )
				usage("--stats: ?");
			if ( i < argc && argv[i][0] == '-' ) 
				--i;
		}

		else if ( 0==strcmp("--class-summary-suffix", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--class-summary-suffix: ?");
			class_summary_suffix = argv[i];
		}

        
        // minirasters
		else if ( 0==strcmp("--mr-img-suffix", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--mr-img-suffix: ?");
            miniraster_suffix = argv[i];
		}
		
        
        // miniraster strip
		else if ( 0==strcmp("--mrst-img-suffix", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--mrst-img-suffix: ?");
            mrst_img_suffix = argv[i];
		}
		else if ( 0==strcmp("--mrst-shp-suffix", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--mrst-shp-suffix: ?");
            mrst_shp_suffix = argv[i];
		}
		else if ( 0==strcmp("--mrst-fid-suffix", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--mrst-fid-suffix: ?");
            mrst_fid_suffix = argv[i];
		}
		else if ( 0==strcmp("--mrst-glt-suffix", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--mrst-shp-suffix: ?");
            mrst_glt_suffix = argv[i];
		}
		
		
		else if ( 0==strcmp("--rasterize-suffix", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' ) {
				usage("--rasterize: suffix?");
            }
            rasterize_suffix = argv[i];
            // TODO: accept other parameters
		}
        
		
		else if ( 0==strcmp("--dump_geometries", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--dump_geometries: which name?");
			dump_geometries_filename = argv[i];
		}
		
		else if ( 0==strcmp("--jtstest", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--jtstest: which JTS test file name?");
			jtstest_filename = argv[i];
		}
		else if ( 0==strcmp("--report", argv[i]) ) {
			do_report = true;
		}
		else if ( 0==strcmp("--show-fields", argv[i]) ) {
			show_fields = true;
		}
		
		//
		// OPTIONS
		//                                                        
		else if ( 0==strcmp("--pixprop", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' ) {
				usage("--pixprop: pixel proportion?");
            }
			double pix_prop = atof(argv[i]);
			if ( pix_prop < 0.0 || pix_prop > 1.0 ) {
				usage("invalid pixel proportion");
            }
            globalOptions.pix_prop = pix_prop;
		}
		
		else if ( 0==strcmp("--nodata", argv[i]) ) {
			if ( ++i == argc || strncmp(argv[i], "--", 2) == 0 )
				usage("--nodata: value?");
			globalOptions.nodata = atof(argv[i]);
		}
		
		else if ( 0==strcmp("--buffer", argv[i]) ) {
			if ( ++i == argc || strncmp(argv[i], "--", 2) == 0 )
				usage("--buffer: distance?");
			globalOptions.bufferParams.distance = argv[i];
			if ( i+1 < argc && strncmp(argv[i+1], "--", 2) != 0 )
				globalOptions.bufferParams.quadrantSegments = argv[++i];
			globalOptions.bufferParams.given = true;				
		}
		
		else if ( 0==strcmp("--box", argv[i]) ) {
			if ( ++i == argc || strncmp(argv[i], "--", 2) == 0 ) {
				usage("--box: missing argument");
            }
            string bw = argv[i];
            string bh = bw;
            if ( 1 + i < argc && strncmp(argv[1 + i], "--", 2) != 0 ) {
                bh = argv[++i];
            }
            globalOptions.boxParams.width  = bw;
            globalOptions.boxParams.height = bh;
            globalOptions.boxParams.given = true;
		}
		
		else if ( 0==strcmp("--mini_raster_parity", argv[i]) ) {
			if ( ++i == argc || strncmp(argv[i], "--", 2) == 0 )
				usage("--mini_raster_parity: even, odd, or @<field>?");
			globalOptions.mini_raster_parity = argv[i];
		}
		
		else if ( 0==strcmp("--separation", argv[i]) ) {
			if ( ++i == argc || strncmp(argv[i], "--", 2) == 0 )
				usage("--separation: number of pixels?");
			globalOptions.mini_raster_separation = atoi(argv[i]);
			if ( globalOptions.mini_raster_separation < 0 )
				usage("--separation: invalid number of pixels");
		}
		
		else if ( 0==strcmp("--fid", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--fid: desired FID?");
			globalOptions.FID = atol(argv[i]);
			if ( globalOptions.FID < 0 )
				usage("invalid FID");
		}
		
		else if ( 0==strcmp("--sql", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--sql: missing statement");
			globalOptions.vSelParams.sql = argv[i];
		}
		
		else if ( 0==strcmp("--where", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--where: missing condition");
			globalOptions.vSelParams.where = argv[i];
		}
		
		else if ( 0==strcmp("--dialect", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--dialect: missing string");
			globalOptions.vSelParams.dialect = argv[i];
		}
		
		else if ( 0==strcmp("--noColRow", argv[i]) ) {
			globalOptions.noColRow = true;
		}
		
		else if ( 0==strcmp("--noXY", argv[i]) ) {
			globalOptions.noXY = true;
		}
		
		else if ( 0==strcmp("--ppoly", argv[i]) ) {
			globalOptions.use_pixpolys = true;
		}
		
		else if ( 0==strcmp("--skip_invalid_polys", argv[i]) ) {
			globalOptions.skip_invalid_polys = true;
		}
		
		else if ( 0==strcmp("--fields", argv[i]) ) {
			if ( !select_fields ) {
				select_fields = new vector<const char*>();
			}
			// the special name "none" will indicate no fields at all:
			bool none = false;
			while ( ++i < argc && argv[i][0] != '-' ) {
				const char* str = argv[i];
				none = none || 0==strcmp(str, "none");
				if ( !none ) {
					select_fields->push_back(str);
				}
			}
			if ( none ) {
				select_fields->clear();
			}
			if ( i < argc && argv[i][0] == '-' ) 
				--i;
		}

		else if ( 0==strcmp("--in", argv[i]) ) {
			globalOptions.only_in_feature = true;
		}
		
		else if ( 0==strcmp("--RID", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' ) {
				usage("--RID: what raster ID?");
			}
			globalOptions.RID = argv[i];
			if ( globalOptions.RID != "file"
			&&   globalOptions.RID != "path"
			&&   globalOptions.RID != "none" ) {
				usage("--RID: expecting one of: file, path, none");
			}
		}
		
		else if ( 0==strcmp("--delimiter", argv[i]) ) {
			if ( ++i == argc || strncmp(argv[i], "--", 2) == 0 ) {
				usage("--delimiter: what separator?");
			}
			globalOptions.delimiter = argv[i];
		}
		
		else if ( 0==strcmp("--progress", argv[i]) ) {
			if ( i+1 < argc && argv[i+1][0] != '-' )
				globalOptions.progress_perc = atof(argv[++i]);
			globalOptions.progress = true;
		}
		
		else if ( 0==strcmp("--verbose", argv[i]) ) {
			globalOptions.verbose = true;
			report_elapsed_time = true;
		}
		
		else if ( 0==strcmp("--elapsed_time", argv[i]) ) {
			report_elapsed_time = true;
		}
		
		else if ( 0==strcmp("--srs", argv[i]) ) {
			if ( ++i == argc || argv[i][0] == '-' )
				usage("--srs: which srs?");
			mini_srs = argv[i];
		}

		// HELP: --help or --help-dev
		else if ( 0==strncmp("--help", argv[i], 6) ) {
			show_dev_options = 0==strcmp("--help-dev", argv[i]);
			usage(NULL);
		}
		// version
		else if ( 0==strcmp("--version", argv[i]) ) {
			fprintf(stdout, "starspan %s\n", STARSPAN_VERSION);
			exit(0);
		}
		else {
            string msg = string("invalid argument: ") +argv[i];
            usage_string(msg);
		}
	}
	// ---------------------------------------------------------------------


    
    if ( globalOptions.bufferParams.given && globalOptions.boxParams.given ) {
        usage("Only one of --buffer/--box may be given");
    }
    
    if ( globalOptions.bufferParams.given ) {
        if ( globalOptions.verbose ) {
            cout<< "Using buffer parameters:"
                << "\n\tdistance = " <<globalOptions.bufferParams.distance
                << "\n\tquadrantSegments = " <<globalOptions.bufferParams.quadrantSegments
                <<endl;
            ;
        }
    }
        
    if ( globalOptions.boxParams.given ) {
        if ( globalOptions.verbose ) {
            cout<< "Given box parameters:"
                << " width = " <<globalOptions.boxParams.width
                << ", height = " <<globalOptions.boxParams.height
                <<endl;
            ;
        }
    }
    
	time_t time_start = time(NULL);
	
	// module initialization
	Raster::init();
	Vector::init();

	int res = 0;
	
	Vector* vect = 0;
	
    // the traverser object	
    Traverser traversr;

    
	// preliminary checks:
	
	if ( vector_filename ) {
		vect = Vector::open(vector_filename);
		if ( !vect ) {
			fprintf(stderr, "Cannot open %s\n", vector_filename);
			res = 1;
			goto end;
		}  

        if ( vector_layername && globalOptions.vSelParams.sql.length() > 0 ) {
            cout<< "Warning: --layer option ignored when --sql option is given" <<endl;
        }
        
		// 
		// get the layer number for the given layer name 
		// if not specified or there is only a single layer, use the first layer 
		//
		if ( vector_layername && vect->getLayerCount() != 1 ) { 
			for ( unsigned i = 0; i < vect->getLayerCount(); i++ ) {    
				if ( 0 == strcmp(vector_layername, 
						 vect->getLayer(i)->GetLayerDefn()->GetName()) ) {
					vector_layernum = i;
				}
			}
		}
		else {
		       vector_layernum = 0; 
		}
        
        traversr.setVector(vect);
        traversr.setLayerNum(vector_layernum);
	}

	
    if ( mask_filenames.size() > 0 ) {
        size_t noPairs = min(raster_filenames.size(), mask_filenames.size());
        raster_filenames.resize(noPairs);
        mask_filenames.resize(noPairs);
        masks = &mask_filenames;
        
        if ( globalOptions.verbose ) {
            cout<< "raster-mask pairs given:" << endl;
            for ( size_t i = 0; i < noPairs; i++ ) {
                cout<< "\t" <<raster_filenames[i]<< endl
                    << "\t" <<mask_filenames[i]<< endl<<endl;
            }
        }
    }
    
    if ( validateInputs 
    && starspan_validate_input_elements(vect, vector_layernum, raster_filenames, masks) ) {
        usage("invalid inputs; check error messages");
    }        
    

	if ( globalOptions.dupPixelModes.size() > 0 ) {
		if ( globalOptions.verbose ) {
			cout<< "--duplicate modes given:" << endl;
			for (int k = 0, count = globalOptions.dupPixelModes.size(); k < count; k++ ) {
				cout<< "\t" <<globalOptions.dupPixelModes[k].toString() << endl;
			}
		}
	}
	
    

    //    
    // dispatch simpler commands:
	//
    
	if ( do_report ) {
		bool done = vect || raster_filenames.size() > 0;
		if ( vect ) {
			starspan_report_vector(vect);
		}
		for ( unsigned i = 0; i < raster_filenames.size(); i++ ) {
			Raster* rast = new Raster(raster_filenames[i]);
			starspan_report_raster(rast);
			delete rast;
		}

		if ( !done ) {
			usage("--report: Please give at least one input file to report\n");
		}
        
        goto end;
	}
    
    if ( show_fields ) {
        if ( !vect ) {
            usage("--show-fields: provide the vector datasource\n");
        }
        vect->showFields(stdout);
        
        goto end;
    }
    
    

    if ( globalOptions.FID >= 0 ) {
        traversr.setDesiredFID(globalOptions.FID);
    }
    
    if ( globalOptions.progress ) {
        traversr.setProgress(globalOptions.progress_perc, cout);
    }
    
    
    // some observers with minimum check of requirements

    if ( dump_geometries_filename ) {
        add_rasters_to_traverser(raster_filenames, traversr);
        
        if ( traversr.getNumRasters() == 0 && traversr.getVector() && globalOptions.FID >= 0 ) {
            dumpFeature(traversr.getVector(), globalOptions.FID, dump_geometries_filename);
        }
        else {
            Observer* obs = starspan_dump(traversr, dump_geometries_filename);
            if ( obs ) {
                traversr.addObserver(obs);
            }
        }
    }

    if ( jtstest_filename ) {
        add_rasters_to_traverser(raster_filenames, traversr);
        
        Observer* obs = starspan_jtstest(traversr, jtstest_filename);
        if ( obs ) {
            traversr.addObserver(obs);
        }
    }

    //
    // commands with more strict checking of inputs:
    //
    
    if ( !outprefix ) {
        usage("--out-prefix: ?");
    }
    
    if ( !outtype.length() ) {
        usage("--out-type: ?");
    }
    
    if ( outtype != "table" 
    &&   outtype != "mini_raster_strip" 
    &&   outtype != "mini_rasters"
    &&   outtype != "rasterization"      ) {
        // hmm, perhaps is one of the more internal commands:
        if ( traversr.getNumObservers() > 0 ) {
            // OK, continue with the traversal.
            // See above for observers created by starspan_dump or starspan_jtstest.
        }
        else {
            //  complain about not receiving a typical command:
            string msg = string("--out-type: unrecognized type: ") +outtype;
            usage_string(msg);
        }
    }
    

    
    //
    // Output: Table
    //
    if ( outtype == "table" ) {
		if ( !vect ) {
			usage("--out-type table expects a vector input (use --vector)");
		}

        if ( raster_filenames.size() == 0 && globalOptions.dupPixelModes.size() == 0 ) {
            usage("--out-type table expects at least a raster input (use --raster)");
        }

        if ( table_suffix ) {
            csv_name = string(outprefix) + table_suffix;
            if ( globalOptions.dupPixelModes.size() > 0 ) {
                res = starspan_csv2(
                    vect,
                    raster_filenames,
                    masks,
                    select_fields,
                    vector_layernum,
                    globalOptions.dupPixelModes,
                    csv_name.c_str()
                );
            }
            else if ( raster_filenames.size() > 0 ) {
                res = starspan_csv(
                    vect,  
                    raster_filenames,
                    select_fields, 
                    csv_name.c_str(),
                    vector_layernum
                );
            }
        }
        
        //
        // summaries:
        //
        
        if ( summary_suffix ) {
            string stats_name = string(outprefix) + summary_suffix;
            
            if ( select_stats.size() == 0 ) {
                select_stats.push_back(DEFAULT_STAT);
            }
            
            res = starspan_stats(
                vect,  
                raster_filenames,     
                select_stats,
                select_fields, 
                stats_name.c_str(),
                vector_layernum
            );
        }
        
        if ( class_summary_suffix ) {
            add_rasters_to_traverser(raster_filenames, traversr);
            
            string count_by_class_name = string(outprefix) + class_summary_suffix;
            Observer* obs = starspan_getCountByClassObserver(traversr, count_by_class_name.c_str());
            if ( obs ) {
                traversr.addObserver(obs);
            }
        }
    }
    
    else if ( outtype == "mini_raster_strip" ) {
        string mrst_img_filename = string(outprefix) + mrst_img_suffix;
        string mrst_shp_filename = string(outprefix) + mrst_shp_suffix;
        string mrst_fid_filename = string(outprefix) + mrst_fid_suffix;
        string mrst_glt_filename = string(outprefix) + mrst_glt_suffix;
        
        if ( globalOptions.dupPixelModes.size() > 0 ) {
            res = starspan_minirasterstrip2(
                vect,  
                raster_filenames,
                masks,
                select_fields, 
                vector_layernum,
                globalOptions.dupPixelModes,
                mrst_img_filename, 
                mrst_shp_filename,
                mrst_fid_filename,
                mrst_glt_filename
            );
        }
        
        else {
            add_rasters_to_traverser(raster_filenames, traversr);
            
			Observer* obs = starspan_getMiniRasterStripObserver(
                traversr, mrst_img_filename.c_str(), mrst_shp_filename.c_str()
            );
			if ( obs ) {
				traversr.addObserver(obs);
            }
		}
    }
    
    else if ( outtype == "mini_rasters" ) {
        string mini_prefix = string(outprefix) + miniraster_suffix;
        
        if ( globalOptions.dupPixelModes.size() > 0 ) {
            res = starspan_miniraster2(
                vect,  
                raster_filenames,
                masks,
                select_fields, 
                vector_layernum,
                globalOptions.dupPixelModes,
                mini_prefix.c_str(),
                mini_srs
            );
        }
        
		else {
            add_rasters_to_traverser(raster_filenames, traversr);
			Observer* obs = starspan_getMiniRasterObserver(mini_prefix.c_str(), mini_srs);
			if ( obs ) {
				traversr.addObserver(obs);
            }
		}
        
    }
    
    else if ( outtype == "rasterization" ) {
        add_rasters_to_traverser(raster_filenames, traversr);
        
        rasterizeParams.outRaster_filename = (string(outprefix) + rasterize_suffix).c_str();
        rasterizeParams.fillNoData = true;
        
        GDALDataset* ds = traversr.getRaster(0)->getDataset();
        rasterizeParams.projection = ds->GetProjectionRef();
        ds->GetGeoTransform(rasterizeParams.geoTransform);
        Observer* obs = starspan_getRasterizeObserver(&rasterizeParams);
        if ( obs ) {
            traversr.addObserver(obs);
        }
    }
    
    
    
    /////////////////////////////////////////////////////////////
    // now, do the traversal for the observer-based processing:
    //
    
    if ( traversr.getNumObservers() > 0 ) {
        // let's get to work!	
        traversr.traverse();

        if ( globalOptions.report_summary ) {
            traversr.reportSummary();
        }
        
        // release observers:
        traversr.releaseObservers();
    }
    
    // release data input objects:
    // Note that we get the rasters from the traverser, while
    // the single vector is directly deleted below:
    for ( int i = 0; i < traversr.getNumRasters(); i++ ) {
        delete traversr.getRaster(i);
    }
	
end:
	if ( vect ) {
		delete vect;
	}
	
	Vector::end();
	Raster::end();
	
	// more final cleanup:
	Unload::Release();	
	
	if ( report_elapsed_time ) {
		cout<< "Elapsed time: ";
		// report elapsed time:
		time_t secs = time(NULL) - time_start;
		if ( secs < 60 )
			cout<< secs << "s\n";
		else {
			time_t mins = secs / 60; 
			secs = secs % 60; 
			if ( mins < 60 ) 
				cout<< mins << "m:" << secs << "s\n";
			else {
				time_t hours = mins / 60; 
				mins = mins % 60; 
				if ( hours < 24 ) 
					cout<< hours << "h:" << mins << "m:" << secs << "s\n";
				else {
					time_t days = hours / 24; 
					hours = hours % 24; 
					cout<< days << "d:" <<hours << "h:" << mins << "m:" << secs << "s\n";
				}
			}
		}
	}

	return res;
}

