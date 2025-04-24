#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <iostream>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
using namespace cv;
using namespace std;


typedef struct course {
  string name;
  string location;
  string days;
  string start_time;
  string end_time;

  course() : name(""), location(""), start_time(""), end_time("") {}
  course(string n, string l, string s, string e) : name(n), location(l), start_time(s), end_time(e) {}
} course;

// function headers
bool containsDuplicate3(unordered_map<int, int>& map, int value_diff, int val);
vector<course*> parseCourses(vector<int>& v_line, vector<int>& h_line);
string parseWords(tesseract::TessBaseAPI* ocr, cv::Mat& obj);
void parseTimes(course* curr, vector<int>& v_lines, cv::Rect location);
void parseDays(course* curr, vector<int>& h_lines, cv::Rect location);
// testing purposes
void curl_test();
void xml_test();
// parsing html
xmlNodePtr get_next_element_sibling(xmlNodePtr node);
xmlNodePtr find_closest_ancestor(xmlNodePtr node, const char* tag);
void parse_and_navigate(const string& html);


unordered_map<int, int> buckets_x;
unordered_map<int, int> buckets_y;
string file_name;
vector<string> all_days {"M", "T", "W", "Th", "F"};

int main(int argc, char** argv) {
  curl_test();
  return 0;

  // Declare the output variables
  Mat dst, cdst, cdstP;
  const char* default_file = "spr2024.jpg";
  const char* filename = argc >=2 ? argv[1] : default_file;
  file_name = filename;
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
  
  // Probabilistic Line Transform
  vector<Vec4i> linesP; // will hold the results of the detection
  HoughLinesP(dst, linesP, 1, CV_PI/180, 50, 50, 10 ); // runs the actual detection
  // Draw the lines
  int count = 0;
  for( size_t i = 0; i < linesP.size(); i++ )
  {
      Vec4i l = linesP[i];
      // std::cout << "[" << l[0] << ", " << l[1] << "] [" << l[2] << ", " << l[3] << "]" << endl;
      
      // for x-values, vertical lines
      // these LT bounds may cause issues depending on how close to the edge the screenshot is
      if ((l[1] < 30 || l[3] < 30) && l[1] != l[3] && l[0] == l[2]) { // last cond to remove drawn lines through days
        /*
          problem 220 [Contains Duplicate 3] might be useful here to get "unique" lines
          i need distinctive enough y-values to determine where certain times fall
        */ 
        if (!containsDuplicate3(buckets_x, 10, l[0])) { 
          //line( cdstP, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, LINE_AA);
          //std::cout << "[" << l[0] << ", " << l[1] << "] [" << l[2] << ", " << l[3] << "]" << endl;
        }
      } else if ((l[0] < 50 || l[2] < 50) && l[0] != l[2]) { // for y-values, horizontal lines 
        if (!containsDuplicate3(buckets_y, 10, l[1])) {
          line( cdstP, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, LINE_AA);
        }
      }
  }
  vector<int> v_line; // for days of week
  for (const auto& [key, value] : buckets_x)
    v_line.push_back(value);
  sort(v_line.begin(), v_line.end());

  vector<int> h_line; // for times during the day
  for (const auto& [key, value] : buckets_y)
    h_line.push_back(value);
  sort(h_line.begin(), h_line.end());


  // resize results
  cv::resize(src, src, cv::Size(), 0.75, 0.75);
  //cv::resize(cdst, cdst, cv::Size(), 0.75, 0.75);
  cv::resize(cdstP, cdstP, cv::Size(), 0.75, 0.75);
  
  vector<course*> courses = parseCourses(v_line, h_line);

  // Show results
  imshow("Source", src);
  //imshow("Detected Lines (in red) - Standard Hough Line Transform", cdst);
  imshow("Detected Lines (in red) - Probabilistic Line Transform", cdstP);
  // Wait and Exit
  waitKey();
  return 0;
}

