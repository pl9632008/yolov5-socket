#include "DeepNet.h"
#include "Logr.h"
#include <unistd.h>
#include <fstream>
#include "logger.h"
#define _access access

// char * trt_model_stream896_= new char[1109182076];
// char * trt_model_stream640_= new char[152546830];

void DeepNet::loadEngine(std::string str){
        size_t size{0};
        std::ifstream file(str, std::ios::binary);
        char * trt_model_stream{nullptr};
        if(file.good()){
            file.seekg(0,std::ios::end);
            size = file.tellg();
            file.seekg(0,std::ios::beg);
            trt_model_stream = new char[size];
            file.read(trt_model_stream,size);
            file.close();
        
// The engine plan file is generated on an incompatible device, expecting compute 5.3
         if(str=="./engine/model896.engine"){
                runtime896_ = createInferRuntime(logger);
                engine896_ = runtime896_->deserializeCudaEngine(trt_model_stream,size);
                context896_ = engine896_->createExecutionContext(); 
            }else{
                runtime640_ = createInferRuntime(logger);
                engine640_ = runtime640_->deserializeCudaEngine(trt_model_stream,size);
                context640_ = engine640_->createExecutionContext();
            }
        }
        delete[] trt_model_stream;     
}

bool DeepNet::load_model(){

    std::string model_path="./engine/";

    // std::string ForeignBody_model_path=model_path+"model896.engine";
    // LOGI("<load model>load ForeignBody model");
    // if(_access(ForeignBody_model_path.c_str(), 0) != -1){
    // if(engine896_==nullptr){
    //         loadEngine(ForeignBody_model_path);
    //         if(engine896_==nullptr){
    //             LOGE("load model failed");
    //             return false;
    //         }
    //     }
    // }
    // LOGI("<load model>load ForeignBody model successful!");

    // //GroundWater(use yolov5s.engine)
    LOGI("<load model>load GroundWater model");
    std::string GroundWater_model_path=model_path+"model640.engine";
    if(_access(GroundWater_model_path.c_str(), 0) != -1){
       if(engine640_==nullptr){
            loadEngine(GroundWater_model_path);
            if(engine640_==nullptr){
                LOGE("load model failed");
                return false;
            }
        }
    }
    LOGI("<load model>load GroundWater model successful!");
    return true;
}

