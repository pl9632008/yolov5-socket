#include "client.h"

int AnoClient::initClient(){

    int client_socket =socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(client_socket == -1){
        printf("client error!\n");
        return -1;
    }

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr("192.168.10.13");
    client_addr.sin_port = htons(DEFAULT_PORT);

	printf("wait for starting AnoServer...\n");

	while(connect(client_socket, (sockaddr*)&client_addr, sizeof(client_addr)) == -1);

	printf("connect AnoServer successfully\n");
    return client_socket;
}

void AnoClient::preprocessFileInfo(std::string &filePath , std::string &anoType , std::string &status , std::string &res , FileInfo &fileinfo ){

    cv::Mat img = cv::imread(filePath);
   
    fileinfo.img_cols = img.cols;
    fileinfo.img_rows = img.rows;
    fileinfo.img_channels = img.channels();

	strncpy(fileinfo.file_path, filePath.c_str(), strlen(filePath.c_str()));
    fileinfo.file_path[strlen(filePath.c_str())] = '\0';

	// std::string save_path = std::string(fileinfo.file_path).replace(strlen(fileinfo.file_path) - 4, strlen(fileinfo.file_path), "_result.jpg");
	// strcpy(fileinfo.file_path, save_path.c_str());
	// fileinfo.file_path[save_path.size()] = '\0';

	strncpy(fileinfo.ano_type, anoType.c_str(), strlen(anoType.c_str()));
	fileinfo.ano_type[strlen(anoType.c_str())] = '\0';

	strncpy(fileinfo.status, status.c_str(), strlen(status.c_str()));
	fileinfo.status[strlen(status.c_str())] = '\0';

	strncpy(fileinfo.ano_res, res.c_str(), strlen(res.c_str()));
	fileinfo.ano_res[strlen(res.c_str())] = '\0';
    return ;

}

int AnoClient::sendFileInfo(int new_client_socket, FileInfo& sendInfo){

    int ret = send(new_client_socket, (char *)&sendInfo, sizeof(FileInfo) ,MSG_NOSIGNAL);

    return ret;
}



int AnoClient::sendImg(int new_client_socket, FileInfo &sendInfo){

    cv::Mat img = cv::imread(sendInfo.file_path);

	int total = img.rows*img.cols*img.channels();
	char *data = new char[total];
	memcpy(data, img.data, sizeof(unsigned char)*total);

	int send_size = 0;
	send_size = send(new_client_socket, data, sizeof(unsigned char)*total, MSG_NOSIGNAL);
	if (send_size != total){
		printf("client send img failed!\n");
		delete data;
		return 0;
	}
	delete data;
	return 1;
}


int AnoClient::recvFileInfo(int new_client_socket, FileInfo &recvInfo){

	int total = sizeof(FileInfo);

	int sum = recv(new_client_socket, (char *)&recvInfo, total, 0);

	if (sum < 0){
		printf("<RecvFileInfo> Client Recieve Data Failed!\n");
		return 0;
	}
	while (sum != total){
		char *temp_data = new char[total - sum];
		int length = recv(new_client_socket, temp_data, total - sum, 0);
		if (length>0){
			memcpy(&recvInfo + sum, temp_data, length);
			sum += length;
		}
		delete temp_data;
		if (length <= 0){
			printf("<AnoClient> Client Recieve Data Failed!\n");
			return 0;
		}
	}
	printf("<RecvInfo>recvInfo cols= %d\n",recvInfo.img_cols);
	printf("<RecvInfo>recvInfo rows= %d\n",recvInfo.img_rows);
	printf("<RecvInfo>recvInfo channels= %d\n",recvInfo.img_channels);
	printf("<RecvInfo>recvInfo status= %s\n",recvInfo.status);
	printf("<RecvInfo>recvInfo file_path= %s\n",recvInfo.file_path);
	printf("<RecvInfo>recvInfo ano_type= %s\n",recvInfo.ano_type);
	return 1;
}


int AnoClient::recvImg(int new_client_socket, FileInfo *recvInfo, cv::Mat &img){

	int img_height = recvInfo->img_rows;
	int img_width = recvInfo->img_cols;
	int channels = recvInfo->img_channels;
	int total = img_height*img_width * channels;
	char *data = new char[total];

	int sum = recv(new_client_socket, data, total, 0);

 	if (sum<=0){
		printf("<RecvImg> Client Recieve IMG Failed!\n");
		return 0;
	}
	while (sum != total){
		char *temp_data = new char[total-sum];
		int length = recv(new_client_socket, temp_data, total-sum,0);
		if (length>0){			
			memcpy(data + sum, temp_data, length);
			sum += length;
		}
		delete temp_data;
		if (length <= 0){
			printf("<AnoClient> Client Recieve IMG Failed!\n");
			return 0;
		}
	}

	img.create(cv::Size(img_width, img_height), recvInfo->img_type);
	memcpy(img.data, data, img_height*img_width * channels);
	memset(data, 0, sizeof(char) * img_height*img_width * channels);
	delete data;
	return 1;
}
