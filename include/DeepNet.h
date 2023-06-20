#ifndef _DEEPNET_H
#define _DEEPNET_H
#include <opencv2/opencv.hpp>
#include "logger.h"

class DeepNet
{
    public:
        bool load_model();
        void loadEngine(std::string str);
        
        // static char * trt_model_stream896_;
        // static char * trt_model_stream640_;

        Logger logger;

        IRuntime * runtime896_ ;
        ICudaEngine * engine896_;
        IExecutionContext *context896_ ;

        IRuntime * runtime640_ ;
        ICudaEngine * engine640_;
        IExecutionContext *context640_ ;

};
// char * DeepNet::trt_model_stream896_= new char[1109182076];
// char * DeepNet::trt_model_stream640_= new char[114890330];
#endif
