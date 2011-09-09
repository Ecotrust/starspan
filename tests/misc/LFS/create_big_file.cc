//
//  A simple program to create a BIG file.
//  $Id: create_big_file.cc,v 1.1 2007-10-19 01:52:23 crueda Exp $
//
//    make simple
//

#include "lfs.h"

#include <cstdio>
#include <iostream>
using namespace std;


int main(int argc, char** argv) {
	if ( argc != 3 ) {
		cerr<< " USAGE:   ./create_big_file  filename  size_incr" << endl
		    << "     filename will be 2147483647 + size_incr bytes size" << endl
		    << " Example: ./create_big_file  BIG  10" << endl
		    << "   should create a file with size 2147483657" << endl;
		return 1;
	}
	const char* filename = argv[1];
	my_OFF_T incr = atol(argv[2]);

	cout<< "  " << lfs_logmsg << endl;
	cout<< "  sizeof(my_OFF_T) = " << sizeof(my_OFF_T) << endl;

	FILE* file = fopen(filename, "w");
	if ( !file ) {
		cerr<< " could not create " << filename << endl;
		return 1;
	}
	my_OFF_T size = (my_OFF_T) 2147483647 + incr;
	cout<< "  seeking one byte before size: " << size-1 << endl;
	my_FSEEK(file, size-1, SEEK_SET);
	cout<< "  writing a byte "<< endl;
	fputc('\1', file);

	cout<< "  closing.\n";
	fclose(file);
	cout<< "  done.\n";
	cout<< "  File size should be: " << size << endl;
	return 0;
}
