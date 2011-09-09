//
// StarSpan project
// Carlos A. Rueda
// starspan_grass GRASS interface
// $Id: starspan_grass.cc,v 1.16 2008-05-09 00:50:36 crueda Exp $
//
/*
    This is a GRASS interface to some of the available StarSpan commands.
    In this interface, the command must be the first argument in the invocation
    of the program, eg:
       starspan cmd=csv vector=myvector rasters=myraster output=myoutput.csv
    
    The dispatching routine, starspan_grass(), prepares the passed arguments so
    the GRASS library is used on a per command basis. For example, to get
    the help message for the "csv" command, the user will type:
       starspan csv help
    
    GRASS datasources are opened by using the corresponding GDAL/OGR drivers,
    that is, not by using the GRASS structures. This is currently the approach
    to minimize changes to the base code. Note also that non-GRASS datasources
    can still be opened: this module first tries to open a datasource assuming
    it's a GRASS map; if the map cannot be found, then it's opened as a regular
    dataset.
    
*/

#include "starspan.h"


///////////////////////////////////////////////////////////////////////////////
// With GRASS
#ifdef HAVE_LIBGRASS_GIS

extern "C" {
#include "grass/gis.h"
#include "grass/Vect.h"
#include "grass/glocale.h"
}

#include <geos/version.h>
#if GEOS_VERSION_MAJOR < 3
	#include <geos.h>
#else
	#include <geos/unload.h>
	using namespace geos::io;   // for Unload
#endif

#include <cassert>


// utility to prepare arguments
static void shift_args(int *argc, char ** argv) {
    for ( int i = 2; i < *argc; i++ ) {
        argv[i - 1] = argv[i];
    }
    (*argc)--;
}


bool use_grass(int *argc, char ** argv) {
    if ( NULL == getenv("GISBASE") ) {
        return false;
    }
    
    if ( *argc > 1 && strcmp("S", argv[1]) == 0 ) {
        shift_args(argc, argv);
        return false;
    }
    
    return true;
    
}


// utility to get the module description for either the main program
// or a specific command.
static char* get_module_description(const char* cmd) {
    static char module_description[5*1024];
    sprintf(module_description,
        "\n"
        "CSTARS StarSpan %s (%s %s)\n"
        "GRASS interface"
        ,
        VERSION, __DATE__, __TIME__ 
    );
    if ( cmd ) {
        strcat(module_description, " - Command: ");
        strcat(module_description, cmd);
    }
    strcat(module_description, "\n");
    return module_description;
}

// GRASS initialization for a specific command
static void init(const char* cmd) {
	char long_prgname[1024];
    
	struct GModule *module;
    
	sprintf(long_prgname, "%s %s (%s %s)", "starspan", VERSION, __DATE__, __TIME__);
	G_gisinit(long_prgname);
	
	module = G_define_module();
	module->description = get_module_description(cmd);
}


/////////////////////////////
// the command

static struct Option *opt_cmd;


/////////////////////////////
// Inputs and general options

static struct Option *opt_mask_input;
static struct Option *opt_fields;
static struct Option *opt_duplicate_pixel;
static struct Option *opt_layer_name;
static struct Option *opt_delimiter;


////////////////////////////////////////////////////
// Results of processing some of the general options
static Vector* vect = 0;
static vector<const char*>* select_fields = 0;
static int vector_layernum = 0;
static vector<const char*>* mask_filenames = 0;    


// gets the list of selected fields, if any
static void process_opt_fields(void) {
    select_fields = 0;
    if ( opt_fields && opt_fields->answers ) {
        select_fields = new vector<const char*>();
        // the special name "none" will indicate not fields at all:
        bool none = false;
        for ( int n = 0; opt_fields->answers[n] != NULL; n++ ) {
            const char* str = opt_fields->answers[n];
            none = none || 0==strcmp(str, "none");
            if ( !none ) {
                select_fields->push_back(str);
            }
        }
        if ( none ) {
            select_fields->clear();
        }
    }
}
     

/** Opens a vector file if name given */
static Vector* open_vector(const char* filename) {
    if ( !filename ) {
        return 0;
    }
    
    Vector* vect;
    
    // first, try to open as a GRASS map:
    char *mapset;
    if ( (mapset = G_find_vector2(filename, "") ) != NULL ) {
        // filename may still end with @mapset, so remove that part:
        char name[2048];
        strcpy(name, filename);
        char* ptr = strrchr(name, '@'); 
        if ( ptr ) {
            *ptr = 0;
        }
        // get full namespace according to http://gdal.org/ogr/drv_grass.html
        string fn = string(G_gisdbase())
                        + "/" + G_location()
                        + "/" + mapset
                        + "/vector"
                        + "/" + name
                        + "/head";
        
        vect = Vector::open(fn.c_str());
    }
    else {
        // try to open using GDAL directly:
        vect = Vector::open(filename);
    }
    
    if ( !vect ) {
        G_fatal_error(_("Vector map <%s> not found"), filename);
    }
    return vect;
}


