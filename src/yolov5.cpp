#include "yolov5.h"

// float data640[3*640*640];
// float prob640[1*25200*6];

// float data896[3*896*896];
// float prob896[1*49980*29];


cv::Mat Yolov5::preprocess(cv::Mat &img,cv::Size  new_shape,float& r,float& dw, float& dh)
{
	cv::Mat image_blob;
    cv::cvtColor(img,image_blob,cv::COLOR_BGR2RGB);

    r=std::min(((float)new_shape.width/img.cols),((float)new_shape.height/img.rows));
    cv::Size new_unpad=cv::Size(int(round(r*img.cols)),int(round(r*img.rows)));
    dw=(new_shape.width-new_unpad.width)/2.0;
    dh=(new_shape.height-new_unpad.height)/2.0;
    cv::resize(image_blob,image_blob,new_unpad,cv::INTER_LINEAR);
    cv::copyMakeBorder(image_blob,image_blob,int(round(dh-0.1)),int(round(dh+0.1)),
                       int(round(dw-0.1)),int(round(dw+0.1)),
                       cv::BORDER_CONSTANT,cv::Scalar(0,0,0));


    image_blob.convertTo(image_blob,CV_32F);

    std::vector<cv::Mat> img_channel(3);
    cv::split(image_blob,img_channel);
    for(unsigned int i=0;i<img_channel.size();i++)
        img_channel[i].convertTo(img_channel[i],CV_32FC1,1.0/data_std[i],-data_mean[i]/data_std[i]);
    cv::merge(img_channel,image_blob);


	return image_blob;
}


cv::Mat Yolov5::preprocessImg(cv::Mat& img, int input_w, int input_h) {
    int w, h, x, y;
    float r_w = input_w / (img.cols*1.0);
    float r_h = input_h / (img.rows*1.0);
    if (r_h > r_w) {
        w = input_w;
        h = r_w * img.rows;
        x = 0;
        y = (input_h - h) / 2;
    } else {
        w = r_h * img.cols;
        h = input_h;
        x = (input_w - w) / 2;
        y = 0;
    }
    cv::Mat re(h, w, CV_8UC3);
    cv::resize(img, re, re.size(), 0, 0, cv::INTER_LINEAR);
    cv::Mat out(input_h, input_w, CV_8UC3, cv::Scalar(128, 128, 128));
    re.copyTo(out(cv::Rect(x, y, re.cols, re.rows)));
    return out;
}



