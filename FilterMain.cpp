#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include "Filter.h"

using namespace std;

#include "rtdsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int
main(int argc, char **argv)
{

  if ( argc < 2) {
    fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
  }

  //
  // Convert to C++ strings to simplify manipulation
  //
  string filtername = argv[1];

  //
  // remove any ".filter" in the filtername
  //
  string filterOutputName = filtername;
  string::size_type loc = filterOutputName.find(".filter");
  if (loc != string::npos) {
    //
    // Remove the ".filter" name, which should occur on all the provided filters
    //
    filterOutputName = filtername.substr(0, loc);
  }

  Filter *filter = readFilter(filtername);

  double sum = 0.0;
  int samples = 0;

  for (int inNum = 2; inNum < argc; inNum++) {
    string inputFilename = argv[inNum];
    string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
    struct cs1300bmp *input = new struct cs1300bmp;
    struct cs1300bmp *output = new struct cs1300bmp;
    int ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

    if ( ok ) {
      double sample = applyFilter(filter, input, output);
      sum += sample;
      samples++;
      cs1300bmp_writefile((char *) outputFilename.c_str(), output);
    }
    delete input;
    delete output;
  }
  fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

struct Filter *
readFilter(string filename)
{
  ifstream input(filename.c_str());

  if ( ! input.bad() ) {
    int size = 0;
    input >> size;
    Filter *filter = new Filter(size);
    int div;
    input >> div;
    filter -> setDivisor(div);
    for (int i=0; i < size; i++) {
      for (int j=0; j < size; j++) {
	int value;
	input >> value;
	filter -> set(i,j,value);
      }
    }
    return filter;
  }
}


double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{

  long long cycStart, cycStop;

  cycStart = rdtscll();

  output -> width = input -> width;
  output -> height = input -> height;
    
    /* Turn loop conditions into local variables
    avoid function calls */
    int width = (input -> width) - 1;
    int height = (input -> height) - 1;
    int filterSize = filter -> getSize();
    int x1;
    int x2;
    int x3;
    int newFilter[filterSize][filterSize];
    int divisor = filter -> getDivisor();
    
    	for (int j = 0; j < filterSize; j++) {
	  for (int i = 0; i < filterSize; i++) {
          newFilter[i][j] = filter -> get(i,j);
      }
        }


  for(int col = 1; col < width; col++) {
    for(int row = 1; row < height ; row++) {
      for(int plane = 0; plane < 3; plane++) {

	int t = 0;
	output -> color[plane][row][col] = 0;



          /* j = 0, i = 0,1,2 */
          x1 = (input -> color[plane][row - 1][col - 1] 
		 * newFilter[0][0]);
          x2 = (input -> color[plane][row][col - 1] 
		 * newFilter[1][0]);
          x3 = (input -> color[plane][row + 1][col - 1] 
		 * newFilter[2][0]);
          /* j = 1, i = 0,1,2 */
          x1 += (input -> color[plane][row - 1][col - 1] 
		 * newFilter[0][1]);
          x2 += (input -> color[plane][row][col] 
		 * newFilter[1][1]);
          x3 += (input -> color[plane][row + 1][col + 1] 
		 * newFilter[2][1]);
         /* j = 2, i = 0,1,2 */
          x1 += (input -> color[plane][row - 1][col - 1] 
		 * newFilter[0][2]);
          x2 += (input -> color[plane][row][col] 
		 * newFilter[1][2]);
          x3 += (input -> color[plane][row + 1][col + 1] 
		 * newFilter[2][2]);
          
	output -> color[plane][row][col] = x1 +x2 +x3;
	output -> color[plane][row][col] = 	
	  output -> color[plane][row][col] / divisor;

	if (output -> color[plane][row][col]  < 0 || output -> color[plane][row][col]  > 255 ) {
          if ( output -> color[plane][row][col]  < 0 ) {
	  output -> color[plane][row][col] = 0;
	}

	else if ( output -> color[plane][row][col]  > 255 ) { 
	  output -> color[plane][row][col] = 255;
	}
    }
      }
    }
  }

  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}
