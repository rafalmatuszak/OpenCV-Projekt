#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv/cv.h>
#include <opencv2/optflow.hpp>
#include<iostream>
#include<fstream>
#include<conio.h>
#include<cmath>

#define MHI_DURATION 0.5

using namespace cv;
using namespace cv::motempl;
using namespace std;

char key;
char mode;
short frames=0;
short ycount = 0;

//text file for writing Cmotion
//plik tekstowy do zapisywania wspolczynnika C
ofstream coeff("motion_coeff.txt");
ofstream stdevs("stanard_deviations.txt");

//MOG2
Ptr<BackgroundSubtractorMOG2> pMOG2 = createBackgroundSubtractorMOG2(500, 64);

double odchylenie(vector<double> v);

int main()
{
	//initial Mat's
	//poczatkowe Mat-y
	Mat frame;
	Mat gray;
	Mat mhi;
	Mat eroded, dilated;

	VideoCapture cap("video.mp4");
	//VideoCapture cap;

	//cap.open(0);

	//wektory do odchylen
	vector<double> angles;
	vector<double> a_to_b;
	vector<double> ycoord;

	while ((char)key != 'q' && (char)key != 27) {
		key = 0;
		frames += 1;
		double timestamp = (double)clock() / CLOCKS_PER_SEC;

		//securing input
		//Zabezpieczenie przed brakiem obrazu zrodlowego
		if (!cap.isOpened()) {
			cerr << "Zrodlo wideo nie jest zdefiniowane!";
			exit(EXIT_FAILURE);
		}

		//securing next frame
		//Zabezpieczenie przed brakiem nastepnej ramki
		if (!cap.read(frame)) {
			cerr << "Nie mozna odczytac kolejnej klatki!";
			exit(EXIT_FAILURE);
		}

		//main algorithm
		//dodatkowe Mat-y i zmienne pomocnicze
		Mat eroded, dilated;
		Mat mask, bin_mask;
		double mot_coeff = 0.0;
		double mhi_sum = 0.0;
		double fg_sum = 0.0;

		//resizing mhi
		//dostosowanie formatu i wymiaru mhi
		Size size = frame.size();
		if (mhi.size() != size) {
			mhi = Mat::zeros(size, CV_32F);
		}

		//morphological cleaning
		//"oczyszczenie" obrazu  - morfologia
		erode(frame, eroded, Mat(), Point(-1, -1), 3);
		dilate(eroded, dilated, Mat(6, 6, CV_32F), Point(-1, -1), 3);

		//MOG2 applying to imgae
		//nalozenie maski do mikstur gaussowskich
		pMOG2->apply(dilated, mask);

		//threshhold
		//binaryzacja i poszukiwanie najwiekszego konturu
		threshold(mask, bin_mask, 10, 255, THRESH_BINARY);

		//calculcating MHI
		//obliczenie MHI - Motion History
		updateMotionHistory(bin_mask, mhi, timestamp, MHI_DURATION);

		//two ways on calculating Cmotion
		//policzenie bialych pikseli w obu obrazach - wykrywany ksztalt
		double white_mhi = (mhi.rows * mhi.cols) - countNonZero(mhi);
		double white_fg = (bin_mask.rows * bin_mask.cols) - countNonZero(bin_mask);
		
		mot_coeff = 1 - (white_mhi / white_fg);
		cout << mot_coeff << endl;
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

		RotatedRect ell;
		//fitting ellipse to the biggest blob
		//nalozenie elipsy na najwiekszy kontur
		if (contours.size() > 0 & contours[largest_id].size() >= 5) {
			ell = fitEllipse(contours[largest_id]);
			ellipse(frame, ell, Scalar(0, 0, 255), 2, 8);
		}

		//dopiero tu sprawdzamy drugi krok algorytmu

		angles.push_back(ell.angle);
		a_to_b.push_back(ell.size.width / ell.size.height);
	
		if(isnan(a_to_b[0])) {
			a_to_b.erase(a_to_b.begin());
		}

		if (ycount >= 10){
			ycount = 0;
			ycoord.clear();
		}
		ycoord.push_back(ell.center.y);
		ycount += 1;

		double std_angle = 0.0;
		double std_a_to_b = 0.0;

		if (angles.size() > 1 & a_to_b.size() > 1) {
			std_angle = odchylenie(angles);
			std_a_to_b = odchylenie(a_to_b);
		}

		//sprawdzenie warunkow algorytmu
		if (mot_coeff > 0.12) {
			stdevs << "Std_angle: " << std_angle << ", std_a_to_: b" << std_a_to_b << "\n";
		}



		//wypisanie na ekran
		stringstream ss, ss1, ss2;
		ss << "Motion coeff: " << mot_coeff;
		ss1 << "Dev theta: " << std_angle;
		ss2 << "Dev atob: " << std_angle;
		putText(frame, ss.str(), Point(10, 10), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 255), 1, 8);
		putText(frame, ss1.str(), Point(10, 30), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 255), 1, 8);
		putText(frame, ss2.str(), Point(10, 50), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 255), 1, 8);

		//writing down the Cmotion to file
		coeff << "Motion coefficient at frame " << frames << ": " << mot_coeff << "\n";

		//showing results
		imshow("Original image", frame);
		imshow("MOG2 mask", bin_mask);
		imshow("MHI", mhi);
		

		//przerwa na wcisniecie klawisza
		key = (char)waitKey(50);
	}
	cap.release();
	cv::destroyAllWindows();
	return 0;
}


//odchylenie
double odchylenie(vector<double> v)
{
	int sum = 0;
	for (int i = 0; i<v.size(); i++)
		sum += v[i];
	double ave = sum / v.size();

	double E = 0;
	for (int i = 0; i<v.size(); i++)
		E += (v[i] - ave)*(v[i] - ave);
	return sqrt(1 / v.size()*E);
}