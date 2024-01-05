#include <iostream>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

#include "HNSDStorageManager.h"

namespace pjs = Poco::JSON;

HNSDStorageFile::HNSDStorageFile()
{
    m_type     = HNSDSM_FT_NOTSET;
    m_indexNum = 0;
}

HNSDStorageFile::~HNSDStorageFile()
{

}

void
HNSDStorageFile::setID( std::string id )
{
    m_id = id;
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

//void
//HNSDStorageFile::getInfo( std::string &fileName, std::string &purpose, std::string &tsStr )
//{
//    fileName = getFilename();
//    purpose = m_purpose;
//    tsStr = m_timestampStr;
//}

std::string 
HNSDStorageFile::getID()
{
    return m_id;
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
HNSDStorageFile::getPurpose()
{
    return m_purpose;
}

std::string
HNSDStorageFile::getTimestampStr()
{
    return m_timestampStr;
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
HNSDStorageManager::getFileLocalPath( std::string fileID, std::string &rstPath )
{
    rstPath.clear();

    return HNSDSM_RESULT_SUCCESS;
}

HNSDSM_RESULT_T
HNSDStorageManager::getFileIDListForOwner( std::string ownerID, std::vector< std::string > &fileIDList )
{

    return HNSDSM_RESULT_SUCCESS;
}

std::string
HNSDStorageManager::getFileListJSON()
{
    std::ostringstream ostr;

    // Create a json root array
    pjs::Array jsRoot;

    // Create list of capture objects
    for( std::map< std::string, HNSDStorageFile* >::iterator rit = m_fileMap.begin(); rit != m_fileMap.end(); rit++ )
    { 
        // Create a json File object
        pjs::Object jsFileObj;

        HNSDStorageFile *filePtr = rit->second;

        // Fill in object fields
        jsFileObj.set( "id", filePtr->getID() );
        jsFileObj.set( "purpose", filePtr->getPurpose() );
        jsFileObj.set( "filename", filePtr->getFilename() );
        jsFileObj.set( "timestamp", filePtr->getTimestampStr() );

        // Add new capture object to return list
        jsRoot.add( jsFileObj );
    }

    try 
    { 
        pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
    }
    catch( ... )
    {
        return ""; 
    }

    return ostr.str();
}

std::string
HNSDStorageManager::getFileJSON( std::string fileID )
{
    std::ostringstream ostr;

    // Create a json root object
    pjs::Object jsRoot;

    std::map< std::string, HNSDStorageFile* >::iterator rit = m_fileMap.find( fileID );

    if( rit != m_fileMap.end() )
    { 
        HNSDStorageFile *filePtr = rit->second;

        // Fill in object fields
        jsRoot.set( "id", filePtr->getID() );
        jsRoot.set( "purpose", filePtr->getPurpose() );
        jsRoot.set( "filename", filePtr->getFilename() );
        jsRoot.set( "timestamp", filePtr->getTimestampStr() );
    }

    try 
    { 
        pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
    }
    catch( ... )
    {
        return ""; 
    }

    return ostr.str();
}