// 
// get the layer number for the given layer name 
// if not specified or there is only a single layer, use the first layer 
//
static void process_opt_layer_name(void) {
    vector_layernum = 0;
    if ( vect && opt_layer_name && opt_layer_name->answer && vect->getLayerCount() != 1 ) { 
        for ( unsigned i = 0; i < vect->getLayerCount(); i++ ) {    
            if ( 0 == strcmp(opt_layer_name->answer, 
                     vect->getLayer(i)->GetLayerDefn()->GetName()) ) {
                vector_layernum = i;
                break;
            }
        }
    }
}



// gets the list of masks, if any
static void process_opt_masks(void) {
    mask_filenames = 0;    
    if ( opt_mask_input && opt_mask_input->answers ) {
        mask_filenames = new vector<const char*>();
        for ( int n = 0; opt_mask_input->answers[n] != NULL; n++ ) {
            mask_filenames->push_back(opt_mask_input->answers[n]);
        }
    }
}

// sets the list of duplicate_pixel modes
static void process_opt_duplicate_pixel(void) {
    if ( !opt_duplicate_pixel || !opt_duplicate_pixel->answers ) {
        return;
    }
    for ( int n = 0; opt_duplicate_pixel->answers[n] != NULL; n++ ) {
        char* mode = opt_duplicate_pixel->answers[n];
        char dup_code[256];
        float dup_arg = -1;
        sscanf(mode, "%[^ ] %f", dup_code, &dup_arg);
        if ( 0 == strcmp(dup_code, "direction") ) {
            if ( dup_arg < 0 ) {
                G_fatal_error("duplicate_pixel: direction: missing angle parameter");
            }
            globalOptions.dupPixelModes.push_back(DupPixelMode(dup_code, dup_arg));
        }
        else if ( 0 == strcmp(dup_code, "distance") ) {
            // OK.
            globalOptions.dupPixelModes.push_back(DupPixelMode(dup_code));
        }
        else {
            G_fatal_error("duplicate_pixel: unrecognized mode: %s\n", dup_code);
        }
    }
    if ( globalOptions.verbose ) {
        cout<< "duplicate_pixel modes given:" << endl;
        for (int k = 0, count = globalOptions.dupPixelModes.size(); k < count; k++ ) {
            cout<< "\t" <<globalOptions.dupPixelModes[k].toString() << endl;
        }
    }
}


// Calls G_parser and processes some general options:
static int call_parser(int argc, char ** argv) {
	if ( G_parser(argc, argv) ) {
		return EXIT_FAILURE;
    }
    
    globalOptions.verbose = G_verbose();
    process_opt_fields();
    process_opt_duplicate_pixel();
    process_opt_masks();

    if ( opt_delimiter && opt_delimiter->answer ) {
        globalOptions.delimiter = opt_delimiter->answer;
    }

    return 0;
}



static Option *define_string_option(const char* key, const char* desc, int required) {
	Option *option = G_define_option() ;
	option->key        = (char*) key;
	option->type       = TYPE_STRING;
	option->required   = required;
	option->description= (char*) desc;
    return option;
}


// Defines the command option, which is required and has to be
// equal to the given value. Note that this option is used to
// determine which command is to be run, especially when the program is
// launched from the GRASS GUI.
static void define_cmd_option(const char* cmd) {
    static char *answers[] = { (char*) cmd };
    static char desc[1024];
    sprintf(desc, "Command, only valid argument: %s", cmd);
	opt_cmd = define_string_option("cmd",  desc, YES);
    opt_cmd->answer = (char*) cmd;
    opt_cmd->options = (char*) cmd;
    opt_cmd->answers = answers; 
}



/** Opens a raster file if name given */
static Raster* open_raster(const char* filename) {
    if ( !filename ) {
        return 0;
    }
    
    Raster* rast;
    
    // first, try to open as a GRASS map:
    char *mapset;
    if ( (mapset = G_find_cell2(filename, "") ) != NULL ) {
        // get full namespace according to http://gdal.org/frmt_grass.html
        string fn = string(G_gisdbase())
                        + "/" + G_location()
                        + "/" + mapset
                        + "/cellhd"
                        + "/" + filename;
        
        rast = Raster::open(fn.c_str());
    }
    else {
        // try to open using GDAL directly:
        rast = Raster::open(filename);
    }
    
    if ( !rast ) {
        G_fatal_error(_("Raster map <%s> not found"), filename);
    }
    return rast;
}






