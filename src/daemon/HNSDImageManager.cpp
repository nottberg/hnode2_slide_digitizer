#include <sys/mman.h>
#include <sys/time.h>

#include <iostream>
#include <sstream>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

#include <opencv2/opencv.hpp>

#include "HNSDImageManager.h"

namespace pjs = Poco::JSON;

HNSDCaptureFile::HNSDCaptureFile()
{
    m_type     = HNSDCAP_FT_NOTSET;
    m_indexNum = 0;
}

HNSDCaptureFile::~HNSDCaptureFile()
{

}

void
HNSDCaptureFile::setType( HNSDCAP_FT_T type )
{
    m_type = type;
}

void
HNSDCaptureFile::setPath( std::string path )
{
    m_path = path;
}

void
HNSDCaptureFile::setIndex( uint index )
{
    m_indexNum = index;
}

void
HNSDCaptureFile::setPurpose( std::string purpose )
{
    m_purpose = purpose;
}

void
HNSDCaptureFile::setTimestampStr( std::string tsStr )
{
    m_timestampStr = tsStr;
}

void
HNSDCaptureFile::getInfo( std::string &fileName, std::string &purpose, std::string &tsStr )
{
    fileName = getFilename();
    purpose = m_purpose;
    tsStr = m_timestampStr;
}

std::string 
HNSDCaptureFile::getFilename()
{
    char tmpFN[256];
    std::string rtnStr;

    sprintf( tmpFN, "hnsd_%s_%u_%s.jpg", m_timestampStr.c_str(), m_indexNum, m_purpose.c_str() );
    
    rtnStr = tmpFN;

    return rtnStr;
}

std::string 
HNSDCaptureFile::getPathAndFile()
{
    std::string fullPath;

    fullPath = m_path + "/" + getFilename();

    return fullPath;
}

HNSDCaptureRecord::HNSDCaptureRecord( HNSDIMRootInterface *infoIntf )
{
    m_infoIntf = infoIntf;

    m_orderIndex = 0;

    m_executionState = HNSDCAP_EXEC_STATE_NOTSET;
}

HNSDCaptureRecord::~HNSDCaptureRecord()
{
}

void
HNSDCaptureRecord::setID( std::string id )
{
    m_id = id;
}

void 
HNSDCaptureRecord::setPipeline( HNSDPipeline *pipeline )
{
    m_pipeline = pipeline;
}

void
HNSDCaptureRecord::generateNewID( uint64_t timestamp, uint orderIndex )
{
    char tsStr[64];
    char idStr[64];
    
    sprintf( tsStr, "%lu", timestamp );
    m_timestampStr = tsStr;

    sprintf( idStr, "sdcr_%lu_%u", timestamp, orderIndex );

    setID( idStr );
}

std::string
HNSDCaptureRecord::getID()
{
    return m_id;
}

void
HNSDCaptureRecord::setOrderIndex( uint index )
{
    m_orderIndex = index;
}

uint
HNSDCaptureRecord::getOrderIndex()
{
    return m_orderIndex;
}

HNSDPipeline* 
HNSDCaptureRecord::getPipelinePtr()
{
    return m_pipeline;
}


HNSDCAP_EXEC_STATE_T
HNSDCaptureRecord::getState()
{
    return m_executionState;
}

std::string
HNSDCaptureRecord::getStateAsStr()
{
    switch( m_executionState )
    {
        case HNSDCAP_EXEC_STATE_NOTSET:
            return "NotSet";

        case HNSDCAP_EXEC_STATE_PENDING:
            return "Pending";

        case HNSDCAP_EXEC_STATE_ACTIVE:
            return "Active";

        case HNSDCAP_EXEC_STATE_COMPLETE:
            return "Complete";
    }

    return "Unknown";
}

uint
HNSDCaptureRecord::getFileCount()
{
    return m_fileList.size();
}

