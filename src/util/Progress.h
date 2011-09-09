//
// Progress - A simple progress indicator
// Carlos A. Rueda
// $Id: Progress.h,v 1.1 2005-02-19 21:29:20 crueda Exp $
//

#ifndef Progress_h
#define Progress_h

#include <iostream>
#include <string>

using namespace std;


/**
  * A simple progress indicator.
  * Two kinds of progress objects can be created: percentage and
  * counter.
  * In both cases an internal counter is updated every time
  * update() is called.
  */
class Progress {
public:
	/** creates a percentage progress object.
	  * @param size number of items corresponding to 100%.
	  * @param perc_incr desired increment in percentage to update
	  *        the progress message; must be > 0
	  * @param out output stream where the progress is written out.
	  */
	Progress(long size, double perc_incr, ostream& out=cout);
	
	/** creates a counter progress object.
	  * @param chunksize increment amount. Every time the counter
	  * is a multiple of this size, the counter is written out.
	  * @param out output stream where the progress is written out.
	  */
	Progress(long chunksize, ostream& out=cout);
	
	/** starts this progress */
	void start();
	
	/** Updates this progress object.
	  * An internal counter is incremented to keep track of the number
	  * of times this method has been called.
	  * (For a percentage progress this will normally be called as 
	  * many times as for the 100%.)
	  * A message will be written if necessary according to the
	  * parameters used to create this progress object.
	  */
	void update();

	/** Completes this progress.
	  * If this is a percentage progress and "100%" has not been yet
	  * written, then "100%" will be written regardless of the number
	  * of times update() was called.
	  * If this is a counter progress and the current counter is not
	  * a multiple of the chunksize, then counter will be written out.
	  */
	void complete();
	
private:
	long size;
	long count_update;
	double perc_incr;
	ostream& out;
	double next_perc;
	double curr_perc;
};


#endif

