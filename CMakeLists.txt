cmake_minimum_required(VERSION 3.0)
project(AnomalyServer)
set(CMAKE_BUILD_TYPE Debug)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

find_package (OpenCV REQUIRED )
include_directories(include ${OpenCV_INCLUDE_DIR} )

set(SERVER_LISTS
    src/DeepNet.cpp
    src/logger.cpp
    src/Logr.cpp
    src/Recognize.cpp
    src/server.cpp
    src/yolov5.cpp
    )

set(CLIENT_LISTS
    src/client.cpp
    )

#include_directories(/usr/local/cuda-11.6/include)#cuda
#link_directories(/usr/local/cuda-11.6/lib64)

#include_directories(/root/TensorRT-8.4.0.6/include)#trt
#link_directories(/root/TensorRT-8.4.0.6/lib)

add_executable(AnoServer.bin ano_detect_main.cpp ${SERVER_LISTS})
include_directories(/usr/local/cuda-10.2/targets/aarch64-linux/include)
link_directories(/usr/local/cuda-10.2/targets/aarch64-linux/lib)
target_link_libraries(AnoServer.bin ${OpenCV_LIBRARIES} pthread cudart nvinfer )


add_executable(AnoClient.bin run_client.cpp ${CLIENT_LISTS})
target_link_libraries(AnoClient.bin ${OpenCV_LIBRARIES})
