#ifndef ANOSERVER_INCLUDE_CLIENT_H_
#define ANOSERVER_INCLUDE_CLIENT_H_

#include <iostream>
#include <string>
#include <sys/types.h>   
#include <stdio.h>        
#include <stdlib.h>        
#include <string.h>  
#include <opencv2/opencv.hpp>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h> 
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define FILE_NAME_MAX_SIZE 512
#define DEFAULT_PORT 17788

struct FileInfo{
	char ano_type[50];
	char status[50];
	char file_path[FILE_NAME_MAX_SIZE];
	char ano_res[50];
	int img_cols;
	int img_rows;
	int img_channels;
	int img_type=16;
};


class AnoClient{
    public:
        int initClient();
		void preprocessFileInfo(std::string &filePath , std::string &anoType , std::string &status , std::string &res , FileInfo &fileinfo );
	    int sendFileInfo(int new_client_socket, FileInfo& sendInfo);
        int sendImg(int new_client_socket, FileInfo &sendInfo);
        int recvFileInfo(int new_client_socket, FileInfo &recvInfo);
        int recvImg(int new_client_socket, FileInfo *recvInfo, cv::Mat &img);
};

#endif
