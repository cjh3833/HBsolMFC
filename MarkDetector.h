/*
    2021660044 최광진
    HBSolution 개발용
    MarkDetector.h
    선언부
 */

#pragma once
#ifndef MARKDETECTOR_H
#define MARKDETECTOR_H
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;
using namespace cv::dnn;

struct PointInfo
{
    cv::Point2d Point;
    cv::Point2d Center;
    cv::Point2d Offset;
    cv::Point2d Derivative;
    double Magnitude;
    double Direction;

    void Update(const cv::Point2d& center)
    {
        Center = center;
        Offset = Point - center;
    }
};

class MarkDetector {
private:
    // 멤버 변수
    Point2d sum;
    vector<PointInfo> traindEdge;
    Size templateSize;
    int CannyMinThreshold;
    int CannyMaxThreshold;
    double minScore;
    double greediness;
    CascadeClassifier classifier;
    dnn::Net CNN;

public:
    // 생성자
    MarkDetector();
    MarkDetector(int min, int max, double ms, double gd);
    // getter-setter
    void setCannyMinThreshold(int input);
    void setCannyMaxThreshold(int input);
    void setMinScore(double input);
    void setGreediness(double input);
    void setClassfier(CascadeClassifier input);
    int loadClassifierXML(String input);
    void setCNN(Net input);
    int loadCNNFile(String input);
    int getCannyMinThreshold();
    int getCannyMaxThreshold();
    double getMinScore();
    double getGreediness();
    CascadeClassifier getClassifier();
    Net getCNN();
    // 사용자 지정 함수
    Mat trainEdge(Mat input_templ);
    Mat matchTemplateByEdge(Mat input_shapes);
    Mat objectDetectByCascadeClassifier(Mat input_shapes, double scale, int minNgb, int minSize, int maxSize);
    Mat objectDetectByCNN(Mat input_shapes, int input_size);
};

#endif