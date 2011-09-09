//
// JTS routines
// Carlos A. Rueda
// $Id: jts.h,v 1.1 2005-02-19 21:04:14 crueda Exp $
//

#ifndef jts_h
#define jts_h

#include <stdio.h>


/**
  * Test generator.
  */
class JTS_TestGenerator {
public:
	/**
	  * creates a JTS test suit.
	  */
	JTS_TestGenerator(const char* filename);
	
	/**
	  * calls end();
	  */
	~JTS_TestGenerator();
	
	const char* getFilename(void) { return filename; }
	FILE* getFile(void) { return file; }
	
	/**
	  * opens a case.
	  */
	void case_init(const char* description);

	/**
	  * opens an argument spec
	  */
	void case_arg_init(const char* argname);

	/**
	  * closes an argument spec
	  */
	void case_arg_end(const char* argname);

	/**
	  * ends a case.
	  */
	void case_end(const char* predicate);
	
	/**
	  * cancels a case.
	  */
	void case_cancel(void);
	
	/**
	 * completes and closes the file containing the test suit.
	 * If no cases were added, the file is removed.
	 * Returns the number of completed cases.
	 */
	int end();
	
private:
	const char* filename;
	FILE* file;
	int no_cases;
	long case_pos;
};



#endif

