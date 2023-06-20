#pragma once
#ifndef _REGISTRY_FACTORY_H
#define _REGISTRY_FACTORY_H
#include <map>
#include <opencv2/opencv.hpp>
#include "Logr.h"

class Recognition
{
    public:
        virtual int Recognize(cv::Mat &src,std::string Ano_type,std::string& res, cv::Mat& dst,void *engine)=0;
};

class RecognitionRegistry
{
    public:
        typedef std::map<std::string,Recognition*> createRegistry; 
        static createRegistry& Registry() 
		{
			static createRegistry* _registry = new createRegistry();
			return *_registry;
		}

        static void registerRecognition(const std::string& type, Recognition* reg) 
        {
            createRegistry&  registry=Registry();
            if(registry.count(type)!=0)
            {
                LOGW("Recognition type:"<<type<<"already registered");
                return;
            }
            registry[type] = reg;
        };

        static Recognition* getRecognize(const std::string& type)
        {
            createRegistry&  registry=Registry();
            if(registry.count(type)!=1)
            {
                LOGE("Unknown Recognition type:"<<type);
                return nullptr;
            }
            return registry[type];
        }
};


#define REGISTER_RECOGNITION(type, class_name) \
    class class_name##Registration \
    { \
    public: \
        class_name##Registration() \
        { \
            RecognitionRegistry::registerRecognition(type, new class_name()); \
        } \
    }; \
    static class_name##Registration g_##class_name##Registration;
    
#endif