vector<course*> parseCourses(vector<int>& v_line, vector<int>& h_line) {
  vector<course*> res;
  cv::Mat image = cv::imread(file_name);
    if (image.empty()) {
        std::cerr << "Could not open or find the image!\n";
        return res;
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
    cv::Mat jpgMask2;
    cv::inRange(hsvImage, cv::Scalar(10, 30, 230), cv::Scalar(180, 255, 255), jpgMask2);

    // Invert the mask to get regions with other colors
    cv::Mat maskForOtherColors;
    //maskForOtherColors = lightGrayMask;
    maskForOtherColors = jpgMask2;
    
    //cv::bitwise_not(excludedColorsMask, maskForOtherColors);

    // Find contours of objects that are not white, gray, or black
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(maskForOtherColors, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Preprocessing for before performing OCR
    cv::Mat gray, binary;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);  // Convert to grayscale
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);  // Binarize image
    

    tesseract::TessBaseAPI* ocr = new tesseract::TessBaseAPI();
    if (ocr->Init(NULL, "eng")) {  // Initialize with English language
        std::cerr << "Could not initialize tesseract.\n";
        return res;
    }

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
        cv::circle(image, cv::Point(boundingBox.x, boundingBox.y), 3, cv::Scalar(255, 0, 0));
        cv::Mat box = binary(boundingBox);
        
        string info = parseWords(ocr, box);
        int split = info.find("|");
        course* curr = new course();
        curr->name = info.substr(0, split);
        curr->location = info.substr(split+1);
        cout << curr->name << " | " << curr->location << endl;
        //probably want to have courses with same name, location, etc. be considered the same
        parseDays(curr, v_line, boundingBox);
        parseTimes(curr, h_line, boundingBox);
      }
    }
    // resize image
    image.copyTo(image, jpgMask);
    cv::resize(image, image, cv::Size(), 0.75, 0.75);
    // Display the image with detected boxes
    cv::imshow("Detected Objects", image);
    cv::waitKey(0);

    // Cleanup
    ocr->End();
    delete ocr;

    return res;
}

