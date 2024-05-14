/*
    2021660044 �ֱ���
    HBSolution ���߿�
    MarkDetector.cpp
    ������
 */

#include "pch.h";
#include "MarkDetector.h"
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;


// -------------------------------- //
// ------------ ������ ------------ //


MarkDetector::MarkDetector() {
    this->sum = Point2d(0, 0);
    this->traindEdge.clear();
    this->CannyMinThreshold = 10;
    this->CannyMaxThreshold = 100;
    this->minScore = 0.9;
    this->greediness = 0.9;
}

MarkDetector::MarkDetector(int min, int max, double ms, double gd) {
    this->sum = Point2d(0, 0);
    this->traindEdge.clear();
    this->CannyMinThreshold = min;
    this->CannyMaxThreshold = max;
    this->minScore = ms;
    this->greediness = gd;
}


// ----------------------------------- //
// ---------- getter-setter ---------- //


void MarkDetector::setCannyMinThreshold(int input) {
    this->CannyMinThreshold = input;
}
void MarkDetector::setCannyMaxThreshold(int input) {
    this->CannyMaxThreshold = input;
}
void MarkDetector::setMinScore(double input) {
    this->minScore = input;
}
void MarkDetector::setGreediness(double input) {
    this->greediness = input;
}
void MarkDetector::setClassfier(CascadeClassifier input) {
    this->classifier = input;
}
int MarkDetector::loadClassifierXML(String input) {
    this->classifier.load(input);
    if (this->classifier.empty()) return -1;
    else return 0;
}
void MarkDetector::setCNN(Net input) {
    this->CNN = input;
}
int MarkDetector::loadCNNFile(String input) {
    this->CNN = dnn::readNet(input);
    if (this->CNN.empty()) return -1;
    else return 0;
}
int MarkDetector::getCannyMinThreshold() {
    return this->CannyMinThreshold;
}
int MarkDetector::getCannyMaxThreshold() {
    return this->CannyMaxThreshold;
}
double MarkDetector::getMinScore() {
    return this->minScore;
}
double MarkDetector::getGreediness() {
    return this->greediness;
}
CascadeClassifier MarkDetector::getClassifier() {
    return this->classifier;
}
Net MarkDetector::getCNN() {
    return this->CNN;
}


// -------------------------------------- //
// ---------- ����� ���� �Լ� ---------- //


