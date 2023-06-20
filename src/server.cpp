#include "server.h"
#include "Logr.h"
#include "RegistryFactory.h"

struct timeval timeout = { 10,0 };
struct timeval notimeout = { 0,0 };

void AnoServer::close_sock(int server_socket)
{
	close(server_socket);
}

int AnoServer::InitServer()
{
    int server_socket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int opt=1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(DEFAULT_PORT);

		// server_addr.sin_port = htons(30988);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr))<0) {
		LOGE("<InitServer> server bind error!");
		close_sock(server_socket);
		return -1;
	}
    LOGI("<InitServer> server bind complete!");

    char hostname[128];
	int ret = gethostname(hostname, sizeof(hostname));
	struct hostent *hent;
	hent = gethostbyname(hostname);

    if (listen(server_socket, LENGTH_OF_LISTEN_QUEUE) == -1) 
	{
		LOGE("<InitServer> Server Listen Failed!");
		close_sock(server_socket);
		return -1;
	}
	return server_socket;
}

int AnoServer::Connect2Client(int server_socket)
{
	LOGI("==============Connect2Client==============");
	struct sockaddr_in remote_addr;
	socklen_t remote_addr_size = sizeof(remote_addr);
	int new_server_socket = accept(server_socket, (struct sockaddr*)&remote_addr, &remote_addr_size);
	if (new_server_socket < 0)
	{
		LOGE("<Connect2Client>Server Accept Failed!");
		return -1;
	}
	return new_server_socket;
}

