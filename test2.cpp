#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

int main() {
    // Load the image
    cv::Mat image = cv::imread("spr2024.jpg");
    if (image.empty()) {
        std::cerr << "Could not open or find the image!\n";
        return -1;
    }

    // Preprocessing
    cv::Mat gray, binary;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);  // Convert to grayscale
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);  // Binarize image

    // Initialize Tesseract API
    tesseract::TessBaseAPI* ocr = new tesseract::TessBaseAPI();
    if (ocr->Init(NULL, "eng")) {  // Initialize with English language
        std::cerr << "Could not initialize tesseract.\n";
        return -1;
    }

    // Set the image for OCR
    ocr->SetImage(binary.data, binary.cols, binary.rows, 1, binary.step);

    // Extract text
    std::string extractedText = std::string(ocr->GetUTF8Text());
    std::cout << "Extracted Text:\n" << extractedText << std::endl;

    // Cleanup
    ocr->End();
    delete ocr;

    return 0;
}