///////////////////////////////////////////////////////////////////////////////
// Command: report
static int command_report(int argc, char ** argv) {
	define_cmd_option("report");
    
	struct Option* opt_vector_input = define_string_option("vector",  "Vector map", NO);
	opt_vector_input->gisprompt = (char*) "old,vector,vector";
	struct Option* opt_raster_input = define_string_option("rasters", "Raster map(s)", NO);
    opt_raster_input->multiple = YES;
    opt_raster_input->gisprompt = (char*) "old,cell,raster";

	if ( call_parser(argc, argv) ) {
		return EXIT_FAILURE;
    }
    
    if ( !opt_vector_input->answer && !opt_raster_input->answers ) {
        G_fatal_error("report: Please give at least one input map to report\n");
    }
    
	Vector* vect = open_vector(opt_vector_input->answer);
    if ( vect ) {
        starspan_report_vector(vect);
        delete vect;
    }

    if ( opt_raster_input->answers ) {
        for ( int n = 0; opt_raster_input->answers[n] != NULL; n++ ) {
            Raster* rast = open_raster(opt_raster_input->answers[n]);
            if ( rast ) {
                starspan_report_raster(rast);
                delete rast;
            }
        }
    }
    
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// Command: csv
static int command_csv(int argc, char ** argv) {
    define_cmd_option("csv");
    
	struct Option *opt_vector_input = define_string_option("vector",  "Vector map", YES);
	opt_vector_input->gisprompt = (char*) "old,vector,vector";
    
	struct Option *opt_raster_input = define_string_option("rasters", "Raster map(s)", YES);
    opt_raster_input->multiple = YES;
    opt_raster_input->gisprompt = (char*) "old,cell,raster";
    
	opt_mask_input = define_string_option("masks", "Raster mask(s)", NO);
    opt_mask_input->multiple = YES;
    opt_mask_input->gisprompt = (char*) "old,cell,raster";
    
    struct Option *opt_output = define_string_option("output",  "Output filename", YES);
    
    opt_fields = define_string_option("fields",  "Desired fields", NO);
    opt_fields->multiple = YES;
    
    opt_duplicate_pixel = define_string_option("duplicate_pixel",  "Duplicate pixel modes", NO);
    opt_duplicate_pixel->multiple = YES;
    
    opt_layer_name = define_string_option("layer",  "Layer within vector dataset", NO);
    
    opt_delimiter = define_string_option("delimiter",  "Separator", NO);
    opt_delimiter->answer = (char*) ",";

    
	if ( call_parser(argc, argv) ) {
		return EXIT_FAILURE;
    }
    

    const char* csv_name = opt_output->answer;
    
	Vector* vect = open_vector(opt_vector_input->answer);
    if ( !vect ) {
        return EXIT_FAILURE;
    }
    
    process_opt_layer_name();


    vector<const char*> raster_filenames;    
    for ( int n = 0; opt_raster_input->answers[n] != NULL; n++ ) {
        raster_filenames.push_back(opt_raster_input->answers[n]);
    }

    if ( mask_filenames ) {
        if ( raster_filenames.size() != mask_filenames->size() ) {
            G_fatal_error(_("Different number of rasters and masks"));
        }
        
        if ( globalOptions.verbose ) {
            cout<< "raster-mask pairs given:" << endl;
            for ( size_t i = 0; i < raster_filenames.size(); i++ ) {
                cout<< "\t" <<raster_filenames[i]<< endl
                    << "\t" <<(*mask_filenames)[i]<< endl<<endl;
            }
        }
    }


    int res;
    if ( globalOptions.dupPixelModes.size() > 0 ) {
        res = starspan_csv2(
            vect,
            raster_filenames,
            mask_filenames,
            select_fields,
            vector_layernum,
            globalOptions.dupPixelModes,
            csv_name
        );
    }
    else {    
        int res = starspan_csv(
            vect,  
            raster_filenames,
            select_fields, 
            csv_name,
            vector_layernum
        );
    }
    
    delete vect;
    
    if ( select_fields ) {
        delete select_fields;
    }

    if ( mask_filenames ) {
        delete mask_filenames;
    }
    
    return res;
}


///////////////////////////////////////////////////////////////////////////////
// Command: rasterize  (note: preliminary for basic testing)
static int command_rasterize(int argc, char ** argv) {
    define_cmd_option("rasterize");
    
	struct Option *opt_vector_input = define_string_option("vector",  "Vector map", YES);
	opt_vector_input->gisprompt = (char*) "old,vector,vector";
    
	struct Option *opt_raster_input = define_string_option("raster", "Raster map", YES);
    opt_raster_input->gisprompt = (char*) "old,cell,raster";
    
    struct Option *opt_output = define_string_option("output",  "Output raster", YES);
    
    opt_layer_name = define_string_option("layer",  "Layer within vector dataset", NO);
    
	if ( call_parser(argc, argv) ) {
		return EXIT_FAILURE;
    }
    
	Vector* vect = open_vector(opt_vector_input->answer);
    if ( !vect ) {
        return EXIT_FAILURE;
    }
    
    process_opt_layer_name();

    Raster* rast = open_raster(opt_raster_input->answer);
    if ( !rast ) {
        delete vect;
        return EXIT_FAILURE;
    }
    
    RasterizeParams rasterizeParams;
    rasterizeParams.outRaster_filename = opt_output->answer;
    rasterizeParams.fillNoData = true;
    
    GDALDataset* ds = rast->getDataset();
    rasterizeParams.projection = ds->GetProjectionRef();
    ds->GetGeoTransform(rasterizeParams.geoTransform);
    Observer* obs = starspan_getRasterizeObserver(&rasterizeParams);
    if ( obs ) {
		Traverser tr;
        tr.setVector(vect);
        tr.setLayerNum(vector_layernum);
		tr.addRaster(rast);
        tr.addObserver(obs);
        
        tr.traverse();
        
        tr.releaseObservers();
    }
    
    delete vect;
    delete rast;
    
    return 0;
}




///////////////////////////////////////////////////////////////////////////////
// main dispatcher of the GRASS interface
int starspan_grass(int argc, char ** argv) {
    const char* cmd = argc == 1 ? "help" : argv[1];
    
    if ( strcmp("help", cmd) == 0 ) {
        fprintf(stdout, 
            " %s\n"
            " USAGE:\n"
            "   starspan help\n"
            "   starspan version\n"
            "   starspan <command> ...arguments...\n"
            " The following commands are implemented: \n"
            "    report ...\n"
            "    csv ...\n"
            "\n"
            "For more details about a command, type:\n"
            "    starspan <command> help\n"
            "\n"
            "To run a command using the GRASS GUI, type:\n"
            "    starspan <command>\n"
            "\n"
            "To run the standard (non-GRASS) StarSpan interface:\n"
            "    starspan S ...arguments...\n"
            "(or just run starspan outside of GRASS)\n"
            "\n"
            ,
            get_module_description(0)
        );
        exit(0);
    }
    else if ( strcmp("version", cmd) == 0 || strcmp("--version", cmd) == 0 ) {
        fprintf(stdout, "starspan %s\n", STARSPAN_VERSION);
        exit(0);
    }
    else if ( strcmp("report", cmd) == 0 || strcmp("cmd=report", cmd) == 0 ) {
        cmd = "report";
    }
    else if ( strcmp("csv", cmd) == 0 || strcmp("cmd=csv", cmd) == 0 ) {
        cmd = "csv";
    }
    else if ( strcmp("rasterize", cmd) == 0 || strcmp("cmd=rasterize", cmd) == 0 ) {
        cmd = "rasterize";
    }
    else {
        G_fatal_error(_("%s: command not recognized/implemented"), cmd);
    }

    // command is OK.
    
	// module initialization
	Raster::init();
	Vector::init();
    
    shift_args(&argc, argv);

    init(cmd);
    //char prgname[256];
    //sprintf(prgname, "starspan %s", cmd);
    //argv[0] = prgname;
    argv[0] = (char*) "starspan";
    
                   
    int res = 0;
    if ( strcmp("report", cmd) == 0 ) {
        res = command_report(argc, argv);
    }
    else if ( strcmp("csv", cmd) == 0 ) {
        res = command_csv(argc, argv);
    }
    else if ( strcmp("rasterize", cmd) == 0 ) {
        res = command_rasterize(argc, argv);
    }
    
	Vector::end();
	Raster::end();
	
	// more final cleanup:
	Unload::Release();	

    exit(res);
}


///////////////////////////////////////////////////////////////////////////////
// Not with GRASS
#else

bool use_grass(int *argc, char ** argv) {
    return false;
}

int starspan_grass(int argc, char ** argv) {
    fprintf(stderr, "ERROR: StarSpan not compiled with grass support.\n");
    return -1;
}

#endif