IMM_RESULT_T
HNSDCaptureRecord::getFileInfo( uint fileIndex, std::string &fileName, std::string &purpose, std::string &tsStr )
{
    if( fileIndex >= m_fileList.size() )
        return IMM_RESULT_FAILURE;

    m_fileList[fileIndex]->getInfo( fileName, purpose, tsStr );
    return IMM_RESULT_SUCCESS;
}

IMM_RESULT_T
HNSDCaptureRecord::getFilePath( uint fileIndex, std::string &rtnPath )
{
    std::string filename;

    rtnPath.clear();

    if( fileIndex >= m_fileList.size() )
        return IMM_RESULT_FAILURE;

    rtnPath = m_fileList[fileIndex]->getPathAndFile();

    return IMM_RESULT_SUCCESS;
}

bool
HNSDCaptureRecord::isPending()
{
    return (m_executionState == HNSDCAP_EXEC_STATE_PENDING) ? true : false;
}

std::string
HNSDCaptureRecord::registerNextFilename( std::string purpose )
{
    HNSDCaptureFile *newFile = new HNSDCaptureFile;

    newFile->setPath( m_infoIntf->getStorageRootPath() );
    newFile->setTimestampStr( m_timestampStr );
    newFile->setType( HNSDCAP_FT_JPEG );
    newFile->setPurpose( purpose );
    newFile->setIndex( m_nextFileIndex );
    m_nextFileIndex += 1;

    m_fileList.push_back( newFile );

    return newFile->getPathAndFile();
}

std::string
HNSDCaptureRecord::getLastOutputPathAndFile()
{
    int len = m_fileList.size();
    if( len == 0 )
        return "";

    return m_fileList[len-1]->getPathAndFile();
}

void 
HNSDCaptureRecord::makePending()
{
    m_executionState = HNSDCAP_EXEC_STATE_PENDING;
}

void
HNSDCaptureRecord::makeActive()
{
    if( m_executionState == HNSDCAP_EXEC_STATE_PENDING )
        m_executionState = HNSDCAP_EXEC_STATE_ACTIVE;
}

void
HNSDCaptureRecord::makeComplete()
{
    if( m_executionState == HNSDCAP_EXEC_STATE_ACTIVE )
        m_executionState = HNSDCAP_EXEC_STATE_COMPLETE;
}

HNSDImageManager::HNSDImageManager()
{
    m_nextOrderIndex = 0;
}

HNSDImageManager::~HNSDImageManager()
{

}

IMM_RESULT_T
HNSDImageManager::start( HNSDPipelineManagerIntf *pipelineMgr )
{
    IMM_RESULT_T result = IMM_RESULT_SUCCESS;

    m_pipelineMgr = pipelineMgr;

    return result;
}

IMM_RESULT_T
HNSDImageManager::stop()
{

    return IMM_RESULT_SUCCESS;
}

std::string 
HNSDImageManager::getStorageRootPath()
{
    return "/tmp";
}

IMM_RESULT_T
HNSDImageManager::createCaptures( uint count, bool postAdvance )
{
    struct timeval tv;
    uint64_t timestamp = 0;
    HNSDCaptureRecord *newCapRec = new HNSDCaptureRecord( this );

    gettimeofday( &tv,NULL );

    timestamp = 1000000 * tv.tv_sec + tv.tv_usec;

    newCapRec->generateNewID( timestamp, m_nextOrderIndex );
    m_nextOrderIndex += 1;

    HNSDPipeline *pipeline;
    m_pipelineMgr->allocatePipeline( HNSDP_TYPE_IMAGE_CAPTURE, newCapRec, &pipeline );

    newCapRec->setPipeline( pipeline );

    newCapRec->makePending();

    m_captureRecordMap.insert( std::pair< std::string, HNSDCaptureRecord* >( newCapRec->getID(), newCapRec ) );

    return IMM_RESULT_SUCCESS;
}

void
HNSDImageManager::deleteCapture( std::string capID )
{
}

