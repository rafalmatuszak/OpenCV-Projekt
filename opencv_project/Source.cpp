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
ofstream stdevs2("stdevs.csv");

//MOG2
Ptr<BackgroundSubtractorMOG2> pMOG2 = createBackgroundSubtractorMOG2(500, 64);

double odchylenie(vector<double> v);
bool fallDetect(double cmot,double theta,double ab,double y);

int main()
{
	//initial Mat's
	//poczatkowe Mat-y
	Mat frame;
	Mat gray;
	Mat mhi;
	Mat eroded, dilated;

	stdevs2 << "Theta,ab,y\n";

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
		stringstream dev_theta, dev_atob,dev_y;
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
		blur(bin_mask.clone(), bin_mask, Size(3, 3));

		//calculcating MHI
		//obliczenie MHI - Motion History
		updateMotionHistory(bin_mask, mhi, timestamp, MHI_DURATION);

		//calculating Cmotion - finding contours
		//policzenie bialych pikseli w obu obrazach - wykrywany kontury
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

		
		//fitting ellipse to the biggest blob
		//nalozenie elipsy na najwiekszy kontur
		RotatedRect ell;
		if (contours.size() > 0 & contours[largest_id].size() >= 5) {
			ell = fitEllipse(contours[largest_id]);
			ellipse(frame, ell, Scalar(0, 0, 255), 2, 8);
			angles.push_back(ell.angle);
			a_to_b.push_back(ell.size.width / ell.size.height);
			
			//dodanie do wektorow wartosci y
			if (ycount >= 30) {
				ycount = 0;
				ycoord.clear();
			}
			ycoord.push_back(ell.center.y);
			ycount += 1;
		}

	
		//usuwany jest pierwszy element, jesli jest pusty
		//if(isnan(a_to_b[0])) {
		//	a_to_b.erase(a_to_b.begin());
		//}

		if (angles.size() > 1 & a_to_b.size() > 1) {

			double std_angle = 0.0;
			double std_a_to_b = 0.0;
			double std_y = 0.0;

			std_angle = odchylenie(angles);
			std_a_to_b = odchylenie(a_to_b);
			std_y = odchylenie(ycoord);

			//na razie wpisanie tego do pliku
			stdevs << "Std_angle: " << std_angle << ", std_a_to_b: " << std_a_to_b << ", std_y: " << std_y << "\n";
			stdevs2 << std_angle << "," << std_a_to_b << "," << std_y << "\n";
			dev_theta << "Dev theta: " << std_angle;
			dev_atob << "Dev atob: " << std_a_to_b;
			dev_y << "Dev y: " << std_y;

			if (fallDetect(mot_coeff, std_angle, std_a_to_b, std_y) == true) {
				putText(frame, "FALL DETECTED", Point(frame.cols - 100,10), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 255, 0), 1, 8);
			}

		}

		//wypisanie na ekran
		stringstream ss,y;
		ss << "Motion coeff: " << mot_coeff;
		putText(frame, ss.str(), Point(10, 10), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 255), 1, 8);
		putText(frame, dev_theta.str(), Point(10, 30), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 255), 1, 8);
		putText(frame, dev_atob.str(), Point(10, 50), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 255), 1, 8);
		putText(frame, dev_y.str(), Point(10, 70), FONT_HERSHEY_DUPLEX, 0.5, Scalar(0, 0, 255), 1, 8);

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
	return sqrt(E / v.size());
}


//sprawdzenie warunkow upadku
bool fallDetect(double cmot, double theta, double ab, double y)
{
	//wartosci wyznaczone empirycznie - przy lepszym dopasowaniu elips mozna by sie pokusic o unormowanie tych wartosci
	if (cmot > 0.12) {
		if (theta > 0 & ab > 0) {
			if (y > 0) {
				return true;
			}
		}
	}
	else {
		return false;
	}
	
}


