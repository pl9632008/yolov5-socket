#include "Recognize.h"
#include "Logr.h"

int load_model(void *&engine)
{
    DeepNet* RM=new DeepNet();
    int ret=RM->load_model();
    if(!ret)
    {
        delete RM;
        return false;
    }
    engine=RM;
    return true;
}
