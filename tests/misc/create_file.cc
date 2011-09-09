//
//  A simple program to create a BIG file.
//  $Id: create_file.cc,v 1.4 2007-10-19 01:04:49 crueda Exp $
//
//    g++ -D_FILE_OFFSET_BITS=64 -Wall create_file.cc -o create_file
//    ./create_file BIG 10
//    # should create a file with size 2147483647 + 10 = 2147483657 bytes
//


#if defined mingw32 || defined WIN32
	#define OFF_T off64_t
	#define FSEEK fseeko64
	const char* logmsg = "windows env detected: Using off64_t and fseeko64";
#else
	#define OFF_T off_t
	#define FSEEK fseeko
	const char* logmsg = "Using off_t and fseeko";
#endif

#include <cstdio>
#include <iostream>
using namespace std;


int main(int argc, char** argv) {
	if ( argc != 3 ) {
		cerr<< " USAGE:   ./create_file  filename  size_incr" << endl
		    << "     filename will be 2147483647 + size_incr bytes size" << endl
		    << " Example: ./create_file  BIG  10" << endl
		    << "   should create a file with size 2147483657" << endl;
		return 1;
	}
	const char* filename = argv[1];
	OFF_T incr = atol(argv[2]);

	cout<< "  " << logmsg << endl;
	cout<< "  sizeof(OFF_T) = " << sizeof(OFF_T) << endl;

	FILE* file = fopen(filename, "w");
	if ( !file ) {
		cerr<< " could not create " << filename << endl;
		return 1;
	}
	OFF_T size = (OFF_T) 2147483647 + incr;
	cout<< "  seeking one byte before size: " << size-1 << endl;
	FSEEK(file, size-1, SEEK_SET);
	cout<< "  writing a byte "<< endl;
	fputc('\1', file);

	cout<< "  closing.\n";
	fclose(file);
	cout<< "  done.\n";
	cout<< "  File size should be: " << size << endl;
	return 0;
}
