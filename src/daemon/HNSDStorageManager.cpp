//#include <sys/time.h>

#include <iostream>

//#include <Poco/JSON/Object.h>
//#include <Poco/JSON/Parser.h>
//#include <Poco/StreamCopier.h>

#include "HNSDStorageManager.h"

//namespace pjs = Poco::JSON;

HNSDStorageFile::HNSDStorageFile()
{
    m_type     = HNSDSM_FT_NOTSET;
    m_indexNum = 0;
}

HNSDStorageFile::~HNSDStorageFile()
{

}

void
HNSDStorageFile::setType( HNSDSM_FT_T type )
{
    m_type = type;
}

void
HNSDStorageFile::setPath( std::string path )
{
    m_path = path;
}

void
HNSDStorageFile::setIndex( uint index )
{
    m_indexNum = index;
}

void
HNSDStorageFile::setPurpose( std::string purpose )
{
    m_purpose = purpose;
}

void
HNSDStorageFile::setTimestampStr( std::string tsStr )
{
    m_timestampStr = tsStr;
}

void
HNSDStorageFile::getInfo( std::string &fileName, std::string &purpose, std::string &tsStr )
{
    fileName = getFilename();
    purpose = m_purpose;
    tsStr = m_timestampStr;
}

std::string 
HNSDStorageFile::getFilename()
{
    char tmpFN[256];
    std::string rtnStr;

    sprintf( tmpFN, "hnsd_%s_%u_%s.jpg", m_timestampStr.c_str(), m_indexNum, m_purpose.c_str() );
    
    rtnStr = tmpFN;

    return rtnStr;
}

std::string 
HNSDStorageFile::getPathAndFile()
{
    std::string fullPath;

    fullPath = m_path + "/" + getFilename();

    return fullPath;
}

HNSDStorageManager::HNSDStorageManager()
{

}

HNSDStorageManager::~HNSDStorageManager()
{

}

HNSDSM_RESULT_T
HNSDStorageManager::start()
{
    HNSDSM_RESULT_T result = HNSDSM_RESULT_SUCCESS;

    //m_pipelineMgr = pipelineMgr;

    return result;
}

HNSDSM_RESULT_T
HNSDStorageManager::stop()
{
    return HNSDSM_RESULT_SUCCESS;
}

std::string 
HNSDStorageManager::getStorageRootPath()
{
    return "/tmp";
}

HNSDSM_RESULT_T
HNSDStorageManager::allocateNewFile( std::string ownerID, std::string instanceID, std::string purpose, HNSDStorageFile **filePtr )
{
    return HNSDSM_RESULT_SUCCESS;
}

HNSDSM_RESULT_T
HNSDStorageManager::findFile( std::string ownerID, std::string instanceID, std::string purpose, HNSDStorageFile **filePtr )
{
    return HNSDSM_RESULT_SUCCESS;
}

HNSDSM_RESULT_T
HNSDStorageManager::getCapturePathAndFile( std::string captureID, uint fileIndex, std::string &rstPath )
{
    rstPath.clear();

    return HNSDSM_RESULT_SUCCESS;
}
