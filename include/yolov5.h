#ifndef ANOSERVER_INCLUDE_YOLOV5_H_
#define ANOSERVER_INCLUDE_YOLOV5_H_
#include <iostream>
#include <opencv2/opencv.hpp>
#include "RegistryFactory.h"
#include <math.h>
#include <time.h>
#include <fstream>
#include "logger.h"
#include "DeepNet.h"

using namespace cv;
using namespace dnn;



struct Output {
	int id;
	float confidence;
	cv::Rect box;
};

class Yolov5:public Recognition
{
public:
	Yolov5() {}
	~Yolov5() {}
	cv::Mat preprocess(cv::Mat& img,cv::Size new_shape,float& r,float& dw, float& dh);
	
	float * trtProcess(int batch_size, int input_h, int input_w, Mat &img,DeepNet * deepnet_ ,Mat & SrcImg);

	bool Detect(cv::Mat &SrcImg, std::vector<Output> &output, int type, std::string &detectres,DeepNet * deepnet_ );

	void drawPred(Mat &img, cv::Mat &dst, std::vector<Output> result, std::vector<Scalar> color);
	int Recognize(cv::Mat &src,std::string Ano_type,std::string& res, cv::Mat& dst,void *engine);

	DeepNet * deepnet_ ;

private:
	float nmsThreshold = 0.45;
	float boxThreshold = 0.25;
	float classThreshold = 0.25;
	float data_mean[3]={0,0,0};
    float data_std[3]={255,255,255};
	std::vector<std::string> className;
	std::vector<std::string> className_gw = {"bj_bpmh", "bj_bpzc", "bj_bpps", "bj_wkps", "bjdsyc", "ywzt_yfyc", "hxq_gjtps", \
										  "hxq_gjbs", "jyz_pl", "sly_bjbmyw", "xmbhyc", "xmbhzc", "yw_nc", "yw_gkxfw", "kgg_ybh", \
										  "kgg_ybf", "kgg_ybk", "wcaqm", "aqmzc", "wcgz", "gzzc", "xy", "sly_dmyw", "gbps"};
	// std::vector<std::string> className_jsls = {"jsls"};


std::vector<std::string> className_jsls = {
            "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
            "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
            "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
            "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
            "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
            "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
            "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
            "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
            "hair drier", "toothbrush"
    };


};


#endif
