#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv/cv.h>
#include <opencv2/optflow.hpp>
#include<iostream>
#include<fstream>
#include<conio.h>

#define MHI_DURATION 5

using namespace cv;
using namespace cv::motempl;
using namespace std;

char key;

int main()
{
	Mat frame;
	Mat mask, bin_mask;
	Mat bg;
	Mat gray;
	Mat mhi;
	Mat eroded, dilated;


	//VideoCapture cap("video.mp4");
	VideoCapture cap;

	cap.open(0);
	//MOG2 
	Ptr<BackgroundSubtractorMOG2> pMOG2 = createBackgroundSubtractorMOG2(3000, 64);
	ofstream coeff("motion_coeff.txt");

	while ((char)key != 'q' && (char)key != 27) {
		key = 0;
		double timestamp = (double)clock() / CLOCKS_PER_SEC;
		double mot_coeff = 0.0;
		double mhi_sum = 0.0;
		double fg_sum = 0.0;

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


		//dostosowanie formatu i wymiaru mhi
		Size size = frame.size();
		if (mhi.size() != size) {
			mhi = Mat::zeros(size, CV_32F);
		}

		//"oczyszczenie" obrazu  - morfologia
		erode(frame.clone(), eroded, Mat(), Point(-1, -1), 2);
		dilate(eroded, dilated, Mat(6, 6, CV_32F), Point(-1, -1), 2);

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
			ellipse(frame, ell, Scalar(255, 0, 0), 2, 8);
		}


		stringstream ss;
		ss << "Motion coeff: " << mot_coeff;

		putText(frame, ss.str(), Point(10, 10), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 255), 1, 8);

		//wyswietlanie rezultatow
		imshow("Original image", frame);
		imshow("MOG2 mask", mask);
		imshow("MHI", mhi);
		coeff << "Motion coefficient " << mot_coeff << "\n";
		key = (char)waitKey(50);
	}
	coeff.close();
	cap.release();
	cv::destroyAllWindows();
	return 0;
}