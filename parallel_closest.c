#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "point.h"
#include "serial_closest.h"
#include "utilities_closest.h"


/*
 * Multi-process (parallel) implementation of the recursive divide-and-conquer
 * algorithm to find the minimal distance between any two pair of points in p[].
 * Assumes that the array p[] is sorted according to x coordinate.
 */
double closest_parallel(struct Point *p, int n, int pdmax, int *pcount) {
    *pcount = 0;
    // if n<4 or pdmax is 0 invoke closest_serial
    if (n < 4 || pdmax == 0) {
		return closest_serial(p, n);
    }
    // find the mid index and save the value at that index
    int mid = n /2;
    struct Point mid_point = p[mid];
    // Create two struct Point left and right for the child processes
    struct Point *left_p = p;
    struct Point *right_p = p+mid;
    // Create a pipe array for storing the values  for processes
    int pipe_fd[2][2];
    for (int i = 0; i < 2; i++) {
    	// Pipe the arra to allow communication between parent and children
    	if ((pipe(pipe_fd[i])) == -1) {
	    	perror("fork");
	    	exit(1);
        }
        // Call fork to create child processes
        int r = fork();
        if (r < 0) {
	    	perror("fork");
        }
        // If the current process is child
        else if (r == 0) {
        	// close the reading end of the forked child
	    	if (close(pipe_fd[i][0]) == -1) {
	        	perror("close reading ends of forked child");
				exit(1);
	    	}
	    	for (int child_num = 0; child_num < i; child_num++) {
	    	    // close the reading end of previously forked child
				if (close(pipe_fd[child_num][0]) == -1) {
		    		perror("close reading ends of previously forked children");
		    		exit(1);
	        	}
            }
            // Invoke recursive call to closest_parallel on both the arrays to obtain  the closest
            // distance between pair of points on both sides
	    	double child_dist;
	    	if (i == 0) {
                child_dist = closest_parallel(left_p, mid, --pdmax, pcount);
	    	}
        	else {
                child_dist = closest_parallel(right_p, n - mid, --pdmax, pcount);
	    	}
	    	// Send the distance between the closest pairs to the parent process by writing to the pipe
	    	if (write(pipe_fd[i][1], &child_dist, sizeof(double)) != sizeof(double)) {
		    	perror("write from child to pipe");
		    	exit(1);
			}
			// Close the writing end of the child process
            if (close(pipe_fd[i][1]) == -1) {
	        	perror("closing write end of pipe of child after writing");
				exit(1);
            }
            // Exit with status equal to pcount which represents the number of worker processes
            // in the process tree rooted at curremt process
	    	exit(*pcount);
        }
        // Close the writing end of the parent pipe if the current process is parent
        else {
			if (close(pipe_fd[i][1]) == -1) {
				perror("close writing end of pipe in parent");
				exit(1);
	    	}
        }
    }

    double d_array[2];
    int status;
    // If the current program reaches here, it is guaranteed that the current program is a parent and it has made
    // children. Hence we will increase the pcount by 2
    (*pcount) += 2;
    // If the current process is parent process read the value from the pipe
    for (int i = 0; i < 2; i++) {
    	// Wait for both child process to complete
		if (wait(&status) == -1) {
	    	perror("wait");
	    	exit(1);
		}
        if (WIFEXITED(status)) {
        	// Increase the pcount of parent with the status returned by the child processes
	    	*pcount += WEXITSTATUS(status);
		}
		// Read from the two pipes to retrieve the results from the child processes
		if (read(pipe_fd[i][0], &d_array[i], sizeof(double)) != sizeof(double)) {
	    	perror("reading from pipe of a parent");
	    	exit(1);
		}
		// Close the reading end of the parent pipe
		if (close(pipe_fd[i][0]) == -1) {
	    	perror("close reading of pipe after reading in parent");
	    	exit(1);
		}
    }
    // find the minimum of the result from the two child processes
    double d = min(d_array[0], d_array[1]);
    // Create an array strip[] that contains points closer to the line passing theough the middle point
     struct Point *strip = malloc(sizeof(struct Point) * n);
    if (strip == NULL) {
		perror("malloc");
		exit(1);
    }
    int strip_len = 0;
    for (int i = 0; i < n; i++) {
		if (abs(p[i].x - mid_point.x) < d) {
			strip[strip_len] = p[i], strip_len++;
		}
    }
    // Find the closest points in strip. Return the minimum of d and closest distance in strip[]
    double new_min = min(d, strip_closest(strip, strip_len, d));
    free(strip);
    return new_min;
}
