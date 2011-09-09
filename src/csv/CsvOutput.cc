// 
// Helper class to write out CSV values.
// @author Carlos Rueda
// $Id: CsvOutput.cc,v 1.1 2007-12-14 07:43:01 crueda Exp $
// 

#include "Csv.h"
#include <stdarg.h>


CsvOutput& CsvOutput::startLine() {
	numFields = 0;
	return *this;
}

CsvOutput& CsvOutput::addString(string value) {
	if  ( numFields++ > 0 ) {
		fprintf(file, "%s", separator.c_str());
	}
		
	if ( value.find(separator) != string::npos ) {
		fprintf(file, "%s%s%s", quote.c_str(), value.c_str(), quote.c_str());
	}
	else {
		fprintf(file, "%s", value.c_str());
	}
	return *this;
}

CsvOutput& CsvOutput::addField(const char* fmt, ...) {
	va_list args;
	
	va_start(args, fmt);

	char buff[8*1024];
	vsprintf(buff, fmt, args);
	va_end(args);
	
	return addString(buff);
}

void CsvOutput::endLine() {
	if  ( numFields > 0 ) {	
		fprintf(file, "\n");
	}
	numFields = 0;
}


