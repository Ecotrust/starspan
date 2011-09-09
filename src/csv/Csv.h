/* Copyright (C) 1999 Lucent Technologies */
/* Excerpted from 'The Practice of Programming' */
/* by Brian W. Kernighan and Rob Pike */
// updated by carueda 2004-09-16
// $Id: Csv.h,v 1.2 2007-12-14 07:43:01 crueda Exp $

#ifndef Csv_h
#define Csv_h

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>

using namespace std;

class Csv {	// read and parse comma-separated values
	// sample input: "LU",86.25,"11/4/1998","2:19PM",+4.0625

  public:
	Csv(istream& fin = cin, string sep = ",") : 
		fin(fin), fieldsep(sep) {}

	bool getline(string&);
	string getfield(string::size_type n);
	unsigned getnfield() const { return nfield; }

  private:
	istream& fin;			// input file pointer
	string line;			// input line
	vector<string> field;	// field strings
	unsigned nfield;				// number of fields
	string fieldsep;		// separator characters

	unsigned split();
	bool endofline(char);
	string::size_type advplain(const string& line, string& fld, int);
	string::size_type advquoted(const string& line, string& fld, int);
};


/**
 * Helper class to write out CSV values.
 * @author Carlos Rueda
 */
class CsvOutput {

  public:
  	/**
	 * Creates an instance with (stdout, ",", "\"") as default parameters.
	 */
	CsvOutput(string sep = ",", string quote = "\"") :   
		file(stdout), separator(sep), quote(quote), numFields(0) {}
		
	void setFile(FILE* f) {
		file = f;
	}

	void setSeparator(string sep) {
		separator = sep;
	}

	void setQuote(string q) {
		quote = q;
	}

	/**
	 * Starts a new line.
	 * Make sure you call endLine() first if fields have already been written.
	 * @return this
	 */
	CsvOutput& startLine();
	
	/**
	 * Adds a field to the current line.
	 * @return this
	 */
	CsvOutput& addString(string field);
	
	/**
	 * Adds a formated field to the current line.
	 * @return this
	 */
	CsvOutput& addField(const char* fmt, ...);
	
	/** 
	 * Writes a line feed.
	 */
	void endLine(void);
	
  private:
	FILE* file;
	string separator;
	string quote;
	int numFields;
};

#endif
