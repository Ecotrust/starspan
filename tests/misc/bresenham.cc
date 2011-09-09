#include <iostream>
#include <cstdlib>


void linev6( unsigned x1, unsigned y1,
             unsigned x2, unsigned y2
           )
{
	int dx  = x2 - x1,
	          dy  = y2 - y1,
	                y   = y1,
	                      eps = 0;

	for ( int x = x1; x <= x2; x++ )  {
		cout<< "DataSet: pixel" <<endl;
		cout <<x<< " , " <<y<< endl;
		cout <<x+1<< " , " <<y<< endl;
		cout <<x+1<< " , " <<y+1<< endl;
		cout <<x<< " , " <<y+1<< endl;
		cout <<x<< " , " <<y<< endl;
		eps += dy;
		if ( (eps << 1) >= dx )  {
			y++;  eps -= dx;
		}
	}
}

int main(int argc, char** argv) {
	int arg = 1;
	int x1 = atoi(argv[arg++]);
	int y1 = atoi(argv[arg++]);
	int x2 = atoi(argv[arg++]);
	int y2 = atoi(argv[arg++]);

	cout<< "XTicks:";
	for ( int i = x1; i <= x2 ; i++ )
		cout<< " " <<i<< " " <<i<< ",";
	cout<< endl;
	cout<< "YTicks:";
	for ( int i = y1; i <= y2; i++ )
		cout<< " " <<i<< " " <<i<< ",";
	cout<< endl;
	
	cout<< "DataSet: bresenham" <<endl;
	cout <<0.5+x1<< " , " <<0.5+y1<< endl;
	cout <<0.5+x2<< " , " <<0.5+y2<< endl;
	
	linev6(x1, y1, x2, y2);
}
