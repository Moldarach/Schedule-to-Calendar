/*
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include <opencv2/highgui.hpp>
#include <iostream>
int main( int argc, char** argv )
{
    cv::Mat image;
    image = cv::imread("aut2024.png",cv::IMREAD_COLOR);
    if(! image.data)
        {
            std::cout<<"Could not open file" << std::endl;
            return -1;
        }
    cv::namedWindow("image", cv::WINDOW_NORMAL);
    cv::resizeWindow("image", 1280, 720);
    /*
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    cv::Mat binary;
    cv::threshold(gray, binary, 128, 255, cv::THRESH_BINARY_INV);
    
    cv::imshow("image", image);
    
    cv::waitKey(0);
    return 0;
} */
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
using namespace cv;
using namespace std;

bool containsDuplicate3(unordered_map<int, int>& map, int value_diff, int val);
unordered_map<int, int> buckets_x;
unordered_map<int, int> buckets_y;


int main(int argc, char** argv)
{
    // Declare the output variables
    Mat dst, cdst, cdstP;
    const char* default_file = "spr2024.jpg";
    const char* filename = argc >=2 ? argv[1] : default_file;
    // Loads an image
    Mat src = imread( samples::findFile( filename ), IMREAD_GRAYSCALE );
    // Check if image is loaded fine
    if(src.empty()){
        printf(" Error opening image\n");
        printf(" Program Arguments: [image_name -- default %s] \n", default_file);
        return -1;
    }
    // Edge detection
    Canny(src, dst, 50, 200, 3);
    // Copy edges to the images that will display the results in BGR
    cvtColor(dst, cdst, COLOR_GRAY2BGR);
    cdstP = cdst.clone();
    /*
    // Standard Hough Line Transform
    vector<Vec2f> lines; // will hold the results of the detection
    HoughLines(dst, lines, 1, CV_PI/180, 200, 0, 0 ); // runs the actual detection
    // Draw the lines
    for( size_t i = 0; i < lines.size(); i++ )
    {
        float rho = lines[i][0], theta = lines[i][1];
        Point pt1, pt2;
        double a = cos(theta), b = sin(theta);
        double x0 = a*rho, y0 = b*rho;
        pt1.x = cvRound(x0 + 1000*(-b));
        pt1.y = cvRound(y0 + 1000*(a));
        pt2.x = cvRound(x0 - 1000*(-b));
        pt2.y = cvRound(y0 - 1000*(a));
        line( cdst, pt1, pt2, Scalar(0,0,255), 3, LINE_AA);
    } */
    // Probabilistic Line Transform
    vector<Vec4i> linesP; // will hold the results of the detection
    HoughLinesP(dst, linesP, 1, CV_PI/180, 50, 50, 10 ); // runs the actual detection
    // Draw the lines
    int count = 0;
    for( size_t i = 0; i < linesP.size(); i++ )
    {
        Vec4i l = linesP[i];
        // std::cout << "[" << l[0] << ", " << l[1] << "] [" << l[2] << ", " << l[3] << "]" << endl;
        
        // for y-values, vertical lines
        // these LT bounds may cause issues depending on how close to the edge the screenshot is
        if ((l[1] < 30 || l[3] < 30) && l[1] != l[3] && l[0] == l[2]) { // last cond to remove drawn lines through days
          /*
            problem 220 [Contains Duplicate 3] might be useful here to get "unique" lines
            i need distinctive enough y-values to determine where certain times fall
          */ 
          if (!containsDuplicate3(buckets_y, 10, l[0])) { 
            line( cdstP, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, LINE_AA);
          }
        } else if ((l[0] < 50 || l[2] < 50) && l[0] != l[2]) { // for x-values, horizontal lines 
          if (!containsDuplicate3(buckets_x, 10, l[1])) {
            //line( cdstP, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, LINE_AA);
          }
        }
    }
    // resize results
    cv::resize(src, src, cv::Size(), 0.75, 0.75);
    //cv::resize(cdst, cdst, cv::Size(), 0.75, 0.75);
    cv::resize(cdstP, cdstP, cv::Size(), 0.75, 0.75);

    // Show results
    imshow("Source", src);
    //imshow("Detected Lines (in red) - Standard Hough Line Transform", cdst);
    imshow("Detected Lines (in red) - Probabilistic Line Transform", cdstP);
    // Wait and Exit
    waitKey();
    return 0;
}


/*
Pretty similar to problem 220 from Leetcode
Except 'add' cannot be a negative value
Probabilistic hough line transform doesn't seem to make
long connected lines consistently, so there can be multiple
lines on top of each other.
This function removes these overlapping lines

@param: 
  -nums: a vector currently being built up that contains x or y
    coordinates for determining location of times or dates on a schedule
  -value_diff: the threshold for how close two lines need to be (x or y coord)
    to be considered a duplicate.
  -val: the new value to add, which may be a duplicate
*/
bool containsDuplicate3(unordered_map<int, int>& map, int value_diff, int val) {
  int key = val / (value_diff + 1);
  if (val < 0)
      key--;
  if (map.count(key)) 
      return true;
  else if (map.count(key+1) && map[key+1] - val <= value_diff)
      return true;
  else if (map.count(key-1) && val - map[key-1] <= value_diff)
      return true;
  map[key] = val;
  return false;
}