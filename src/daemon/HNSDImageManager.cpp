#include <sys/mman.h>
#include <sys/time.h>

#include <iostream>
#include <sstream>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

#include "HNSDImageManager.h"

namespace pjs = Poco::JSON;

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
    char idStr[64];
    
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

std::string
HNSDCaptureRecord::registerNextStepFilename()
{
    return "";
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


