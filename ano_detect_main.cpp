#include "Logr.h"
#include "server.h"
#include "DeepNet.h"
#include "Recognize.h"
#include "yolov5.h"

int main()
{
    LogrInit(LOGR_LEVEL_TRACE,"./log/");
    LOGI("----------load model!-----------");
    void *engine;
    if(load_model(engine))
    {
        LOGI("<load model>ALL modl load successful!");
    }
    else
        return 0;

    LOGI("----------AnoServer start!-----------");
    AnoServer server;

    server.Analyse(engine);

    return 0;
}
