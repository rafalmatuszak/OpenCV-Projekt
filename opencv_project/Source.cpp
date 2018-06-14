#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv/cv.h>
#include <opencv2/optflow.hpp>
#include<iostream>
#include<fstream>
#include<conio.h>

#define MHI_DURATION 3

using namespace cv;
using namespace cv::motempl;
using namespace std;

char key;
short frames=0;

bool detectFall(Mat frame, Mat mhi, double timestamp, short frames);

//MOG2
Ptr<BackgroundSubtractorMOG2> pMOG2 = createBackgroundSubtractorMOG2(1000, 64);

int main()
{
	Mat frame;
	Mat mask, bin_mask;
	Mat bg;
	Mat gray;
	Mat mhi;
	Mat eroded, dilated;

	VideoCapture cap("video.mp4");
	//VideoCapture cap;

	//w while powinna dzialac sama funkcja detectFall
	while ((char)key != 'q' && (char)key != 27) {
		key = 0;
		frames += 1;
		double timestamp = (double)clock() / CLOCKS_PER_SEC;

		//Zabezpieczenie przed brakiem obrazu zrodlowego
		if (!cap.isOpened()) {
			cerr << "Zrodlo wideo nie jest zdefiniowane!";
			exit(EXIT_FAILURE);
		}

		//Zabezpieczenie przed brakiem nastepnej ramki
		if (!cap.read(frame)) {
			cerr << "Nie mozna odczytac kolejnej klatki!";
			exit(EXIT_FAILURE);
		}

		detectFall(frame, mhi, timestamp,frames);

		key = (char)waitKey(50);
	}
	cap.release();
	cv::destroyAllWindows();
	return 0;
}


bool detectFall(Mat frame,Mat mhi,double timestamp,short frames) {
	
	//dodatkowe Mat-y i zmienne pomocnicze
	Mat eroded, dilated;
	Mat mask,bin_mask;
	double mot_coeff = 0.0;
	double mhi_sum = 0.0;
	double fg_sum = 0.0;

	//plik tekstowy do zapisywania wspolczynnika C
	ofstream coeff("motion_coeff.txt");

	//dostosowanie formatu i wymiaru mhi
	Size size = frame.size();
	if (mhi.size() != size) {
		mhi = Mat::zeros(size, CV_32F);
	}

	//"oczyszczenie" obrazu  - morfologia
	erode(frame, eroded, Mat(), Point(-1, -1), 3);
	dilate(eroded, dilated, Mat(6, 6, CV_32F), Point(-1, -1), 3);

	//nalozenie maski do mikstur gaussowskich
	pMOG2->apply(dilated, mask);

	//obliczenie MHI - Motion History
	updateMotionHistory(mask, mhi, timestamp, MHI_DURATION);


	//suma dla obrazu pierwszoplanowego i mhi
	for (int i = 0; i < mhi.rows; i++) {
		for (int j = 0; j < mhi.cols; j++) {
			mhi_sum += mhi.at<float>(i, j);
			fg_sum += mask.at<uchar>(i, j);
		}
	}

	//obliczenie wspolczynnika ruchu
	mot_coeff = fg_sum / mhi_sum;

	//binaryzacja i poszukiwanie najwiekszego konturu
	threshold(mask, bin_mask, 10, 255, THRESH_BINARY);
	vector<vector<Point>> contours;
	double largest_contour = 0;
	int largest_id = 0;
	findContours(bin_mask, contours, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
	for (int i = 0; i < contours.size(); i++) {
		double area = contourArea(contours[i], false);
		if (area > largest_contour) {
			largest_contour = area;
			largest_id = i;
		}
	}

	//nalozenie elipsy na najwiekszy kontur
	if (contours.size() > 0 & contours[largest_id].size() >= 5) {
		RotatedRect ell;
		ell = fitEllipse(contours[largest_id]);
		ellipse(frame, ell, Scalar(0, 0, 255), 2, 8);
	}

	//wypisanie wspolczynnika C
	stringstream ss;
	ss << "Motion coeff: " << mot_coeff;
	putText(frame, ss.str(), Point(10, 10), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 255), 1, 8);

	//dopisanie wspolczynnika C do pliku tekstowego
	coeff << "Motion coefficient at frame " << frames << ": " << mot_coeff << "\n";

	//wyswietlanie rezultatow
	imshow("Original image", frame);
	imshow("MOG2 mask", mask);
	imshow("MHI", mhi);

	return false;
}