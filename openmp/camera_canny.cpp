#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include "opencv2/opencv.hpp"
#include "canny_util.h"

using namespace std;
using namespace cv;

/* Possible options: 320x240, 640x480, 1024x768, 1280x1040, and so on. */
/* Pi Camera MAX resolution: 2592x1944 */

#define WIDTH 640
#define HEIGHT 480
#define NFRAME 1.0

int main(int argc, char **argv) {
    char* dirfilename;        /* Name of the output gradient direction image */
    unsigned char *image;     /* The input image */
    unsigned char *edge;      /* The output edge image */
    int rows, cols;           /* The dimensions of the image. */
    float sigma,              /* Standard deviation of the gaussian kernel. */
          tlow,               /* Fraction of the high threshold in hysteresis. */
          thigh;              /* High hysteresis threshold control. */

    if (argc < 4) {
        fprintf(stderr, "\n<USAGE> %s sigma tlow thigh [writedirim]\n", argv[0]);
        exit(1);
    }

    sigma = atof(argv[1]);
    tlow = atof(argv[2]);
    thigh = atof(argv[3]);
    rows = HEIGHT;
    cols = WIDTH;

    if (argc == 5) dirfilename = (char*)"dummy";
    else dirfilename = NULL;

    std::string pipeline = "libcamerasrc ! video/x-raw, width=" +
                           std::to_string(WIDTH) +
                           ", height=" + std::to_string(HEIGHT) +
                           ", format=(string)BGR ! videoconvert ! appsink";
    VideoCapture cap(pipeline, CAP_GSTREAMER);

    if (!cap.isOpened()) {
        cerr << "[ERROR] Failed to open camera!" << endl;
        return -1;
    }

    Mat frame, grayframe;
    int frame_count = 1; // Counter for saved frames
    char filename[128]; // For output filenames

    // Variables for FPS calculation
    double fps = 0.0;
    int frame_counter = 0;
    time_t start_time = time(0);

    printf("[INFO] Press ESC to capture, process, and save images...\n");
    printf("[INFO] Press Q to quit the program...\n");

    while (true) {
        cap >> frame;
        if (frame.empty()) {
            cerr << "[ERROR] Empty frame captured!" << endl;
            break;
        }

        // Calculate FPS every second
        frame_counter++;
        if (time(0) - start_time >= 1) {
            fps = frame_counter / (time(0) - start_time);
            frame_counter = 0;
            start_time = time(0);
        }

        // Overlay FPS on the live feed
        stringstream fps_text;
        fps_text << "FPS: " << fixed << setprecision(2) << fps;
        putText(frame, fps_text.str(), Point(10, 30), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0), 2);

        imshow("[LIVE FEED] Press ESC to process and save frame", frame);

        int key = waitKey(10);

        if (key == 27) { // ESC key to capture and process
            clock_t begin = clock();
            
            // Convert frame to grayscale
            cvtColor(frame, grayframe, COLOR_BGR2GRAY);
            image = grayframe.data;

            // Perform Canny edge detection
            if (VERBOSE) printf("Starting Canny edge detection.\n");
            if (dirfilename != NULL) {
                sprintf(filename, "frame%03d_direction.fim", frame_count);
                dirfilename = filename;
            }
            canny(image, rows, cols, sigma, tlow, thigh, &edge, dirfilename);

            // Save edge-detected image
            sprintf(filename, "frame%03d.pgm", frame_count);
            if (VERBOSE) printf("Saving edge image to file %s\n", filename);
            if (write_pgm_image(filename, edge, rows, cols, NULL, 255) == 0) {
                fprintf(stderr, "Error writing the edge image, %s.\n", filename);
                break;
            }

            clock_t end = clock();
            double time_elapsed = (double)(end - begin) / CLOCKS_PER_SEC;
            printf("[INFO] Frame %03d processed and saved in %lf seconds\n", frame_count, time_elapsed);

            // Show processed edge image
            Mat edgeframe(rows, cols, CV_8UC1, edge);
            imshow("[EDGE DETECTION] Processed Frame", edgeframe);

            frame_count++; // Increment frame count
        } else if (key == 'q' || key == 'Q') { // Q key to quit
            break;
        }
    }

    printf("[INFO] Program terminated. %d frames saved.\n", frame_count - 1);
    return 0;
}