std::string
HNSDImageManager::getCaptureListJSON()
{
    std::ostringstream ostr;

    // Create a json root array
    pjs::Array jsRoot;

    for( std::map< std::string, HNSDCaptureRecord* >::iterator rit = m_captureRecordMap.begin(); rit != m_captureRecordMap.end(); rit++ )
    { 
        // Create a json root object
        pjs::Object jsCapObj;

        HNSDCaptureRecord *capPtr = rit->second;

        // Fill in object fields
        jsCapObj.set( "id", capPtr->getID() );
        jsCapObj.set( "orderIndex", capPtr->getOrderIndex() );
        jsCapObj.set( "state", capPtr->getStateAsStr() );
        jsCapObj.set( "fileCount", capPtr->getFileCount() );

        // Add a file list array
        pjs::Array jsFileList;

        // Populate generated file data
        for( uint fidx = 0; fidx < capPtr->getFileCount(); fidx++ )
        {
            pjs::Object jsFileInfo;
            std::string fileName;
            std::string purpose;
            std::string tsStr;

            jsFileInfo.set( "index", fidx );

            capPtr->getFileInfo( fidx, fileName, purpose, tsStr );

            jsFileInfo.set( "filename", fileName );
            jsFileInfo.set( "purpose", purpose );
            jsFileInfo.set( "timestamp", tsStr );

            jsFileList.add( jsFileInfo );
        }

        jsCapObj.set( "fileList", jsFileList );

        // Add new placement object to return list
        jsRoot.add( jsCapObj );
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
HNSDImageManager::getCaptureJSON( std::string capID )
{
    std::ostringstream ostr;

    // Create a json root object
    pjs::Object jsRoot;

    std::map< std::string, HNSDCaptureRecord* >::iterator rit = m_captureRecordMap.find( capID );

    if( rit != m_captureRecordMap.end() )
    { 
        HNSDCaptureRecord *capPtr = rit->second;

        // Fill in object fields
        jsRoot.set( "id", capPtr->getID() );
        jsRoot.set( "orderIndex", capPtr->getOrderIndex() );
        jsRoot.set( "state", capPtr->getStateAsStr() );
        jsRoot.set( "fileCount", capPtr->getFileCount() );

        // Add a file list array
        pjs::Array jsFileList;

        // Populate generated file data
        for( uint fidx = 0; fidx < capPtr->getFileCount(); fidx++ )
        {
            pjs::Object jsFileInfo;
            std::string fileName;
            std::string purpose;
            std::string tsStr;

            jsFileInfo.set( "index", fidx );

            capPtr->getFileInfo( fidx, fileName, purpose, tsStr );

            jsFileInfo.set( "filename", fileName );
            jsFileInfo.set( "purpose", purpose );
            jsFileInfo.set( "timestamp", tsStr );

            jsFileList.add( jsFileInfo );
        }

        jsRoot.set( "fileList", jsFileList );
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

HNSDCaptureRecord*
HNSDImageManager::getNextPendingCapture()
{
    HNSDCaptureRecord *nextCap = NULL;

    for( std::map< std::string, HNSDCaptureRecord* >::iterator it = m_captureRecordMap.begin(); it != m_captureRecordMap.end(); it++ )
    {
        if( it->second->isPending() == false )
            continue;

        if( nextCap == NULL )
            nextCap = it->second;
        else if( it->second->getOrderIndex() < nextCap->getOrderIndex() )
            nextCap = it->second;
    }

    return nextCap;
}

IMM_RESULT_T
HNSDImageManager::getCapturePathAndFile( std::string captureID, uint fileIndex, std::string &rstPath )
{
    rstPath.clear();

    std::map< std::string, HNSDCaptureRecord* >::iterator it = m_captureRecordMap.find( captureID );

    if( it == m_captureRecordMap.end() )
        return IMM_RESULT_FAILURE;

    return it->second->getFilePath( fileIndex, rstPath );
}