int AnoServer::RecvFileInfo(int new_server_socket, FileInfo* &recvInfo)
{
	int total = sizeof(FileInfo);
	setsockopt(new_server_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
	int sum = recv(new_server_socket, (char *)recvInfo, total, 0);
	if (sum < 0)
	{
		LOGE("<RecvFileInfo> Server Recieve Data Failed!");
		return 0;
	}
	while (sum != total)
	{
		char *temp_data = new char[total - sum];
		int length = recv(new_server_socket, temp_data, total - sum, 0);
		if (length>0)
		{
			memcpy(recvInfo + sum, temp_data, length);
			sum += length;
		}
		delete temp_data;
		if (length <= 0)
		{
			LOGE("<AnoServer> Server Recieve Data Failed!");
			return 0;
		}
	}
	LOGI("<RecvInfo>recvInfo cols="<<recvInfo->img_cols);
	LOGI("<RecvInfo>recvInfo rows="<<recvInfo->img_rows);
	LOGI("<RecvInfo>recvInfo channels="<<recvInfo->img_channels);
	LOGI("<RecvInfo>recvInfo status="<<recvInfo->status);
	LOGI("<RecvInfo>recvInfo file_path="<<recvInfo->file_path);
	LOGI("<RecvInfo>recvInfo ano_type="<<recvInfo->ano_type);
	setsockopt(new_server_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&notimeout, sizeof(struct timeval));
	return 1;
}

int AnoServer::RecvImg(int new_server_socket, FileInfo *recvInfo, cv::Mat &img)
{
	int img_height = recvInfo->img_rows;
	int img_width = recvInfo->img_cols;
	int channels = recvInfo->img_channels;
	int total = img_height*img_width * channels;
	char *data = new char[total];
	// setsockopt(new_server_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
	int sum = recv(new_server_socket, data, total, 0);
	if (sum<=0)
	{
		LOGE("<RecvImg> Server Recieve IMG Failed!");
		return 0;
	}
	while (sum != total)
	{
		char *temp_data = new char[total-sum];
		int length = recv(new_server_socket, temp_data, total-sum,0);
		if (length>0)
		{			
			memcpy(data + sum, temp_data, length);
			sum += length;
		}
		delete temp_data;
		if (length <= 0)
		{
			LOGE("<AnoServer> Server Recieve IMG Failed!");
			return 0;
		}
	}
	// setsockopt(new_server_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&notimeout, sizeof(struct timeval));

	img.create(cv::Size(img_width, img_height), recvInfo->img_type);
	memcpy(img.data, data, img_height*img_width * channels);
	memset(data, 0, sizeof(char) * img_height*img_width * channels);
	delete data;
	return 1;
}

int AnoServer::CreateInfo(std::string anoType, std::string statusStr, std::string res, FileInfo &recvInfo,FileInfo &sendInfo,cv::Mat dst)
{
	sendInfo.img_cols = dst.cols;
	sendInfo.img_rows = dst.rows;
	sendInfo.img_channels = recvInfo.img_channels;
	sendInfo.img_type = recvInfo.img_type;
	strncpy(sendInfo.ano_type, anoType.c_str(), strlen(anoType.c_str()));
	sendInfo.ano_type[strlen(anoType.c_str())] = '\0';
	std::string save_path = std::string(recvInfo.file_path).replace(strlen(recvInfo.file_path) - 4, strlen(recvInfo.file_path), "_result.jpg");
	strcpy(sendInfo.file_path, save_path.c_str());
	sendInfo.file_path[save_path.size()] = '\0';
	strncpy(sendInfo.status, statusStr.c_str(), strlen(statusStr.c_str()));
	sendInfo.status[strlen(statusStr.c_str())] = '\0';
	strncpy(sendInfo.ano_res, res.c_str(), strlen(res.c_str()));
	sendInfo.ano_res[strlen(res.c_str())] = '\0';
	return 1;
}

int AnoServer::SendFileInfo(int new_server_socket, FileInfo &sendInfo)
{
	int send_length = 0;
	int total = sizeof(FileInfo);
	send_length = send(new_server_socket, (char*)&sendInfo, total, MSG_NOSIGNAL);
	if (send_length != total)
	{
		LOGE("Send RES Inform Failed:");
		return 0;
	}
	return 1;
}

int AnoServer::SendImg(int new_server_socket, FileInfo &sendInfo,cv::Mat &img)
{
	int total = img.rows*img.cols*img.channels();
	char *data = new char[total];
	memcpy(data, img.data, sizeof(unsigned char)*total);

	int send_size = 0;
	send_size = send(new_server_socket, data, sizeof(unsigned char)*total, MSG_NOSIGNAL);
	if (send_size != total)
	{
		LOGE("Send RES IMG Failed:");
		delete data;
		return 0;
	}
	delete data;
	return 1;
}

void AnoServer::Analyse(void *engine)
{
	int server_socket = InitServer();
	if(server_socket<0)
	{
		return;
	}
	while(true)
	{

		LOGI("<AnoServer> Server Start Listening ...");
		int new_server_socket = Connect2Client(server_socket);
		if(new_server_socket<0)
		{
			close_sock(new_server_socket);
			continue;
		}
		LOGI("<AnoServer> Server accept complete!");

		FileInfo *recvInfo=new(FileInfo);
		if(!(RecvFileInfo(new_server_socket,recvInfo)))
		{
			delete recvInfo;
			close_sock(new_server_socket);
			continue;
		}

		cv::Mat image;
		if (std::string(recvInfo->status).compare("local") != 0)
		{
			if (RecvImg(new_server_socket, recvInfo, image) == 0)
			{
				delete recvInfo;
				close_sock(new_server_socket);
				continue;
			}
		}
		else
		{
			image = cv::imread(recvInfo->file_path);
		}
		LOGI("<AnoServer> recv image successful!");
		//detect
		cv::Mat dst;
		std::string Ano_type;
		if(strcmp(recvInfo->ano_type,"ForeignBody")==0||strcmp(recvInfo->ano_type,"GroundWater")==0)
		{
			Ano_type="FBGW";
			// Ano_type=recvInfo->ano_type;
		}
		else
		{
			Ano_type=recvInfo->ano_type;
		}
		LOGI("<Recognition> start Recognition,Ano_type="<<Ano_type);
		Recognition* ptr=RecognitionRegistry::getRecognize(Ano_type);
		std::string res;
		int ret=ptr->Recognize(image,recvInfo->ano_type,res,dst,engine);
		LOGI("<Recognition> Recognition finished,res="<<res);

		FileInfo sendInfo;
		if (!CreateInfo(recvInfo->ano_type, "sendRes", res, *recvInfo, sendInfo,dst))
		{
			LOGE("Create sendInfo Failed!");
			delete recvInfo;
			close_sock(new_server_socket);
			continue;
		}

		if (!SendFileInfo(new_server_socket, sendInfo))
		{
			delete recvInfo;
			close_sock(new_server_socket);
			continue;
		}

		if (std::string(recvInfo->status).compare("local") != 0)
		{
			if (std::string(sendInfo.ano_res).compare("empty") != 0)
			{
				setsockopt(new_server_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
				if (!SendImg(new_server_socket, sendInfo, dst))
				{
					delete recvInfo;
					close_sock(new_server_socket);
					continue;
				}
			}
		}
		else
		{
			cv::imwrite(sendInfo.file_path, dst);
		}
		LOGI("Send Res Img Success!");
		delete recvInfo;
		close_sock(new_server_socket);
		if (std::string(recvInfo->status).compare("local") == 0)
		{
			break;
		}
	}
	close_sock(server_socket);
}
