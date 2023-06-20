#pragma once
#ifndef _ANO_SERVER_H
#define _ANO_SERVER_H

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

#define BUFFER_SIZE 1024
#define LENGTH_OF_LISTEN_QUEUE 20
#define FILE_NAME_MAX_SIZE 512
#define DEFAULT_PORT 17788


struct FileInfo
{
	char ano_type[50];
	char status[50];
	char file_path[FILE_NAME_MAX_SIZE];
	char ano_res[50];
	int img_cols;
	int img_rows;
	int img_channels;
	int img_type;
};


class AnoServer 
{
public:
	int InitServer();
	void close_sock(int server_socket);
	int Connect2Client(int server_socket);
	int RecvFileInfo(int new_server_socket, FileInfo* &recvInfo);
	int RecvImg(int new_server_socket, FileInfo *recvInfo, cv::Mat &img);
	int CreateInfo(std::string anoType, std::string statusStr, std::string res, FileInfo &recvInfo, FileInfo &sendInfo,cv::Mat dst);
	int SendFileInfo(int new_server_socket, FileInfo &sendInfo);
	int SendImg(int new_server_socket, FileInfo &sendInfo, cv::Mat &img);
	void Analyse(void *engine);



};

#endif
