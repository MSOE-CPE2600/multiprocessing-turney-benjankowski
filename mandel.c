/// 
//  mandel.c
//  Based on example code found here:
//  https://users.cs.fiu.edu/~cpoellab/teaching/cop4610_fall22/project3.html
//
//  Converted to use jpg instead of BMP and other minor changes
//
// Modified by Benjamin Jankowski
// Multiprocessing Lab 11
///
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sys/wait.h>
#include <pthread.h>
#include "jpegrw.h"

struct pixel_thread {
	int row_start;
	int row_end;
	int image_width;
	int image_height;
	double xmax;
	double xmin;
	double ymax;
	double ymin;
	int max;
	imgRawImage* img;
};

// local routines
static int iteration_to_color( int i, int max );
static int iterations_at_point( double x, double y, int max );
static void compute_image( imgRawImage *img, double xmin, double xmax,
									double ymin, double ymax, int max, int thread_count );
static void show_help();
static void* compute_pixel_thread(void* args);


int main( int argc, char *argv[] )
{
	char c;

	// These are the default configuration values used
	// if no command line arguments are given.
	const char *outfile = "mandel%d.jpg";
	double xcenter = 0;
	double ycenter = 0;
	double xscale = 4;
	double yscale = 0; // calc later
	int    image_width = 1000;
	int    image_height = 1000;
	int    max = 1000;
	int    process_count = 1;
	int    thread_count  = 1;
	const double scale_factor_modifier = 0.90;

	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:o:p:t:h"))!=-1) {
		switch(c) 
		{
			case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				xscale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				break;
			case 'H':
				image_height = atoi(optarg);
				break;
			case 'm':
				max = atoi(optarg);
				break;
			case 'o':
				outfile = optarg;
				break;
			case 'p':
				process_count = atoi(optarg);
				if (process_count < 1 || process_count > 50) {
					printf("Invalid process count, see help for more information.");
					exit(1);
				}
				break;
			case 't':
				thread_count = atoi(optarg);
				if (thread_count < 1 || thread_count > (image_width * image_height)) {
					printf("Invalid thread count, see help for more information.");
					exit(1);
				}
				break;
			case 'h':
				show_help();
				exit(1);
		}
	}

	const int process_work_count = ceil(50.0 / process_count);

	for (int n = 0; n < process_count; n++) {
		if (fork() != 0) {
			continue; // Make sure main process does not execute any code.
		}

		for (int i = process_work_count * n; i < process_work_count * (n + 1); i++) {
			if (i > 50) { exit(0); } // Ensure extra images are not created if the process count is even

			// Calculate y scale based on x scale (settable) and image sizes in X and Y (settable)
			// Will multiply scale by a factor of scale_factor_modifier for every process
			const double current_x_scale = xscale * pow(scale_factor_modifier, i);
			yscale = current_x_scale / image_width * image_height;

			const double current_x_center = xcenter - i * 0.04;

			char file_name[100];
			sprintf(file_name,outfile, i);

			// Display the configuration of the image.
			printf("mandel: x=%lf y=%lf xscale=%lf yscale=%1f max=%d outfile=%s\n",
				current_x_center,ycenter,current_x_scale,yscale,max,file_name);

			// Create a raw image of the appropriate size.
			imgRawImage* img = initRawImage(image_width,image_height);

			// Fill it with a black
			setImageCOLOR(img,0);

			// Compute the Mandelbrot image
			compute_image(img,
				current_x_center-current_x_scale/2,
				current_x_center+current_x_scale/2,
				ycenter-yscale/2,
				ycenter+yscale/2,max, thread_count);

			// Save the image in the stated file.
			storeJpegImageFile(img,file_name);

			// free the mallocs
			freeRawImage(img);
		}

		exit(0); // Ensure child exists
	}

	for (int i = 0; i < process_count; i++) {
		wait(NULL);
	}

	return 0;
}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iter;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/
void compute_image(imgRawImage* img, double xmin, double xmax, double ymin, double ymax, int max, int thread_count)
{
	int width = img->width;
	int height = img->height;

	pthread_t threads[thread_count];
	struct pixel_thread* thread_data[thread_count];

	int lines_per_thread = ceil(height/thread_count);

	for(int i = 0; i < thread_count; i++) {
		thread_data[i] = malloc(sizeof(struct pixel_thread));

		thread_data[i]->row_start = i * lines_per_thread;
		thread_data[i]->row_end = thread_data[i]->row_start + lines_per_thread;
		thread_data[i]->image_width = width;
		thread_data[i]->image_height = height;
		thread_data[i]->xmax = xmax;
		thread_data[i]->xmin = xmin;
		thread_data[i]->ymax = ymax;
		thread_data[i]->ymin = ymin;
		thread_data[i]->max = max;
		thread_data[i]->img = img;

		if (thread_data[i]->row_start > height) {
			free(thread_data[i]);
			break;
		}

		if (thread_data[i]->row_end > height) {
			thread_data[i]->row_end = height;
		}

		printf("Pixel thread: %d %d\n", thread_data[i]->row_start, thread_data[i]->row_end);

		pthread_create(&threads[i], NULL, compute_pixel_thread, thread_data[i]);
	}

	for (int i = 0; i < thread_count; i++) {
		pthread_join(threads[i], NULL);
		free(thread_data[i]);
	}
}

void* compute_pixel_thread(void* args) {
	struct pixel_thread* info = args;

	for (int j = info->row_start; j < info->row_end; j++) {
		for(int i = 0; i < info->image_width; i++) {

			// Determine the point in x,y space for that pixel.
			double x = info->xmin + i*(info->xmax-info->xmin)/info->image_width;
			double y = info->ymin + j*(info->ymax-info->ymin)/info->image_height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,info->max);

			// Set the pixel in the bitmap.
			setPixelCOLOR(info->img,i,j,iteration_to_color(iters,info->max));
		}
	}

	return NULL;
}

/*
Convert a iteration number to a color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/
int iteration_to_color( int iters, int max )
{
	int color = 0xFFFFFF*iters/(double)max;
	return color;
}

// Show help message
void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates (X-axis). (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=1000)\n");
	printf("-H <pixels> Height of the image in pixels. (default=1000)\n");
	printf("-p <count>  Sets the number of processes to make 50 images. (default=4, must be between 1 and 50\n");
	printf("-t <count>  Sets the number of threads to process image pixels (default=1, must be less than pixel count)\n");
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}
