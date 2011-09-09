/* Copyright (C) 1999 Lucent Technologies */
/* Excerpted from 'The Practice of Programming' */
/* by Brian W. Kernighan and Rob Pike */

//
//
// g++ -Wall Csv.cc csvtest.cc -o csvtest
//
//

#include "Csv.h"

// Csvtest main: test Csv class
int main(void)
{
	string line;
	Csv csv;

	while ( csv.getline(line) ) {
		cout << "line = `" << line <<"'\n";
		for (string::size_type i = 0; i < csv.getnfield(); i++)
			cout << "field[" << i << "] = `"
				 << csv.getfield(i) << "'\n";
	}
	return 0;
}
