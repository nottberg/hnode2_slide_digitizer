#include <sys/time.h>

#include <iostream>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

#include "HNSDImageManager.h"

namespace pjs = Poco::JSON;

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

bool
HNSDCaptureRecord::isPending()
{
    return (m_executionState == HNSDCAP_EXEC_STATE_PENDING) ? true : false;
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
    m_storageMgr = NULL;
    m_pipelineMgr = NULL;
    m_nextOrderIndex = 0;
}

HNSDImageManager::~HNSDImageManager()
{

}

IMM_RESULT_T
HNSDImageManager::start( HNSDStorageManager *storageMgr, HNSDPipelineManagerIntf *pipelineMgr )
{
    IMM_RESULT_T result = IMM_RESULT_SUCCESS;

    m_storageMgr = storageMgr;
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
    m_pipelineMgr->allocatePipeline( HNSDP_TYPE_IMAGE_CAPTURE, newCapRec->getID(), newCapRec, &pipeline );

    newCapRec->setPipeline( pipeline );

    m_captureRecordMap.insert( std::pair< std::string, HNSDCaptureRecord* >( newCapRec->getID(), newCapRec ) );

    m_pipelineMgr->submitPipelineForExecution( pipeline );

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

    // Create list of capture objects
    for( std::map< std::string, HNSDCaptureRecord* >::iterator rit = m_captureRecordMap.begin(); rit != m_captureRecordMap.end(); rit++ )
    { 
        // Create a json capture object
        pjs::Object jsCapObj;

        HNSDCaptureRecord *capPtr = rit->second;

        // Fill in object fields
        jsCapObj.set( "id", capPtr->getID() );
        jsCapObj.set( "orderIndex", capPtr->getOrderIndex() );
        jsCapObj.set( "state", capPtr->getStateAsStr() );

        std::vector< std::string > fileIDList;
        m_storageMgr->getFileIDListForOwner( capPtr->getID(), fileIDList );

        jsCapObj.set( "fileCount", fileIDList.size() );

        // Add a file list array
        pjs::Array jsFileIDList;

        // Populate generated file data
        for( std::vector< std::string >::iterator it = fileIDList.begin(); it != fileIDList.end(); it++ )
        {
            jsFileIDList.add( *it );
        }
        jsCapObj.set( "fileIDList", jsFileIDList );

        // Add new capture object to return list
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

        std::vector< std::string > fileIDList;
        m_storageMgr->getFileIDListForOwner( capPtr->getID(), fileIDList );

        jsRoot.set( "fileCount", fileIDList.size() );

        // Add a file list array
        pjs::Array jsFileIDList;

        // Populate generated file data
        for( std::vector< std::string >::iterator it = fileIDList.begin(); it != fileIDList.end(); it++ )
        {
            jsFileIDList.add( *it );
        }
        
        jsRoot.set( "fileIDList", jsFileIDList );
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

#if 0
IMM_RESULT_T
HNSDImageManager::getCapturePathAndFile( std::string captureID, uint fileIndex, std::string &rstPath )
{
    rstPath.clear();

    std::map< std::string, HNSDCaptureRecord* >::iterator it = m_captureRecordMap.find( captureID );

    if( it == m_captureRecordMap.end() )
        return IMM_RESULT_FAILURE;

    return it->second->getFilePath( fileIndex, rstPath );
}
#endif

std::string
HNSDImageManager::getDefaultCaptureParameterListJSON()
{
    std::ostringstream ostr;

    // Create a json root array
    pjs::Array jsRoot;

    // Create a temporary pipeline
    //HNSDCaptureRecord tmpCapture(this);
    //HNSDPipeline *pipeline;
    //m_pipelineMgr->allocatePipeline( HNSDP_TYPE_IMAGE_CAPTURE, &tmpCapture, &pipeline );

    // Get its parameter list

#if 0    
    // For each parameter create a return object
    for( std::map< std::string, HNSDCaptureRecord* >::iterator rit = m_captureRecordMap.begin(); rit != m_captureRecordMap.end(); rit++ )
    { 
        // Create a json capture object
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

        // Add new capture object to return list
        jsRoot.add( jsCapObj );
    }
#endif

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
