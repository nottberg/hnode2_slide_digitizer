#include <sys/mman.h>

#include <iostream>
#include <sstream>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

#include "HNSDImageManager.h"

namespace pjs = Poco::JSON;

HNSDImageManager::HNSDImageManager()
{

}

HNSDImageManager::~HNSDImageManager()
{

}

IMM_RESULT_T
HNSDImageManager::start()
{
    return IMM_RESULT_SUCCESS;
}

IMM_RESULT_T
HNSDImageManager::stop()
{

    return IMM_RESULT_SUCCESS;
}


