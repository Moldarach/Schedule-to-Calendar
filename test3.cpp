#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Load the image
    cv::Mat image = cv::imread("aut2023.jpg");
    if (image.empty()) {
        std::cerr << "Could not open or find the image!\n";
        return -1;
    }

    // Convert the image to HSV color space
    cv::Mat hsvImage;
    cv::cvtColor(image, hsvImage, cv::COLOR_BGR2HSV);

    cv::Scalar purple = cv::Scalar(221, 221, 255);


    // Create masks for colors to exclude (white, gray, black)
    //cv::Mat whiteMask;
    //cv::inRange(hsvImage, cv::Scalar(0, 55, 0), cv::Scalar(0, 100, 0), whiteMask);  // White
    cv::Mat whiteMask, lightGrayMask, darkGrayMask, textMask, jpgMask;
    cv::inRange(hsvImage, cv::Scalar(0, 0, 98), cv::Scalar(0, 0, 101), whiteMask);  // White
    cv::inRange(hsvImage, cv::Scalar(0, 0, 00), cv::Scalar(0, 0, 101), lightGrayMask);  // Light gray
    cv::inRange(hsvImage, cv::Scalar(10, 10, 10), cv::Scalar(256, 256, 100), darkGrayMask);   // Dark gray
    cv::inRange(hsvImage, cv::Scalar(0, 0, 0), cv::Scalar(0, 0, 140), textMask);  // Black

    //cv::inRange(hsvImage, cv::Scalar(10, 0, 80), cv::Scalar(180, 255, 240), jpgMask);
    cv::inRange(hsvImage, cv::Scalar(10, 30, 0), cv::Scalar(180, 255, 255), jpgMask);
    
    // Combine masks for white, gray, and black
    /*
    cv::Mat excludedColorsMask;
    cv::bitwise_or(whiteMask, lightGrayMask, excludedColorsMask);
    cv::bitwise_or(excludedColorsMask, darkGrayMask, excludedColorsMask);
    cv::bitwise_or(excludedColorsMask, blackMask, excludedColorsMask);
    */
    // Invert the mask to get regions with other colors
    cv::Mat maskForOtherColors;
    maskForOtherColors = lightGrayMask;
    // maskForOtherColors = jpgMask;
    
    //cv::bitwise_not(excludedColorsMask, maskForOtherColors);

    // Find contours of objects that are not white, gray, or black
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(maskForOtherColors, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Draw bounding boxes around detected objects
    for (size_t i = 0; i < contours.size(); i++) {
        cv::Rect boundingBox = cv::boundingRect(contours[i]);
      if (boundingBox.width >= 50 && boundingBox.height >= 50) {
        cv::rectangle(image, boundingBox, cv::Scalar(0, 255, 0), 2);  // Draw green rectangle
        std::cout << "Detected object at: " 
                  << "x = " << boundingBox.x 
                  << ", y = " << boundingBox.y 
                  << ", width = " << boundingBox.width 
                  << ", height = " << boundingBox.height << std::endl;
      }
    }
    // resize image
    image.copyTo(image, jpgMask);
    //cv::resize(image, image, cv::Size(), 0.75, 0.75);
    // Display the image with detected boxes
    cv::imshow("Detected Objects", image);
    cv::waitKey(0);

    return 0;
}
