#include "include/client.h"

int main(int argc , char * argv[]){

    AnoClient client;
    int fd = client.initClient();

    FileInfo fileinfo;
    std::string filePath = "demo/test.jpg";
   // std::string anoType = "ForeignBody";//896
    std::string anoType = "GroundWater";//640

    std::string status = "ok";
    std::string res = "none";

    client.preprocessFileInfo(filePath,anoType,status,res,fileinfo);
    client.sendFileInfo(fd,fileinfo);
    client.sendImg(fd,fileinfo);

    FileInfo fileinfo_out;
    client.recvFileInfo(fd,fileinfo_out);

    cv::Mat img_res ;
    client.recvImg(fd, &fileinfo_out,img_res);
    cv::imwrite("demo/sly_result.jpg",img_res);
    printf("img already saved in demo\n");

}


