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

HNSDPipelineParameter::HNSDPipelineParameter()
{

}

HNSDPipelineParameter::~HNSDPipelineParameter()
{

}

HNSDPipelineParameterMap::HNSDPipelineParameterMap()
{
    
}

HNSDPipelineParameterMap::~HNSDPipelineParameterMap()
{

}

void
HNSDPipelineParameterMap::addParameter( std::string instance, std::string name, std::string defaultValue )
{

}

HNSDPipelineStepBase::HNSDPipelineStepBase( std::string instance )
{
    m_instance = instance;
}

HNSDPipelineStepBase::~HNSDPipelineStepBase()
{

}

std::string
HNSDPipelineStepBase::getInstance()
{
    return m_instance;
}

HNSDPBulkRotate::HNSDPBulkRotate( std::string instance )
: HNSDPipelineStepBase( instance )
{

}

HNSDPBulkRotate::~HNSDPBulkRotate()
{

}

void 
HNSDPBulkRotate::initSupportedParameters( HNSDPipelineManagerInterface *capture )
{
    // Get access to the parameters
    HNSDPipelineParameterMap *params = capture->getParamPtr();

    // Add the parameters that apply to this transform
    params->addParameter( getInstance(), "bulk_rotate_enable", "1" );
    params->addParameter( getInstance(), "bulk_rotate_degrees", "270" );
    params->addParameter( getInstance(), "result_image_index", "" );
}

bool
HNSDPBulkRotate::doesStepApply( HNSDPipelineManagerInterface *capture )
{
    return true;
}

IMM_RESULT_T
HNSDPBulkRotate::applyStep( HNSDPipelineManagerInterface *capture )
{
    std::cout << "HNSDPBulkRotate::applyStep - start" << std::endl;

    // Read image as grayscale
    cv::Mat srcImage;
    cv::Mat rotImage;

    std::string inFile = capture->getLastOutputPathAndFile();

    srcImage = cv::imread( inFile, cv::IMREAD_COLOR );

    if ( !srcImage.data )
    {
        printf("No image data \n");
        return IMM_RESULT_FAILURE;
    }

    // Print width and height
    uint ih = srcImage.rows;
    uint iw = srcImage.cols;
    printf( "HNSDPBulkRotate - width: %u  height: %u\n", iw, ih );

    cv::rotate( srcImage, rotImage, cv::ROTATE_90_COUNTERCLOCKWISE );

    std::string outFile = capture->registerNextFilename( "bulkRotate" );

    if( cv::imwrite( outFile, rotImage ) == false )
    {
        printf("Failed to write output file\n");
        return IMM_RESULT_FAILURE;
    }

    return IMM_RESULT_SUCCESS;
}

HNSDPCrop::HNSDPCrop( std::string instance )
: HNSDPipelineStepBase( instance )
{

}

HNSDPCrop::~HNSDPCrop()
{

}

void 
HNSDPCrop::initSupportedParameters( HNSDPipelineManagerInterface *capture )
{
    // Get access to the parameters
    HNSDPipelineParameterMap *params = capture->getParamPtr();

    // Add the parameters that apply to this transform
    params->addParameter( getInstance(), "crop_enable", "1" );
    params->addParameter( getInstance(), "crop_factors", "0.13, 0.22, 0.0, 0.0" );
    params->addParameter( getInstance(), "result_image_index", "" );
}

bool
HNSDPCrop::doesStepApply( HNSDPipelineManagerInterface *capture )
{
    return true;
}

IMM_RESULT_T
HNSDPCrop::applyStep( HNSDPipelineManagerInterface *capture )
{
    std::cout << "HNSDPCrop::applyStep - start" << std::endl;

    // Read image as grayscale
    cv::Mat srcImage;
    cv::Mat rotImage;

    std::string inFile = capture->getLastOutputPathAndFile();

    srcImage = cv::imread( inFile, cv::IMREAD_COLOR );

    if ( !srcImage.data )
    {
        printf("No image data \n");
        return IMM_RESULT_FAILURE;
    }

    // Print width and height
    uint ih = srcImage.rows;
    uint iw = srcImage.cols;
    printf( "HNSDPCrop - width: %u  height: %u\n", iw, ih );

    // Crop to the region of interest
    double cropfactor[4];
    cropfactor[0] = 0.13;
    cropfactor[1] = 0.22;
    cropfactor[2] = 0.0;
    cropfactor[3] = 0.0;

    uint cx1 = (uint)( (double)ih * cropfactor[0] );
    uint cx2 = ih - (uint)( (double)ih * cropfactor[1] );

    uint cy1 = (uint)( (double)iw * cropfactor[2] );
    uint cy2 = iw - (uint)( (double)ih * cropfactor[3] );

    printf( "Crop Offsets - cx1: %u, cx2: %u, cy1: %u, cy2: %u\n", cx1, cx2, cy1, cy2 );

    cv::Mat cropImage = srcImage( cv::Range( cx1, cx2 ), cv::Range( cy1, cy2 ) );

    std::string outFile = capture->registerNextFilename( "crop" );

    if( cv::imwrite( outFile, cropImage ) == false )
    {
        printf("Failed to write output file\n");
        return IMM_RESULT_FAILURE;
    }

    return IMM_RESULT_SUCCESS;
}

