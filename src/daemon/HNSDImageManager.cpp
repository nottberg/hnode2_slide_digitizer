#include <sys/mman.h>
#include <sys/time.h>

#include <iostream>
#include <sstream>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

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

std::string 
HNSDCaptureFile::getPathAndFile()
{
    char tmpFN[256];
    std::string fullPath;

    sprintf( tmpFN, "hnsd_%s_%u_%s.jpg", m_timestampStr.c_str(), m_indexNum, m_purpose.c_str() );
    
    fullPath = m_path + "/" + tmpFN;

    return fullPath;
}


HNSDCaptureRecord::HNSDCaptureRecord( HNSDCaptureInfoInterface *infoIntf )
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

bool
HNSDCaptureRecord::isPending()
{
    return (m_executionState == HNSDCAP_EXEC_STATE_PENDING) ? true : false;
}

std::string
HNSDCaptureRecord::registerNextFilename( std::string purpose )
{
    HNSDCaptureFile newFile;

    newFile.setPath( m_infoIntf->getStorageRootPath() );
    newFile.setTimestampStr( m_timestampStr );
    newFile.setType( HNSDCAP_FT_JPEG );
    newFile.setPurpose( purpose );
    newFile.setIndex( m_nextFileIndex );
    m_nextFileIndex += 1;

    m_fileList.push_back( newFile );

    return newFile.getPathAndFile();
}

HNSDCAP_ACTION_T
HNSDCaptureRecord::checkNextStep()
{
    switch( m_executionState )
    {
        case HNSDCAP_EXEC_STATE_PENDING:
        case HNSDCAP_EXEC_STATE_CAPTURE_WAIT:
        case HNSDCAP_EXEC_STATE_MOVE_WAIT:
        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS_WAIT:
            return HNSDCAP_ACTION_WAIT;

        case HNSDCAP_EXEC_STATE_CAPTURE:
            return HNSDCAP_ACTION_START_CAPTURE;

        case HNSDCAP_EXEC_STATE_MOVE:
            return HNSDCAP_ACTION_START_ADVANCE;

//        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS:

        case HNSDCAP_EXEC_STATE_NOTSET:
        case HNSDCAP_EXEC_STATE_COMPLETE:
            break;
    }

    return HNSDCAP_ACTION_COMPLETE;
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
        m_executionState = HNSDCAP_EXEC_STATE_CAPTURE;
}

void
HNSDCaptureRecord::startedAction()
{
    switch( m_executionState )
    {
        case HNSDCAP_EXEC_STATE_CAPTURE:
            m_executionState = HNSDCAP_EXEC_STATE_CAPTURE_WAIT;

        case HNSDCAP_EXEC_STATE_MOVE:
            m_executionState = HNSDCAP_EXEC_STATE_MOVE_WAIT;

        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS:
            m_executionState = HNSDCAP_EXEC_STATE_IMAGE_PROCESS_WAIT;

        case HNSDCAP_EXEC_STATE_CAPTURE_WAIT:
        case HNSDCAP_EXEC_STATE_MOVE_WAIT:
        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS_WAIT:
        case HNSDCAP_EXEC_STATE_PENDING:
        case HNSDCAP_EXEC_STATE_NOTSET:
        case HNSDCAP_EXEC_STATE_COMPLETE:
            break;
    }
}

void
HNSDCaptureRecord::completedAction()
{
    switch( m_executionState )
    {
        case HNSDCAP_EXEC_STATE_CAPTURE_WAIT:
            m_executionState = HNSDCAP_EXEC_STATE_MOVE;

        case HNSDCAP_EXEC_STATE_MOVE_WAIT:
            m_executionState = HNSDCAP_EXEC_STATE_COMPLETE;

        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS_WAIT:
            m_executionState = HNSDCAP_EXEC_STATE_COMPLETE;

        case HNSDCAP_EXEC_STATE_CAPTURE:
        case HNSDCAP_EXEC_STATE_MOVE:
        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS:
        case HNSDCAP_EXEC_STATE_PENDING:
        case HNSDCAP_EXEC_STATE_NOTSET:
        case HNSDCAP_EXEC_STATE_COMPLETE:
            break;
    }
}


HNSDImageManager::HNSDImageManager()
{
    m_nextOrderIndex = 0;
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
    HNSDCaptureRecord newCapRec( this );

    gettimeofday( &tv,NULL );

    timestamp = 1000000 * tv.tv_sec + tv.tv_usec;

    newCapRec.generateNewID( timestamp, m_nextOrderIndex );
    m_nextOrderIndex += 1;

            
    //HNSDHardwareOperation *newOp = new HNSDHardwareOperation( idStr, HNHW_OPTYPE_SINGLE_CAPTURE );

    //CaptureRequest *crPtr = newOp->getCaptureRequestPtr();

    //crPtr->setImageFormat( CS_STILLMODE_YUV420, 4624, 3472 );

    //m_opMap.insert( std::pair< std::string, HNSDHardwareOperation* >( newOp->getID(), newOp ) );

    // Return the newly created capture id
    //m_curUserAction->setNewID( newOp->getID() );

    // Kick off the capture thread
    //m_hardwareCtrl.startOperation( newOp );

    newCapRec.makePending();

    m_captureRecordMap.insert( std::pair< std::string, HNSDCaptureRecord >( newCapRec.getID(), newCapRec ) );

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

    for( std::map< std::string, HNSDCaptureRecord >::iterator rit = m_captureRecordMap.begin(); rit != m_captureRecordMap.end(); rit++ )
    { 
        // Add new placement object to return list
        jsRoot.add( rit->first );
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

    std::map< std::string, HNSDCaptureRecord >::iterator rit = m_captureRecordMap.find( capID );

    if( rit != m_captureRecordMap.end() )
    { 
        // Add new placement object to return list
        jsRoot.set( "id", rit->second.getID() );
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

    for( std::map< std::string, HNSDCaptureRecord >::iterator it = m_captureRecordMap.begin(); it != m_captureRecordMap.end(); it++ )
    {
        if( it->second.isPending() == false )
            continue;

        if( nextCap == NULL )
            nextCap = &(it->second);
        else if( it->second.getOrderIndex() < nextCap->getOrderIndex() )
            nextCap = &(it->second);
    }

    return nextCap;
}


