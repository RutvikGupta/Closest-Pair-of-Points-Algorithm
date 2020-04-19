#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "point.h"
#include "utilities_closest.h"
#include "serial_closest.h"
#include "parallel_closest.h"


void print_usage() {
    fprintf(stderr, "Usage: closest -f filename -d pdepth\n\n");
    fprintf(stderr, "    -d Maximum process tree depth\n");
    fprintf(stderr, "    -f File that contains the input points\n");

    exit(1);
}

int main(int argc, char **argv) {
    int n = -1;
    long pdepth = -1;
    char *filename = NULL;
    int pcount = 0;

    // TODO: Parse the command line arguments
    int opt;
    // check the number of command line arguments
    if (argc != 5) {
		print_usage();
    }
   // use getopt to read in the command line arguments with flag f and d
    while ((opt = getopt(argc, argv, "f:d:")) != -1) {
		switch(opt) {
			// if the current flag is -f store the name of input file containing the points
			case 'f':
	    		if (optarg != NULL) {
	        		filename = optarg;
	    		}
	    		break;
	    	// if the current flag is -d store the  maximum number of process tree depth
			case 'd':
	    		if (optarg != NULL && optarg >= 0) {
	        		pdepth = atoi(optarg);
	    		}
	    		break;
	    	// if any flag other than -f or -d is used or the input order is incorrect throw an error and exit
			default: /* '?' */
	    		print_usage();
		}
    }
    // Read the points
    n = total_points(filename);
    struct Point points_arr[n];
    read_points(filename, points_arr);

    // Sort the points
    qsort(points_arr, n, sizeof(struct Point), compare_x);

    // Calculate the result using the parallel algorithm.
    double result_p = closest_parallel(points_arr, n, pdepth, &pcount);
    printf("The smallest distance: is %.2f (total worker processes: %d)\n", result_p, pcount);
    exit(0);
}