/*
use Tesseract to extract text from an image
i use this to extract the text from each course block
*/
string parseWords(tesseract::TessBaseAPI* ocr, cv::Mat& obj) {
  // Set the image for OCR
  ocr->SetImage(obj.data, obj.cols, obj.rows, 1, obj.step);
  // Extract text
  string text = string(ocr->GetUTF8Text());
  int i = text.find("\n");
  //cout << text.substr(0, i) << "|" << text.substr(i+1, text.length()-i-2) << endl;
  
  string res(text.substr(0, i) + "|" + text.substr(i+1, text.length()-i-2));
  return res;
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

void parseTimes(course* curr, vector<int>& h_lines, cv::Rect location) {
  // use bin-search
  // i don't want to manually map each horizontal line to a time, so i'll just add times
  auto it = lower_bound(h_lines.begin(), h_lines.end(), location.y);
  // std::lower_bound will find iterator after the requested value if it does not exist
  if (*it != location.y)
    it--;  
  int index = it - h_lines.begin();
  cout << "iterator index " << index << endl;
  //cout << curr->days << endl << endl;
  bool half_hour = true;
  int hour = 8;
  for (int i = 1; i < index; i++) {
    half_hour = !half_hour;
    if (!half_hour)
      hour++;
    if (hour > 12)
      hour -= 12;
  }
  string start_min = (half_hour) ? "30" : "00";
  string start_time = to_string(hour) + ":" + start_min;
  curr->start_time = start_time;
  cout << "time at " << start_time << endl;

  while (*it < location.y + location.height) {
    it++;
    half_hour = !half_hour;
    if (!half_hour)
      hour++;
    if (hour > 12)
      hour -= 12;
  }
  string end_min = (half_hour) ? "20" : "50";
  string end_time = to_string(hour) + ":" + end_min;
  curr->end_time = end_time;
  cout << "time ends " << end_time << endl << endl;
}

void parseDays(course* curr, vector<int>& v_lines, cv::Rect location) {
  // use bin-search
  // there should only be 7 lines total no matter what
  auto it = lower_bound(v_lines.begin(), v_lines.end(), location.x);
  // std::lower_bound will find iterator after the requested value if it does not exist
  if (*it != location.x)
    it--;
  curr->days = all_days[(it - v_lines.begin()) - 1]; // -1 to skip the first vertical line
  //cout << "iterator index " << it - v_lines.begin() << endl;
  //cout << curr->days << endl << endl;
}

// searching for the nearest value to target
// since the target likely does not exist in the vector
// but the target is also likely to be slightly larger than the nearest element
int bin_search(vector<int>& vec, int target) {
  if (vec.size() == 0)
    return -1;
  return 0;
}


/*
modified 4/23/25 to restart progress
need to figure out how to curl the html page source into a string
*/ 
// taken from https://stackoverflow.com/a/45017565
size_t write_callback(char *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void curl_test() {
  String url("https://www.washington.edu/students/timeschd/AUT2024/cse.html");
  
  CURL* curl;
  CURLcode res;
  std::string html_content;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  
  if (curl) {
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html_content);
      res = curl_easy_perform(curl);
      
      if (res != CURLE_OK) {
          std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
      }
      
      curl_easy_cleanup(curl);
      cout<<"html results: " << html_content.size() <<endl;
      cerr<<"help me"<<endl;
      parse_and_navigate(html_content);
      //cout<<html_content<<endl;

  }
}

void xml_test(string& html) {
  htmlDocPtr doc = htmlReadMemory(html.c_str(), html.size(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
  // document.getElementsByName("cse121")[0].closest("table").nextElementSibling.getElementsByTagName("pre")[0].innerText
  if (doc == NULL) {
      std::cerr << "Failed to parse HTML document" << std::endl;
      return;
  }

}

// Helper function to move to the next element sibling
xmlNodePtr get_next_element_sibling(xmlNodePtr node) {
    while (node != nullptr) {
        node = node->next;
        if (node && node->type == XML_ELEMENT_NODE) return node;
    }
    return nullptr;
}

// Helper function to find the closest ancestor with a specific tag
xmlNodePtr find_closest_ancestor(xmlNodePtr node, const char* tag) {
    while (node != nullptr) {
        if (node->type == XML_ELEMENT_NODE && xmlStrEqual(node->name, BAD_CAST tag))
            return node;
        node = node->parent;
    }
    return nullptr;
}

void parse_and_navigate(const string& html) {
    htmlDocPtr doc = htmlReadMemory(html.c_str(), html.size(), NULL, NULL,
                                    HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (!doc) {
        std::cerr << "Failed to parse HTML.\n";
        return;
    }

    // Create XPath context
    xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
    if (!ctx) {
        xmlFreeDoc(doc);
        std::cerr << "Failed to create XPath context.\n";
        return;
    }

    // Find the first element with name="test_name"
    xmlXPathObjectPtr name_nodes = xmlXPathEvalExpression(BAD_CAST "//*[@name='cse469']", ctx);
    if (!name_nodes || name_nodes->nodesetval->nodeNr == 0) {
        std::cerr << "No element found with name='test_name'\n";
        return; //goto cleanup;
    }

    xmlNodePtr target = name_nodes->nodesetval->nodeTab[0];

    // Find the closest <table> ancestor
    xmlNodePtr table = find_closest_ancestor(target, "table");
    if (!table) {
        std::cerr << "No ancestor <table> found.\n";
        return; //goto cleanup;
    } else {
      cout<<"table contents: " << xmlNodeGetContent(table) << endl;
      
    }
    
    // Find the next element sibling of that table
    xmlNodePtr sibling = get_next_element_sibling(table);
    if (!sibling) {
        std::cerr << "No next element sibling found."<<endl;
        return; //goto cleanup;
    } else {
      cout<<"sibling contents: " << xmlNodeGetContent(sibling) <<endl;
      cout<<"next sibling contents: " << xmlNodeGetContent(get_next_element_sibling(sibling)) << endl;
      printf("%p\n", sibling->content);
    }

    // Set context node to the sibling to evaluate XPath relative to it
    ctx->node = sibling;

    // Find the first <pre> tag under that sibling
    xmlXPathObjectPtr pre_nodes = xmlXPathEvalExpression(BAD_CAST ".//pre", ctx);
    if (!pre_nodes || pre_nodes->nodesetval->nodeNr == 0) {
        std::cerr << "No <pre> tag found.\n";
        return; //goto cleanup;
    }

    xmlNodePtr pre = pre_nodes->nodesetval->nodeTab[0];

    // Print the text content of <pre>
    if (pre->children && pre->children->content) {
      String content((char*)xmlNodeGetContent(pre));
      
      std::cout << "Text content: " << content << endl;

    } else {
        std::cout << "Empty <pre> tag.\n";
    }

cleanup:
    if (name_nodes) xmlXPathFreeObject(name_nodes);
    if (pre_nodes) xmlXPathFreeObject(pre_nodes);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);
}


// document.getElementsByName("cse121")[0].closest("table").nextElementSibling.getElementsByTagName("pre")[0].innerText