HNSDPipeline::HNSDPipeline()
{

}

HNSDPipeline::~HNSDPipeline()
{

}

IMM_RESULT_T
HNSDPipeline::init()
{
    // Initialize the pipeline transformations.
    HNSDPBulkRotate* bulkRotate = new HNSDPBulkRotate( "firstRotate" );
    m_pipeline.push_back( bulkRotate );

    HNSDPCrop* crop = new HNSDPCrop( "cropRaw" );
    m_pipeline.push_back( crop );

    return IMM_RESULT_SUCCESS;
}

uint 
HNSDPipeline::getStepCount()
{
    return m_pipeline.size();
}

HNSDPipelineStepBase*
HNSDPipeline::getStepByIndex( uint index )
{
    if( index >= m_pipeline.size() )
        return NULL;

    return m_pipeline[ index ];
}

HNSDCaptureRecord::HNSDCaptureRecord( HNSDIMRootInterface *infoIntf )
{
    m_infoIntf = infoIntf;

    m_orderIndex = 0;

    m_executionState = HNSDCAP_EXEC_STATE_NOTSET;

    m_curStep = NULL;

    m_curStepIndex = 0;
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

HNSDPipelineParameterMap*
HNSDCaptureRecord::getParamPtr()
{
    return &m_params;
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

        case HNSDCAP_EXEC_STATE_CAPTURE:
            return "Capture";

        case HNSDCAP_EXEC_STATE_CAPTURE_WAIT:
            return "CaptureWait";

        case HNSDCAP_EXEC_STATE_MOVE:
            return "Move";

        case HNSDCAP_EXEC_STATE_MOVE_WAIT:
            return "MoveWait";

        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS:
            return "ImageProcess";

        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS_WAIT:
            return "ImageProcessWait";

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

        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS:
            return HNSDCAP_ACTION_START_PIPELINE_STEP;

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
        break;

        case HNSDCAP_EXEC_STATE_MOVE:
            m_executionState = HNSDCAP_EXEC_STATE_MOVE_WAIT;
        break;

        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS:
            m_executionState = HNSDCAP_EXEC_STATE_IMAGE_PROCESS_WAIT;
        break;

        case HNSDCAP_EXEC_STATE_CAPTURE_WAIT:
        case HNSDCAP_EXEC_STATE_MOVE_WAIT:
        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS_WAIT:
        case HNSDCAP_EXEC_STATE_PENDING:
        case HNSDCAP_EXEC_STATE_NOTSET:
        case HNSDCAP_EXEC_STATE_COMPLETE:
        break;
    }

    std::cout << "CaptureRecord - StartedAction - id: " << m_id << "  nextState: " << m_executionState << std::endl;
}

void
HNSDCaptureRecord::completedAction()
{
    switch( m_executionState )
    {
        case HNSDCAP_EXEC_STATE_CAPTURE_WAIT:
            m_executionState = HNSDCAP_EXEC_STATE_MOVE;
        break;

        case HNSDCAP_EXEC_STATE_MOVE_WAIT:
            if( findNextPipelineStage() == true )
                m_executionState = HNSDCAP_EXEC_STATE_IMAGE_PROCESS;
            else
                m_executionState = HNSDCAP_EXEC_STATE_COMPLETE;
        break;

        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS_WAIT:
            m_curStep = NULL;
            m_curStepIndex += 1;
            if( findNextPipelineStage() == true )
                m_executionState = HNSDCAP_EXEC_STATE_IMAGE_PROCESS;
            else
                m_executionState = HNSDCAP_EXEC_STATE_COMPLETE;
        break;

        case HNSDCAP_EXEC_STATE_CAPTURE:
        case HNSDCAP_EXEC_STATE_MOVE:
        case HNSDCAP_EXEC_STATE_IMAGE_PROCESS:
        case HNSDCAP_EXEC_STATE_PENDING:
        case HNSDCAP_EXEC_STATE_NOTSET:
        case HNSDCAP_EXEC_STATE_COMPLETE:
        break;
    }

    std::cout << "CaptureRecord - CompletedAction - id: " << m_id << "  nextState: " << m_executionState << std::endl;
}

bool
HNSDCaptureRecord::findNextPipelineStage()
{
    HNSDPipeline *pline = m_infoIntf->getPipelinePtr();

    m_curStep = NULL;

    while( m_curStepIndex < pline->getStepCount() )
    {
        HNSDPipelineStepBase *step = pline->getStepByIndex( m_curStepIndex );

        if( step->doesStepApply( this ) )
        {
            m_curStep = step;
            return true;
        }

        m_curStepIndex += 1;
    }

    return false;
}

void
HNSDCaptureRecord::executeStep()
{
    if( m_curStep == NULL )
        return;

    m_curStep->applyStep( this );
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
    IMM_RESULT_T result = IMM_RESULT_SUCCESS;

    result = m_pipeline.init();
    if( result != IMM_RESULT_SUCCESS )
        return result;

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

HNSDPipeline* 
HNSDImageManager::getPipelinePtr()
{
    return &m_pipeline;
}
