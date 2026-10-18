#include "stdafx.h"
#include "common.h"
#include <opencv2/core/utils/logger.hpp>

#include  <fstream>
#include <iostream>
#include <random>
#include <float.h>

using namespace std;
wchar_t* projectPath;

struct Center {
	Vec3f color;   // l,a,b
	Point2f pos;   // x,y
};


void computeGradient(const Mat& lab, Mat& grad)
{
    vector<Mat> channels;
    split(lab, channels);
    Mat l = channels[0];

    Mat gx, gy;
    Sobel(l, gx, CV_32F, 1, 0, 3);
    Sobel(l, gy, CV_32F, 0, 1, 3);
    magnitude(gx, gy, grad);
}


vector<Center> initializeCenters(const Mat& lab, const Mat& grad, int S)
{
    int rows = lab.rows, cols = lab.cols;
    vector<Center> centers;

    for (int y = S / 2; y < rows; y += S) 
    {
        for (int x = S / 2; x < cols; x += S) 
        {
            Point best = Point(x, y);
            float minG = grad.at<float>(y, x);

            for (int dy = -1; dy <= 1; dy++) 
            {
                for (int dx = -1; dx <= 1; dx++) 
                {
                    int yy = y + dy, xx = x + dx;
                    if (xx < 0 || xx >= cols || yy < 0 || yy >= rows) 
                        continue;

                    float g = grad.at<float>(yy, xx);
                    if (g < minG) {
                        minG = g;
                        best = Point(xx, yy);
                    }
                }
            }
            Vec3f c = lab.at<Vec3f>(best);
            centers.push_back({ c, Point2f((float)best.x, (float)best.y) });
        }
    }
    return centers;
}


float computeDistance(const Vec3f& c1, const Vec3f& c2, const Point2f& p1, const Point2f& p2, float invwt)
{
    float d_lab = norm(c1 - c2);
    float d_xy = 5 * norm(p1 - p2);
    return d_lab + invwt * d_xy;
}


void assignLabels(const Mat& lab, const vector<Center>& centers, Mat& labels, Mat& dists, float invwt, int S)
{
    labels = Mat::ones(lab.size(), CV_32S) * -1;
    dists = Mat::ones(lab.size(), CV_32F) * FLT_MAX;

    for (int i = 0; i < centers.size(); i++) 
    {
        const Center& C = centers[i];
        int x0 = max(int(C.pos.x - S), 0);
        int x1 = min(int(C.pos.x + S), lab.cols - 1);
        int y0 = max(int(C.pos.y - S), 0);
        int y1 = min(int(C.pos.y + S), lab.rows - 1);

        for (int y = y0; y <= y1; y++) 
        {
            for (int x = x0; x <= x1; x++) 
            {
                float D = computeDistance(lab.at<Vec3f>(y, x), C.color,
                    Point2f((float)x, (float)y), C.pos,
                    invwt);

                if (D < dists.at<float>(y, x)) {
                    dists.at<float>(y, x) = D;
                    labels.at<int>(y, x) = i;
                }
            }
        }
    }
}


void updateCenters(const Mat& lab, vector<Center>& centers, const Mat& labels)
{
    vector<Vec3f> sumColor(centers.size(), Vec3f(0, 0, 0));
    vector<Point2f> sumPos(centers.size(), Point2f(0, 0));
    vector<int> count(centers.size(), 0);

    for (int y = 0; y < lab.rows; y++) 
    {
        for (int x = 0; x < lab.cols; x++) 
        {
            int label = labels.at<int>(y, x);
            if (label >= 0) {
                sumColor[label] += lab.at<Vec3f>(y, x);
                sumPos[label] += Point2f((float)x, (float)y);
                count[label]++;
            }
        }
    }

    for (int i = 0; i < centers.size(); i++) 
    {
        if (count[i] > 0) {
            centers[i].color = sumColor[i] / float(count[i]);
            centers[i].pos = sumPos[i] / float(count[i]);
        }
    }
}


Mat colorSuperpixels(const Mat& img, const Mat& labels, int K)
{
    vector<Vec3d> sumCol(K, Vec3d(0, 0, 0));
    vector<int> cntCol(K, 0);

    for (int y = 0; y < img.rows; y++) 
    {
        for (int x = 0; x < img.cols; x++) 
        {
            int label = labels.at<int>(y, x);

            if (label >= 0 && label < K) {
                Vec3b bgr = img.at<Vec3b>(y, x);
                sumCol[label] += Vec3d(bgr[0], bgr[1], bgr[2]);
                cntCol[label]++;
            }
        }
    }

    vector<Vec3b> meanCol(K);
    for (int i = 0; i < K; i++) {
        if (cntCol[i] > 0) {
            Vec3d avg = sumCol[i] / double(cntCol[i]);
            meanCol[i] = Vec3b(
                static_cast<uchar>(avg[0]),
                static_cast<uchar>(avg[1]),
                static_cast<uchar>(avg[2])
            );
        }
    }

    Mat colorized(img.size(), img.type());
    for (int y = 0; y < img.rows; y++) {
        for (int x = 0; x < img.cols; x++) {
            int lbl = labels.at<int>(y, x);
            colorized.at<Vec3b>(y, x) =
                (lbl >= 0 && lbl < K) ? meanCol[lbl] : Vec3b(0, 0, 0);
        }
    }

    return colorized;
}


int main() 
{
	Mat_<Vec3b> img = imread("Data_Set/man.jpg", IMREAD_COLOR);
    imshow("Original image", img);

    int K = 100;
    float m = 20;

    Mat lab;
    cvtColor(img, lab, COLOR_BGR2Lab);
    lab.convertTo(lab, CV_32F);

    int S = int(sqrt((img.rows * img.cols) / double(K)));
    float invwt = m / float(S);

    Mat grad;
    computeGradient(lab, grad);

    vector<Center> centers = initializeCenters(lab, grad, S);

    Mat labels, dists;

    for (int i = 0; i < 50; i++) 
    {
        assignLabels(lab, centers, labels, dists, invwt, S);
        updateCenters(lab, centers, labels);
    }

    Mat mask = Mat::zeros(img.size(), CV_8U);
    for (int y = 1; y < img.rows - 1; y++) 
    {
        for (int x = 1; x < img.cols - 1; x++) 
        {
            int l0 = labels.at<int>(y, x);
            if (l0 != labels.at<int>(y, x + 1) ||
                l0 != labels.at<int>(y + 1, x) ||
                l0 != labels.at<int>(y, x - 1) ||
                l0 != labels.at<int>(y - 1, x))
            {
                mask.at<uchar>(y, x) = 255;
            }
        }
    }

    Mat colorized = colorSuperpixels(img, labels, (int)centers.size());
    imshow("Superpixel-filled", colorized);

    Mat overlay = colorized.clone();
    overlay.setTo(Vec3b(0, 0, 255), mask);
    imshow("Color + Boundaries", overlay);


    waitKey();


	return 0;
}