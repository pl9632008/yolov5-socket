#include "yolov5.h"

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



float * Yolov5::trtProcess(int batch_size, int input_h, int input_w , Mat &img, DeepNet * deepnet_ ,Mat & SrcImg){

		const char * images;
        const char * output;
		int channels = 3;
		IRuntime * runtime;
		ICudaEngine * engine;
		IExecutionContext *context;
		int num=0;
		// float *data,*prob=nullptr;
		if(input_h==896){

				runtime = deepnet_->runtime896_ ;
        		engine  = deepnet_->engine896_;
        		context = deepnet_->context896_ ;
				num = 49980*29;

 				images = "images";
        		output = "output";
        		const char * output892 = "892";
       			const char * output951 = "951";
       			const char * output1010 = "1010";
      			const char * output1069 = "1069";

				int32_t images_index = engine->getBindingIndex(images);
				int32_t output_index = engine->getBindingIndex(output);
			
				int32_t output892_index = engine->getBindingIndex(output892);
				int32_t output951_index = engine->getBindingIndex(output951);
				int32_t output1010_index = engine->getBindingIndex(output1010);
				int32_t output1069_index = engine->getBindingIndex(output1069);


				void * buffers[5];
				cudaMalloc(&buffers[images_index],batch_size*channels*input_h*input_w*sizeof (float));

				cudaMalloc(&buffers[output892_index],batch_size*channels*112*112*29*sizeof (float));
				cudaMalloc(&buffers[output951_index],batch_size*channels*56*56*29*sizeof (float));
				cudaMalloc(&buffers[output1010_index],batch_size*channels*28*28*29*sizeof (float));
				cudaMalloc(&buffers[output1069_index],batch_size*channels*14*14*29*sizeof (float));
			
				cudaMalloc(&buffers[output_index],batch_size*num*sizeof (float));

				float data[channels*input_h*input_w];
				for(int i = 0 ; i < input_h*input_w;i++){
					data[i] = img.at<Vec3f>(i)[0];
					data[i+input_h*input_w] = img.at<Vec3f>(i)[1];
					data[i+2*input_h*input_w]=img.at<Vec3f>(i)[2];
				}
				cudaStream_t stream;
				cudaStreamCreate(&stream);
				cudaMemcpyAsync(buffers[images_index],data,batch_size*channels*input_h*input_w*sizeof(float),cudaMemcpyHostToDevice,stream);
				
				context->enqueueV2(buffers,stream, nullptr);

				float* prob= new float[num];
				cudaMemcpyAsync(prob,buffers[output_index],batch_size*num* sizeof(float),cudaMemcpyDeviceToHost,stream);
			
				cudaStreamSynchronize(stream);
				cudaStreamDestroy(stream);

				cudaFree(buffers[images_index]);
				cudaFree(buffers[output_index]);
				cudaFree(buffers[output892_index]);
				cudaFree(buffers[output951_index]);
				cudaFree(buffers[output1010_index]);
				cudaFree(buffers[output1069_index]);

				// delete runtime;
				// delete engine;
				// context=nullptr;

				return prob;
		}else if(input_h==640){
				
				runtime = deepnet_->runtime640_ ;
        		engine  = deepnet_->engine640_;
        		context = deepnet_->context640_ ;
				num = 25200*85;

 				images = "images";
        		output = "output0";

				int32_t images_index = engine->getBindingIndex(images);
				int32_t output_index = engine->getBindingIndex(output);
				
				void * buffers[2];
				cudaMalloc(&buffers[images_index],batch_size*channels*input_h*input_w*sizeof (float));

				cudaMalloc(&buffers[output_index],batch_size*num*sizeof (float));

				float data[channels*input_h*input_w];
				for(int i = 0 ; i < input_h*input_w;i++){
					data[i] = img.at<Vec3f>(i)[0];
					data[i+input_h*input_w] = img.at<Vec3f>(i)[1];
					data[i+2*input_h*input_w]=img.at<Vec3f>(i)[2];
				}
				cudaStream_t stream;
				cudaStreamCreate(&stream);
				cudaMemcpyAsync(buffers[images_index],data,batch_size*channels*input_h*input_w*sizeof(float),cudaMemcpyHostToDevice,stream);
				
				context->enqueueV2(buffers,stream, nullptr);

				float* prob= new float[num];
				cudaMemcpyAsync(prob,buffers[output_index],batch_size*num* sizeof(float),cudaMemcpyDeviceToHost,stream);
			
				cudaStreamSynchronize(stream);
				cudaStreamDestroy(stream);

				cudaFree(buffers[images_index]);
				cudaFree(buffers[output_index]);

				// delete runtime;
				// delete engine;
				// context=nullptr;

				return prob;
		}
		
		return nullptr;
     
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
		image_blob = preprocess(SrcImg,cv::Size(640,640),r,dw,dh);
		p = trtProcess(1,640,640,image_blob,deepnet_ ,SrcImg);
	}
	else if (type == 0)
	{
		className = className_gw;
		image_blob=preprocess(SrcImg,cv::Size(896,896),r,dw,dh);
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

	for(int i=0;i<box_count;i++)
	{
		if(*(p+total_num*i+4)>classThreshold)
		{
			class_score=cv::Mat(1,total_num-5,CV_32FC1,p+5+total_num*i);
			float *p1=(float*)class_score.data;
			double maxVal;
			cv::Point maxLoc;
			cv::minMaxLoc(class_score,0,&maxVal,0,&maxLoc);
			if(maxVal*(*(p+total_num*i+4))>=classThreshold)
			{
				cv::Rect box_rect;
				box_rect.x=(*(p+total_num*i)-*(p+total_num*i+2)/2.0-dw)/r;
				box_rect.y=(*(p+total_num*i+1)-*(p+total_num*i+3)/2.0-dh)/r;
				box_rect.width=(*(p+total_num*i+2))/r;
				box_rect.height=(*(p+total_num*i+3))/r;
				boxes.push_back(box_rect);
				classIds.push_back(maxLoc.x);
				confidences.push_back(maxVal*(*(p+total_num*i+4)));
			}
		}
	}
	delete p;
	std::vector<int> nms_result;
	NMSBoxes(boxes, confidences, classThreshold, nmsThreshold, nms_result);
	for (int i = 0; i < nms_result.size(); i++) {
		int idx = nms_result[i];
		Output result;
		result.id = classIds[idx];
		result.confidence = confidences[idx];
		result.box = boxes[idx];
		output.push_back(result);
	}

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
