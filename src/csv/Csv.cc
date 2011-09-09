/* Copyright (C) 1999 Lucent Technologies */
/* Excerpted from 'The Practice of Programming' */
/* by Brian W. Kernighan and Rob Pike */
// updated by carueda 2004-09-16
// $Id: Csv.cc,v 1.1 2005-02-19 21:03:46 crueda Exp $

#include "Csv.h"

// endofline: check for and consume \r, \n, \r\n, or EOF
bool Csv::endofline(char c)
{
	bool eol;

	eol = (c=='\r' || c=='\n');
	if (c == '\r') {
		fin.get(c);
		if (!fin.eof() && c != '\n')
			fin.putback(c);	// read too far
	}
	return eol;
}

// getline: get one line, grow as needed
bool Csv::getline(string& str)
{	
	char c;

	for (line = ""; fin.get(c) && !endofline(c); ) {
		line += c;
	}
	split();
	str = line;
	return !fin.eof();
}

// split: split line into fields
unsigned Csv::split()
{
	string fld;
	string::size_type i, j;

	nfield = 0;
	if (line.length() == 0)
		return 0;
	i = 0;

	const string::size_type line_len = line.length();
	
	do {
		if (i < line_len && line[i] == '"')
			j = advquoted(line, fld, ++i);	// skip quote
		else
			j = advplain(line, fld, i);
		if (nfield >= field.size())
			field.push_back(fld);
		else
			field[nfield] = fld;
		nfield++;
		i = j + 1;
	} while (j < line_len);

	return nfield;
}

// advquoted: quoted field; return index of next separator
string::size_type Csv::advquoted(const string& s, string& fld, int i)
{
	string::size_type j;

	fld = "";
	for (j = i; j < s.length(); j++) {
		if (s[j] == '"' && s[++j] != '"') {
			string::size_type k = s.find_first_of(fieldsep, j);
			if (k == string::npos )	// no separator found
				k = s.length();
			for (k -= j; k-- > 0; )
				fld += s[j++];
			break;
		}
		fld += s[j];
	}
	return j;
}

// advplain: unquoted field; return index of next separator
string::size_type Csv::advplain(const string& s, string& fld, int i)
{
	string::size_type j;

	j = s.find_first_of(fieldsep, i); // look for separator
	if (j == string::npos )               // none found
		j = s.length();
	fld = string(s, i, j-i);
	return j;
}


// getfield: return n-th field
string Csv::getfield(string::size_type n)
{
	if (n < 0 || n >= nfield)
		return "";
	else
		return field[n];
}

