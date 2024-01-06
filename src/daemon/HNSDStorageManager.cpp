#include <sys/time.h>

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
HNSDStorageFile::setOwnerID( std::string id )
{
    m_ownerID = id;
}

void
HNSDStorageFile::setInstanceID( std::string id )
{
    m_instanceID = id;
}

void
HNSDStorageFile::setPurpose( std::string purpose )
{
    m_purpose = purpose;
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
HNSDStorageFile::setTimestamp( uint64_t timestamp )
{
    m_timestamp = timestamp;
}

void
HNSDStorageFile::setTimestampFromSystem()
{
    struct timeval tv;
    uint64_t timestamp = 0;

    // Get a timestamp in microseconds
    gettimeofday( &tv,NULL );

    m_timestamp = 1000000 * tv.tv_sec + tv.tv_usec;
}

std::string 
HNSDStorageFile::getID()
{
    return m_id;
}

std::string
HNSDStorageFile::getOwnerID()
{
    return m_ownerID;
}

std::string
HNSDStorageFile::getInstanceID()
{
    return m_instanceID;
}

std::string
HNSDStorageFile::getPurpose()
{
    return m_purpose;
}

std::string 
HNSDStorageFile::getFilename()
{
    char tmpFN[256];
    std::string rtnStr;

    sprintf( tmpFN, "hnsd_%s_%u_%s.jpg", getTimestampAsUSStr().c_str(), m_indexNum, m_purpose.c_str() );
    
    rtnStr = tmpFN;

    return rtnStr;
}

uint64_t
HNSDStorageFile::getTimestamp()
{
    return m_timestamp;
}

std::string
HNSDStorageFile::getTimestampAsUSStr()
{
    char tmpStr[64];

    sprintf( tmpStr, "%lu", m_timestamp );

    return tmpStr;
}

std::string 
HNSDStorageFile::getLocalFilePath()
{
    std::string fullPath;

    fullPath = m_path + "/" + getFilename();

    return fullPath;
}

HNSDStorageManager::HNSDStorageManager()
{
    m_nextFileIndex = 0;
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
    char tmpStr[64];
    HNSDStorageFile *newFile = new HNSDStorageFile;

    // Create a new id string
    sprintf( tmpStr, "smf%u", m_nextFileIndex );

    newFile->setID( tmpStr );

    newFile->setTimestampFromSystem();

    newFile->setOwnerID( ownerID );
    newFile->setInstanceID( instanceID );
    newFile->setPurpose( purpose );
    newFile->setIndex( m_nextFileIndex );
    
    newFile->setPath( getStorageRootPath() );

    newFile->setType( HNSDSM_FT_JPEG );

    m_nextFileIndex += 1;

    m_fileMap.insert(std::pair< std::string, HNSDStorageFile* >( newFile->getID(), newFile ) );

    *filePtr = newFile;

    return HNSDSM_RESULT_SUCCESS;
}

HNSDSM_RESULT_T
HNSDStorageManager::findFile( std::string fileID, HNSDStorageFile **filePtr )
{
    std::map< std::string, HNSDStorageFile* >::iterator it = m_fileMap.find( fileID );

    if( it == m_fileMap.end() )
        return HNSDSM_RESULT_NOT_FOUND;

    *filePtr = it->second;

    return HNSDSM_RESULT_SUCCESS;
}

HNSDSM_RESULT_T
HNSDStorageManager::getLocalFilePath( std::string fileID, std::string &rstPath )
{
    rstPath.clear();

    return HNSDSM_RESULT_SUCCESS;
}

HNSDSM_RESULT_T
HNSDStorageManager::getFileIDListForOwner( std::string ownerID, std::vector< std::string > &fileIDList )
{
    for( std::map< std::string, HNSDStorageFile* >::iterator it = m_fileMap.begin(); it != m_fileMap.end(); it++ )
    {
        if( it->second->getOwnerID() == ownerID )
            fileIDList.push_back( it->second->getID() );
    }

    return HNSDSM_RESULT_SUCCESS;
}

void
HNSDStorageManager::deleteFile( std::string fileID )
{

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
        jsFileObj.set( "timestamp", filePtr->getTimestampAsUSStr() );

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
        jsRoot.set( "timestamp", filePtr->getTimestampAsUSStr() );
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
