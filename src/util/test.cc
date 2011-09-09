// a test for Progress
#include "Progress.h"

int main() {
	long size = 500;
	
	Progress* p;

	cout<< "size=" << size<< "  %incr=" << 11<< ": ";	
	p = new Progress(size, 11);
	p->start();
	for ( int i = 0; i < size; i++ )
		p->update();
	p->complete();
	delete p;
	cout << endl;
	
	cout<< "chunks of size=" << size<< ": ";
	p = new	Progress(size);
	p->start();
	for ( int i = 0; i < size*7 - 123; i++ )
		p->update();
	p->complete();
	delete p;
	cout << endl;
	
	return 0;
}