// �Է¹��� Mat���� �ܰ��� ���� ����, traindEdge�� ������
// �Է¹��� Mat�� �߽ɰ� �ܰ��� ǥ�� �� ��ȯ.
Mat MarkDetector::trainEdge(Mat input_templ) {
    // Grayscale ��ȯ
    if (input_templ.channels() != 1) {
        cvtColor(input_templ, input_templ, COLOR_BGR2GRAY);
    }

    // ��������
    cv::GaussianBlur(input_templ, input_templ, Size(3, 3), 5);

    Mat templ_canny;
    cv::Canny(input_templ, templ_canny, this->CannyMinThreshold, this->CannyMaxThreshold);

    vector<vector<Point>> contours;
    cv::findContours(templ_canny, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

    // �ܰ��� ���� �н� ����
    Mat gx, gy;
    cv::Sobel(input_templ, gx, CV_64F, 1, 0, 3);
    cv::Sobel(input_templ, gy, CV_64F, 0, 1, 3);

    // ũ��, ���� ���
    Mat magnitude, direction;
    cv::cartToPolar(gx, gy, magnitude, direction);

    // �ʱ�ȭ
    this->sum = Point2d(0, 0);
    this->traindEdge.clear();
    this->templateSize = input_templ.size();

    for (int i = 0; i < contours.size(); i++) {
        for (int j = 0; j < contours[i].size(); j++) {
            Point2d cnt = contours[i][j];
            double fdx = gx.at<double>(cnt.y, cnt.x);
            double fdy = gy.at<double>(cnt.y, cnt.x);
            Point2d der = Point2d(fdx, fdy);
            double mag = magnitude.at<double>(cnt.y, cnt.x);
            double dir = direction.at<double>(cnt.y, cnt.x);

            PointInfo pointInfo;
            pointInfo.Point = cnt;
            pointInfo.Derivative = der;
            pointInfo.Direction = dir;
            pointInfo.Magnitude = mag == 0 ? 0 : 1 / mag;

            this->traindEdge.emplace_back(pointInfo);

            this->sum += cnt;
        }
    }

    Point2d center(this->sum.x / this->traindEdge.size(), this->sum.y / this->traindEdge.size());

    for (int i = 0; i < this->traindEdge.size(); i++) {
        this->traindEdge[i].Update(center);
    }

    cv::cvtColor(input_templ, input_templ, COLOR_GRAY2BGR);

    cv::drawContours(input_templ, contours, -1, Scalar(0, 255, 0), 2);
    cv::circle(input_templ, center, 2, Scalar(0, 0, 255), 1);

    return input_templ;
}

// ����� traindEdge�� �������� �Է¹��� Mat���� MatchTemplate ����
// ���� ������ ROI�� �߽ɺο� �ܰ��� ǥ�� �� ��ȯ
Mat MarkDetector::matchTemplateByEdge(Mat input_shapes) {
    // Grayscale ��ȯ
    if (input_shapes.channels() != 1) {
        cv::cvtColor(input_shapes, input_shapes, COLOR_BGR2GRAY);
    }

    // ��������
    cv::GaussianBlur(input_shapes, input_shapes, Size(3, 3), 5);

    Mat gx, gy;
    cv::Sobel(input_shapes, gx, CV_64F, 1, 0, 3);
    cv::Sobel(input_shapes, gy, CV_64F, 0, 1, 3);

    Mat magnitude, direction;
    cv::cartToPolar(gx, gy, magnitude, direction);

    double min_score = this->minScore;
    double greediness = this->greediness;

    int num_of_cordinates = this->traindEdge.size();
    double norm_min_score = min_score / num_of_cordinates;
    double norm_greediness = (1 - greediness * min_score) / (1 - greediness) / num_of_cordinates;
    double partial_score = 0;
    double result_score = 0;
    double sumOfCoords = 0;
    Point2d center = Point2d(0, 0);

    int h = input_shapes.rows;
    int w = input_shapes.cols;

    for (int i = 0; i < h; i++) {
        //cout << i << endl;
        for (int j = 0; j < w; j++) {
            double partial_sum = 0;
            for (int m = 0.0; m < num_of_cordinates; m++) {
                PointInfo item = this->traindEdge[m];
                int cntx = (int)(j + item.Offset.x);
                int cnty = (int)(i + item.Offset.y);

                double iTx = item.Derivative.x;
                double iTy = item.Derivative.y;
                if (cntx < 0 || cnty < 0 || cnty > h - 1 || cntx > w - 1)
                    continue;

                double iSx = gx.at<double>(cnty, cntx);
                double iSy = gy.at<double>(cnty, cntx);

                if ((iSx != 0 || iSy != 0) && (iTx != 0 || iTy != 0))
                {
                    double mag = magnitude.at<double>(cnty, cntx);
                    double mat_grad_mag = mag == 0 ? 0 : 1 / mag; // 1/��(dx��+dy��)
                    partial_sum += ((iSx * iTx) + (iSy * iTy)) * (item.Magnitude * mat_grad_mag);
                }

                sumOfCoords = m + 1;
                partial_score = partial_sum / sumOfCoords;
                if (partial_score < std::min((min_score - 1) + norm_greediness * sumOfCoords, norm_min_score * sumOfCoords))
                    break;
            }

            if (partial_score > result_score)
            {
                result_score = partial_score;
                center.x = j;
                center.y = i;
            }
        }
    }

    int width = this->templateSize.width;
    int height = this->templateSize.height;
    int x = center.x - width / 2;
    int y = center.y - height / 2;

    // ���� ��Ż ó��
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + width > input_shapes.cols) width = input_shapes.cols - x;
    if (y + height > input_shapes.rows) height = input_shapes.rows - y;

    Rect ROI = Rect(x, y, width, height);

    vector<vector<Point>> contours;
    Mat ROI_Canny;
    cv::Canny(input_shapes(ROI), ROI_Canny, this->CannyMinThreshold, this->CannyMaxThreshold);
    cv::findContours(ROI_Canny, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    cv::cvtColor(input_shapes, input_shapes, cv::COLOR_GRAY2BGR);
    cv::drawContours(input_shapes(ROI), contours, -1, Scalar(0, 255, 0), 1);
    cv::circle(input_shapes, center, 2, Scalar(0, 0, 255), 1);
    return input_shapes;
}

Mat MarkDetector::objectDetectByCascadeClassifier(Mat input_shapes, double scale, int minNgb, int minSize, int maxSize) {
    // Grayscale ��ȯ
    if (input_shapes.channels() != 1) {
        cv::cvtColor(input_shapes, input_shapes, COLOR_BGR2GRAY);
    }

    //���� ����
    GaussianBlur(input_shapes, input_shapes, Size(3, 3), 5);

    vector<Rect> center;
    this->classifier.detectMultiScale(input_shapes, center, scale, minNgb, 0, Size(minSize, minSize), Size(maxSize, maxSize));

    cv::cvtColor(input_shapes, input_shapes, cv::COLOR_GRAY2BGR);

    for (int i = 0; i < center.size(); i++) {

        Mat ROI_Canny;
        Mat ROI = input_shapes(center[i]);
        cv::Canny(ROI, ROI_Canny, this->CannyMinThreshold, this->CannyMaxThreshold);

        vector<vector<Point>> contours;
        cv::findContours(ROI_Canny, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

        int cx = center[i].x + center[i].width / 2;
        int cy = center[i].y + center[i].height / 2;
        cout << Point(cx, cy) << endl;
        cv::drawContours(ROI, contours, -1, Scalar(0, 255, 0), 1);
        cv::circle(input_shapes, Point(cx, cy), 2, Scalar(0, 0, 255), 1);
    }

    return input_shapes;
}

// ������
//
Mat MarkDetector::objectDetectByCNN(Mat input_shapes, int input_size) {
    if (input_shapes.channels() != 1) {
        cv::cvtColor(input_shapes, input_shapes, cv::COLOR_BGR2GRAY);
    }

    //���� ����
    GaussianBlur(input_shapes, input_shapes, Size(3, 3), 5);

    Mat input_threshold;
    adaptiveThreshold(input_shapes, input_threshold, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 99, 5);

    Mat ROI, CNNResult;
    vector<vector<Point>> contours;
    findContours(input_threshold, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

    cv::cvtColor(input_shapes, input_shapes, cv::COLOR_GRAY2BGR);

    /*
    for (int i = 0; i < contours.size(); i++) {
        // �� �ʱ�ȭ
        ROI.setTo(0); CNNResult.setTo(0);

        // ���ɱ��� ����, �켱 ���� ���������� ��� �ܰ����� ǥ����.
        Rect ROI_rect = boundingRect(contours[i]);
        drawContours(input_shapes, contours, i, Scalar(0, 0, 255), 1);

        // �ʹ� ���� ���µ��� ������.
        if (ROI_rect.width > 25 and ROI_rect.height > 25) {
            // ���ɱ��� ���� �� 5px ũ���� Zero-Padding ����.

            rectangle(input_shapes, ROI_rect, Scalar(255, 0, 0), 1);

            ROI = input_threshold(ROI_rect);
            copyMakeBorder(ROI, ROI, 5, 5, 5, 5, BORDER_CONSTANT, Scalar(0));

            // 4���� ������ Blob���� ��ȯ��. �̹� ����ȭ�� �̹����̹Ƿ� 1�� ����.
            // �ش� �Ű���� �Է����� ũ�Ⱑ 96*96�̹Ƿ� ũ�� ����
            Mat blob = blobFromImage(ROI, 1, Size(80, 80));

            // �Ű���� �����͸� ���ε�
            this->CNN.setInput(blob);

            // ������ �߷�(inference) ����. ��� ���� 1x1 ũ���� ����̸�, float(32bit) ���� ����.
            // 0 �̸� �߽ɽ��ڷ� �Ǻ�(Cross), 1�̸� �߽����� �ƴ�(Not_Cross)
            CNNResult = this->CNN.forward();

            // ��ȯ�� ��� ��Ŀ��� ���� �󺧰� Ȯ���� �̾Ƴ�.
            Point max_loc;
            double max_val;
            minMaxLoc(CNNResult, NULL, &max_val, NULL, &max_loc);

            // ������ -> �� ��ȯ
            String result_string;
            if (max_loc.x == 0) {
                result_string = "Cross";
            }
            else {
                result_string = "not_Cross";
            }

            cout << " - ���� : " << result_string << "  Ȯ��(%) : " << round(max_val * 100) << endl;

            // �ܰ��� �׸���
            if (max_loc.x == 0 and max_val >= 0.70) {
                drawContours(input_shapes, contours, i, Scalar(0, 255, 0), 3);
                //center_location = i;
            }
            else {
                drawContours(input_shapes, contours, i, Scalar(0, 0, 255), 1);
            }
        }
    }*/

    return input_shapes;
}