void Yolov5::draw_objects(const cv::Mat& bgr, const std::vector<Output>& objects)
{
    static const char* class_names[] = {
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

    cv::Mat image = bgr.clone();

    for (size_t i = 0; i < objects.size(); i++)
    {
        const Output& obj = objects[i];

        fprintf(stderr, "%d = %.5f at %.2f %.2f %.2f x %.2f\n", obj.id, obj.confidence,
                obj.box.x, obj.box.y, obj.box.width, obj.box.height);

        cv::rectangle(image, obj.box, cv::Scalar(255, 0, 0));

        char text[256];
        sprintf(text, "%s %.1f%%", class_names[obj.id], obj.confidence * 100);

        int baseLine = 0;
        cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

        int x = obj.box.x;
        int y = obj.box.y - label_size.height - baseLine;
        if (y < 0)
            y = 0;
        if (x + label_size.width > image.cols)
            x = image.cols - label_size.width;

        cv::rectangle(image, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                      cv::Scalar(255, 255, 255), -1);

        cv::putText(image, text, cv::Point(x, y + label_size.height),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
    }

    imwrite("/home/aaeon/Anoserver_old/demo/out.jpg",image);
	printf("save !!!!!\n");

}



float * Yolov5::trtProcess(int batch_size, int input_h, int input_w , Mat &img, DeepNet * deepnet_ ,Mat & SrcImg){

		const char * images = "images";
        const char * output = "output0";
		int channels = 3;
		IRuntime * runtime;
		ICudaEngine * engine;
		IExecutionContext *context;
		int num=0;
		// float *data,*prob=nullptr;
		switch(input_h){
			case 896:
				runtime = deepnet_->runtime896_ ;
        		engine  = deepnet_->engine896_;
        		context = deepnet_->context896_ ;
				num = 1*49980*29;
			//	data = data896;
			//	prob = prob896;

				break;
			case 640:
				runtime = deepnet_->runtime640_ ;
        		engine  = deepnet_->engine640_;
        		context = deepnet_->context640_ ;
				//num = 1*25200*6;
				num = 1*25200*85;
			//    data = data640;
			//	prob = prob640;
				break;	
		}

        int32_t images_index = engine->getBindingIndex(images);
        int32_t output_index = engine->getBindingIndex(output);
		printf("images_inde=%d\n",images_index);
		printf("output_index=%d\n",output_index);

        std::cout<<engine->getNbBindings()<<std::endl;

        void * buffers[2];
        cudaMalloc(&buffers[images_index],batch_size*channels*input_h*input_w*sizeof (float));
        cudaMalloc(&buffers[output_index],batch_size*num*sizeof (float));

        cudaStream_t stream;
        cudaStreamCreate(&stream);

		float* data = new float[channels*input_h*input_w];


		for(int i = 0 ; i < input_h*input_w; i++){
            data[i] = img.at<Vec3f>(i)[0];
            data[i+input_h*input_w] = img.at<Vec3f>(i)[1];
            data[i+2*input_h*input_w] = img.at<Vec3f>(i)[2];

        }

		float *prob= new float[num];


        cudaMemcpyAsync(buffers[images_index],data,batch_size*input_h*input_w* sizeof(float),cudaMemcpyHostToDevice,stream);
        context->enqueueV2(buffers,stream, nullptr);

        cudaMemcpyAsync(prob,buffers[output_index],batch_size*num*sizeof(float ),cudaMemcpyDeviceToHost,stream);

        cudaStreamSynchronize(stream);
        cudaStreamDestroy(stream);

        cudaFree(buffers[images_index]);
        cudaFree(buffers[output_index]);


 int box_count = 25200;
int		INPUT_W=640;
	int	INPUT_H=640;
	int total_num=85;
 std::vector<Output> objects;
	for(int i = 0 ; i<box_count;i++){
            if(prob[total_num*i+4]<=classThreshold) continue;

            int l ,r,t,b;
            float r_w = INPUT_W/(SrcImg.cols*1.0);
            float r_h = INPUT_H/(SrcImg.rows*1.0);

            float x = prob[total_num*i+0];
            float y = prob[total_num*i+1];
            float w = prob[total_num*i+2];
            float h = prob[total_num*i+3];
            float score =prob[total_num*i+4];


            if(r_h>r_w){
                l = x-w/2.0;
                r = x+w/2.0;
                t = y-h/2.0-(INPUT_H-r_w*SrcImg.rows)/2;
                b = y+h/2.0-(INPUT_H-r_w*SrcImg.rows)/2;
                l=l/r_w;
                r=r/r_w;
                t=t/r_w;
                b=b/r_w;
            }else{

                l = x-w/2.0-(INPUT_W-r_h*SrcImg.cols)/2;
                r = x+w/2.0-(INPUT_W-r_h*SrcImg.cols)/2;
                t = y-h/2.0;
                b = y+h/2.0;
                l=l/r_h;
                r=r/r_h;
                t=t/r_h;
                b=b/r_h;
            }

                int label_index = std::max_element(prob+total_num*i+5,prob+total_num*(i+1))-(prob+total_num*i+5);

				Output obj;
				obj.box.x = l;
				obj.box.y = t;
				obj.box.width=r-l;
				obj.box.height=b-t;
				obj.id = label_index;
				obj.confidence = score;

            objects.push_back(obj);


	}


		printf("objects = %d\n",objects.size());
		qsort_descent_inplace(objects);


		std::vector<int> picked;
        nms_sorted_bboxes(objects,picked,nmsThreshold);

		int count = picked.size();
		// output.resize(count);
		std::vector<Output>outputv;
		outputv.resize(count);
        for(int i = 0 ; i <count ; ++i){
            outputv[i]=objects[picked[i]];

        }
		printf("count = %d\n",count);
		printf("output = %d\n",outputv.size());

		cv::Mat testimg=SrcImg.clone();
		draw_objects(testimg,outputv);















		

        // delete context;
        // delete runtime;
        // delete engine;
		delete [] data;
		return prob;
}

float Yolov5::intersection_area(Output & a,Output&b) {
    Rect2f inter = a.box&b.box;
    return inter.area();

}


void Yolov5::qsort_descent_inplace(std::vector<Output>&faceobjects,int left, int right){
    int i = left;
    int j = right;
    float p = faceobjects[(left+right)/2].confidence;
    while (i<=j){
        while (faceobjects[i].confidence>p ){
            i++;
        }
        while (faceobjects[j].confidence<p){
            j--;
        }
        if(i<=j){
            swap(faceobjects[i],faceobjects[j]);
            i++;
            j--;

        }

    }
#pragma omp parallel sections
    {
#pragma omp section
        {
            if (left < j) qsort_descent_inplace(faceobjects, left, j);
        }
#pragma omp section
        {
            if (i < right) qsort_descent_inplace(faceobjects, i, right);
        }

    }
}



void  Yolov5::qsort_descent_inplace(std::vector<Output>&faceobjects){
    if(faceobjects.empty()){
        return ;
    }
    qsort_descent_inplace(faceobjects,0,faceobjects.size()-1);

}


void Yolov5::nms_sorted_bboxes(std::vector<Output>& faceobjects, std::vector<int>& picked, float nms_threshold)
{
    picked.clear();

    const int n = faceobjects.size();

    std::vector<float> areas(n);
    for (int i = 0; i < n; i++)
    {
        areas[i] = faceobjects[i].box.area();
    }

    for (int i = 0; i < n; i++)
    {
         Output& a = faceobjects[i];

        int keep = 1;
        for (int j = 0; j < (int)picked.size(); j++)
        {
          Output& b = faceobjects[picked[j]];

            // intersection over union
            float inter_area = intersection_area(a, b);
            float union_area = areas[i] + areas[picked[j]] - inter_area;
            // float IoU = inter_area / union_area
            if (inter_area / union_area > nms_threshold)
                keep = 0;
        }

        if (keep)
            picked.push_back(i);
    }
}


// type=0 jsls
// type=1 gw24
bool Yolov5::Detect(Mat &SrcImg, std::vector<Output> &output, int type, std::string &detectres,DeepNet * deepnet_ ) 
{
	LOGI("<Yolov5Detect>recognition type="<<type);
 
	double t_start=(double)getTickCount();
	cv::Mat image_blob;
	float r,dw,dh;
	float *p=nullptr;
	if(type==1)
	{
		className=className_jsls;
		// image_blob = preprocess(SrcImg,cv::Size(640,640),r,dw,dh);
		//cv::imwrite("/home/aaeon/AnoServer_old/demo/sly_blob.jpg",image_blob);

//				cv::imwrite("/home/aaeon/AnoServer_old/demo/srcimg_before.jpg",SrcImg);
		image_blob = preprocessImg(SrcImg,640,640);

		cv::imwrite("/home/aaeon/AnoServer_old/demo/srcimg.jpg",SrcImg);
		p = trtProcess(1,640,640,image_blob,deepnet_ ,SrcImg);
	}
	else if (type == 0)
	{
		className = className_gw;
		// image_blob=preprocess(SrcImg,cv::Size(896,896),r,dw,dh);

		image_blob = preprocessImg(SrcImg,896,896);

		p = trtProcess(1,896,896,image_blob,deepnet_,SrcImg );
	}

	std::vector<int> classIds;
	std::vector<float> confidences;
	std::vector<cv::Rect> boxes;
	cv::Mat class_score;

	int total_num = className.size()+5;
	int box_count=0;
	int INPUT_W=0;
	int INPUT_H=0;
	if(type==1){
		box_count = 25200;
		INPUT_W=640;
		INPUT_H=640;
	}else if (type==0){
		box_count = 49980;
		INPUT_W=896;
		INPUT_H=896;
	}

//  std::vector<Output> objects;
// 	for(int i = 0 ; i<box_count;i++){
//             if(p[total_num*i+4]<=classThreshold) continue;

//             int l ,r,t,b;
//             float r_w = INPUT_W/(SrcImg.cols*1.0);
//             float r_h = INPUT_H/(SrcImg.rows*1.0);

//             float x = p[total_num*i+0];
//             float y = p[total_num*i+1];
//             float w = p[total_num*i+2];
//             float h = p[total_num*i+3];
//             float score =p[total_num*i+4];


//             if(r_h>r_w){
//                 l = x-w/2.0;
//                 r = x+w/2.0;
//                 t = y-h/2.0-(INPUT_H-r_w*SrcImg.rows)/2;
//                 b = y+h/2.0-(INPUT_H-r_w*SrcImg.rows)/2;
//                 l=l/r_w;
//                 r=r/r_w;
//                 t=t/r_w;
//                 b=b/r_w;
//             }else{

//                 l = x-w/2.0-(INPUT_W-r_h*SrcImg.cols)/2;
//                 r = x+w/2.0-(INPUT_W-r_h*SrcImg.cols)/2;
//                 t = y-h/2.0;
//                 b = y+h/2.0;
//                 l=l/r_h;
//                 r=r/r_h;
//                 t=t/r_h;
//                 b=b/r_h;
//             }

//                 int label_index = std::max_element(p+total_num*i+5,p+total_num*(i+1))-(p+total_num*i+5);

// 				Output obj;
// 				obj.box.x = l;
// 				obj.box.y = t;
// 				obj.box.width=r-l;
// 				obj.box.height=b-t;
// 				obj.id = label_index;
// 				obj.confidence = score;

//             objects.push_back(obj);


// 	}


// 		printf("objects = %d\n",objects.size());
// 		qsort_descent_inplace(objects);


// 		std::vector<int> picked;
//         nms_sorted_bboxes(objects,picked,nmsThreshold);

// 		int count = picked.size();
// 		// output.resize(count);

//         for(int i = 0 ; i <count ; ++i){
//             output[i]=objects[picked[i]];

//         }
// 		printf("count = %d\n",count);
// 		printf("output = %d\n",output.size());


	// for(int i=0;i<box_count;i++)
	// {
	// 	if(*(p+total_num*i+4)>classThreshold)
	// 	{
	// 		class_score=cv::Mat(1,total_num-5,CV_32FC1,p+5+total_num*i);
	// 		float *p1=(float*)class_score.data;
	// 		double maxVal;
	// 		cv::Point maxLoc;
	// 		cv::minMaxLoc(class_score,0,&maxVal,0,&maxLoc);
	// 		if(maxVal*(*(p+total_num*i+4))>=classThreshold)
	// 		{
	// 			cv::Rect box_rect;
	// 			box_rect.x=(*(p+total_num*i)-*(p+total_num*i+2)/2.0-dw)/r;
	// 			box_rect.y=(*(p+total_num*i+1)-*(p+total_num*i+3)/2.0-dh)/r;
	// 			box_rect.width=(*(p+total_num*i+2))/r;
	// 			box_rect.height=(*(p+total_num*i+3))/r;
	// 			boxes.push_back(box_rect);
	// 			classIds.push_back(maxLoc.x);
	// 			confidences.push_back(maxVal*(*(p+total_num*i+4)));
	// 		}
	// 	}
	// }
	// std::vector<int> nms_result;
	// NMSBoxes(boxes, confidences, classThreshold, nmsThreshold, nms_result);
	// for (int i = 0; i < nms_result.size(); i++) {
	// 	int idx = nms_result[i];
	// 	Output result;
	// 	result.id = classIds[idx];
	// 	result.confidence = confidences[idx];
	// 	result.box = boxes[idx];
	// 	output.push_back(result);
	// }

	double t_interval = ((double)getTickCount() - t_start) / getTickFrequency();
	LOGI("<Yolov5Detect>total detect time="<<t_interval);
	if (output.size()) {
		for (int i = 0; i<output.size(); i++) {
			detectres.append(className[output[i].id]);
			detectres.append(",");
		}
		detectres = detectres.substr(0, detectres.length() - 1);
		return true;
	}
	else
		return false;
}


void Yolov5::drawPred(Mat &img, cv::Mat &dst,std::vector<Output> result, std::vector<Scalar> color) {
	dst = img.clone();
	for (int i = 0; i < result.size(); i++) 
	{
		int left, top;
		left = result[i].box.x;
		top = result[i].box.y;
		int color_num = i;
 		rectangle(dst, result[i].box, color[result[i].id], 1, 8);
   
		std::string label = className[result[i].id] + ":" + std::to_string(result[i].confidence);

		int baseLine;
		Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
		top = max(top, labelSize.height);

		putText(dst, label, Point(left, top), FONT_HERSHEY_SIMPLEX, 1, color[result[i].id], 1);
   
	}
}

int Yolov5::Recognize(cv::Mat &src,std::string Ano_type,std::string& res, cv::Mat& dst,void *engine)
{
	DeepNet * deepnet_ = (DeepNet *)engine;
	int taskType;
	if(Ano_type=="ForeignBody")
	{
		taskType=0;
	}
	else if(Ano_type=="GroundWater")
	{
		taskType=1;
	}
	std::vector<Scalar> color;
	srand(time(0));
 
	for (int i = 0; i < 80; i++) {
		int b = rand() % 256;
		int g = rand() % 256;
		int r = rand() % 256;
		color.push_back(Scalar(b, g, r));
	}
 
	std::vector<Output> result;
	if(Detect(src,result,taskType,res,deepnet_))
	{
		drawPred(src,dst,result,color);
	}
	else
	{
		LOGE("<Yolov5>Detect finished,is empty");
		res="empty";
	}
	return 0;
}

REGISTER_RECOGNITION("FBGW",Yolov5);
