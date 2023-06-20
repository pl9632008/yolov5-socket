#ifndef ANOSERVER_INCLUDE_LOGGER_H_
#define ANOSERVER_INCLUDE_LOGGER_H_
#include "cuda_runtime.h"
#include "NvInfer.h"
#include <iostream>
using namespace nvinfer1;
class Logger: public ILogger{
    void log(Severity severity, const char* msg)noexcept override;
};

#endif