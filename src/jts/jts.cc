//
// STARSpan project
// JTS routines
// Carlos A. Rueda
// $Id: jts.cc,v 1.1 2005-02-19 21:04:14 crueda Exp $
// See jts.h for public documentation
//

#include "jts.h"           
#include "unistd.h"   // unlink           
#include "stdlib.h"   // exit -- on some systems           


JTS_TestGenerator::JTS_TestGenerator(const char* filename_) {
	filename = filename_;
	file = fopen(filename, "w");
	if ( !file ) {
		fprintf(stderr, "Could not create %s\n", filename);
		exit(1);
	}
	fprintf(file, "<run>\n");
	fprintf(file, "  <desc>testset created by starspan</desc>\n");
	fprintf(file, "  <precisionModel type=\"FLOATING\" />\n");
	no_cases = 0;
	case_pos = ftell(file);
}

JTS_TestGenerator::~JTS_TestGenerator() {
	end();
}

void JTS_TestGenerator::case_init(const char* description) {
	if ( file ) {
		case_pos = ftell(file);
		fprintf(file, "  <case>\n");
		fprintf(file, "    <desc>%s</desc>\n", description);
	}
}

void JTS_TestGenerator::case_arg_init(const char* argname) {
	if ( file ) {
		fprintf(file, "    <%s>\n", argname);
	}
}

void JTS_TestGenerator::case_arg_end(const char* argname) {
	if ( file ) {
		fprintf(file, "    </%s>\n", argname);
	}
}

void JTS_TestGenerator::case_end(const char* predicate) {
	if ( file ) {
		fprintf(file, "    <test>\n");
		fprintf(file, "      <op name=\"%s\" arg1=\"a\" arg2=\"b\">true</op>\n", predicate);
		fprintf(file, "    </test>\n");
		fprintf(file, "  </case>\n");
		no_cases++;
	}
}

void JTS_TestGenerator::case_cancel() {
	if ( file ) {
		fseek(file, case_pos, SEEK_SET);
		ftruncate(fileno(file), case_pos);
	}
}

int JTS_TestGenerator::end() {
	if ( file ) {
		fprintf(file, "</run>\n");
		fclose(file);
		file = NULL;
		if ( no_cases == 0 ) {
			unlink(filename);
		}
	}
	return no_cases;
}

