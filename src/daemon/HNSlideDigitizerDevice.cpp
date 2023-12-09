#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/eventfd.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Checksum.h"
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

#include <hnode2/HNodeDevice.h>

#include "HNSDAction.h"
#include "HNSlideDigitizerDevicePrivate.h"

using namespace Poco::Util;

namespace pjs = Poco::JSON;
namespace pdy = Poco::Dynamic;

// Forward declaration
extern const std::string g_HNode2TestRest;

void 
HNSlideDigitizerDevice::defineOptions( OptionSet& options )
{
    ServerApplication::defineOptions( options );

    options.addOption(
              Option("help", "h", "display help").required(false).repeatable(false));

    options.addOption(
              Option("debug","d", "Enable debug logging").required(false).repeatable(false));

    options.addOption(
              Option("instance", "", "Specify the instance name of this daemon.").required(false).repeatable(false).argument("name"));

}

void 
HNSlideDigitizerDevice::handleOption( const std::string& name, const std::string& value )
{
    ServerApplication::handleOption( name, value );
    if( "help" == name )
        _helpRequested = true;
    else if( "debug" == name )
        _debugLogging = true;
    else if( "instance" == name )
    {
         _instancePresent = true;
         _instance = value;
    }
}

void 
HNSlideDigitizerDevice::displayHelp()
{
    HelpFormatter helpFormatter(options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("[options]");
    helpFormatter.setHeader("HNode2 Switch Daemon.");
    helpFormatter.format(std::cout);
}

int 
HNSlideDigitizerDevice::main( const std::vector<std::string>& args )
{
    m_instanceName = "default";
    if( _instancePresent == true )
        m_instanceName = _instance;

    m_devState = HNSD_DEVSTATE_INIT;

    // Initialize the capture queue
    //m_nextOpID = 0;
    m_curUserAction = NULL;
    m_userActionQueue.init();

    m_curCapture = NULL;
    m_activeHWOp = NULL;
    
    // Initialize the image manager object
    m_imageMgr.start();

    // Initialize the hardware control object
    m_hardwareCtrl.init( &m_hardwareNotifyTrigger );

    m_hnodeDev.setDeviceType( HNODE_SLIDE_DIGITIZER_DEVTYPE );
    m_hnodeDev.setInstance( m_instanceName );

    HNDEndpoint hndEP;

    hndEP.setDispatch( "hnode2Test", this );
    hndEP.setOpenAPIJson( g_HNode2TestRest ); 

    m_hnodeDev.addEndpoint( hndEP );

    m_hnodeDev.setRestPort(8088);

    std::cout << "Looking for config file" << std::endl;
    
    if( configExists() == false )
    {
        initConfig();
    }

    readConfig();

    // Setup the event loop
    m_testDeviceEvLoop.setup( this );

    m_testDeviceEvLoop.setupTriggerFD( m_configUpdateTrigger );
    m_testDeviceEvLoop.setupTriggerFD( m_hardwareNotifyTrigger );

    // Register some format strings
    m_hnodeDev.registerFormatString( "Error: %u", m_errStrCode );
    m_hnodeDev.registerFormatString( "This is a test note.", m_noteStrCode );

    // Enable the health monitoring and add some fake components
    //m_hnodeDev.enableHealthMonitoring();

    //m_hnodeDev.getHealthRef().registerComponent( "test device hc1", HNDH_ROOT_COMPID, m_hc1ID );
    //std::cout << "Health Component 1 id: " << m_hc1ID << std::endl;
    //m_hnodeDev.getHealthRef().registerComponent( "test device hc2", HNDH_ROOT_COMPID, m_hc2ID );
    //std::cout << "Health Component 2 id: " << m_hc2ID << std::endl;
    //m_hnodeDev.getHealthRef().registerComponent( "test device hc2.1", m_hc2ID, m_hc3ID );
    //std::cout << "Health Component 3 id: " << m_hc3ID << std::endl;

    //m_healthStateSeq = 0;
    //generateNewHealthState();

    // Hook the local action queue into the event loop
    if( m_testDeviceEvLoop.addFDToEPoll( m_userActionQueue.getEventFD() ) != HNEP_RESULT_SUCCESS )
    {
        // Failed to add client socket.
        std::cerr << "ERROR: Failed to add local capture queue to event loop." << std::endl;
        return Application::EXIT_SOFTWARE;
    }

    // Start accepting device notifications
    m_hnodeDev.setNotifySink( this );

    // Start up the hnode device
    m_hnodeDev.start();

    m_devState = HNSD_DEVSTATE_IDLE;

    // Start event processing loop
    m_testDeviceEvLoop.run();

    waitForTerminationRequest();

    return Application::EXIT_OK;
}

HNSD_DEVSTATE_T
HNSlideDigitizerDevice::getState()
{
    return m_devState;
}

#if 0
void
HNSlideDigitizerDevice::generateNewHealthState()
{
    m_hnodeDev.getHealthRef().startUpdateCycle( time(NULL) );

    switch( m_healthStateSeq )
    {
        case 0:
          m_hnodeDev.getHealthRef().setComponentStatus( HNDH_ROOT_COMPID, HNDH_CSTAT_OK );
          m_hnodeDev.getHealthRef().setComponentStatus( m_hc1ID, HNDH_CSTAT_OK );
          m_hnodeDev.getHealthRef().setComponentStatus( m_hc2ID, HNDH_CSTAT_OK );
          m_hnodeDev.getHealthRef().setComponentStatus( m_hc3ID, HNDH_CSTAT_OK );
        break;

        case 1:
          m_hnodeDev.getHealthRef().setComponentStatus( HNDH_ROOT_COMPID, HNDH_CSTAT_OK );

          m_hnodeDev.getHealthRef().setComponentStatus( m_hc1ID, HNDH_CSTAT_FAILED );
          m_hnodeDev.getHealthRef().setComponentErrMsg( m_hc1ID, 200, m_errStrCode, 200 );
          
          m_hnodeDev.getHealthRef().setComponentStatus( m_hc2ID, HNDH_CSTAT_OK );
          m_hnodeDev.getHealthRef().setComponentStatus( m_hc3ID, HNDH_CSTAT_OK );
        break;

        case 2:
          m_hnodeDev.getHealthRef().setComponentStatus( HNDH_ROOT_COMPID, HNDH_CSTAT_OK );

          m_hnodeDev.getHealthRef().setComponentStatus( m_hc1ID, HNDH_CSTAT_OK );
          m_hnodeDev.getHealthRef().clearComponentErrMsg( m_hc1ID );

          m_hnodeDev.getHealthRef().setComponentStatus( m_hc2ID, HNDH_CSTAT_OK );

          m_hnodeDev.getHealthRef().setComponentStatus( m_hc3ID, HNDH_CSTAT_FAILED );
          m_hnodeDev.getHealthRef().setComponentErrMsg( m_hc3ID, 400, m_errStrCode, 400 );
        break;

        case 3:
          m_hnodeDev.getHealthRef().setComponentStatus( HNDH_ROOT_COMPID, HNDH_CSTAT_OK );
          m_hnodeDev.getHealthRef().setComponentStatus( m_hc1ID, HNDH_CSTAT_OK );

          m_hnodeDev.getHealthRef().setComponentStatus( m_hc2ID, HNDH_CSTAT_OK );
          m_hnodeDev.getHealthRef().setComponentNote( m_hc2ID, m_noteStrCode );

          m_hnodeDev.getHealthRef().setComponentStatus( m_hc3ID, HNDH_CSTAT_OK );
          m_hnodeDev.getHealthRef().clearComponentErrMsg( m_hc3ID );
        break;

        case 4:
          m_hnodeDev.getHealthRef().setComponentStatus( HNDH_ROOT_COMPID, HNDH_CSTAT_OK );
          m_hnodeDev.getHealthRef().setComponentStatus( m_hc1ID, HNDH_CSTAT_OK );
          m_hnodeDev.getHealthRef().setComponentStatus( m_hc2ID, HNDH_CSTAT_OK );
          m_hnodeDev.getHealthRef().clearComponentNote( m_hc2ID );
          m_hnodeDev.getHealthRef().setComponentStatus( m_hc3ID, HNDH_CSTAT_OK );
        break;
    }

    // Advance to next simulated health state.
    m_healthStateSeq += 1;
    if( m_healthStateSeq > 4 )
      m_healthStateSeq = 0;

    bool changed = m_hnodeDev.getHealthRef().completeUpdateCycle();
} 
#endif

bool 
HNSlideDigitizerDevice::configExists()
{
    HNodeConfigFile cfgFile;

    return cfgFile.configExists( HNODE_SLIDE_DIGITIZER_DEVTYPE, m_instanceName );
}

HNSDD_RESULT_T
HNSlideDigitizerDevice::initConfig()
{
    HNodeConfigFile cfgFile;
    HNodeConfig     cfg;

    m_hnodeDev.initConfigSections( cfg );

    cfg.debugPrint(2);
    
    std::cout << "Saving config..." << std::endl;
    if( cfgFile.saveConfig( HNODE_SLIDE_DIGITIZER_DEVTYPE, m_instanceName, cfg ) != HNC_RESULT_SUCCESS )
    {
        std::cout << "ERROR: Could not save initial configuration." << std::endl;
        return HNSDD_RESULT_FAILURE;
    }

    return HNSDD_RESULT_SUCCESS;
}

HNSDD_RESULT_T
HNSlideDigitizerDevice::readConfig()
{
    HNodeConfigFile cfgFile;
    HNodeConfig     cfg;

    if( configExists() == false )
        return HNSDD_RESULT_FAILURE;

    std::cout << "Loading config..." << std::endl;

    if( cfgFile.loadConfig( HNODE_SLIDE_DIGITIZER_DEVTYPE, m_instanceName, cfg ) != HNC_RESULT_SUCCESS )
    {
        std::cout << "ERROR: Could not load saved configuration." << std::endl;
        return HNSDD_RESULT_FAILURE;
    }
  
    std::cout << "cl1" << std::endl;
    m_hnodeDev.readConfigSections( cfg );

    std::cout << "Config loaded" << std::endl;

    return HNSDD_RESULT_SUCCESS;
}

HNSDD_RESULT_T
HNSlideDigitizerDevice::updateConfig()
{
    HNodeConfigFile cfgFile;
    HNodeConfig     cfg;

    m_hnodeDev.updateConfigSections( cfg );

    cfg.debugPrint(2);
    
    std::cout << "Saving config..." << std::endl;
    if( cfgFile.saveConfig( HNODE_SLIDE_DIGITIZER_DEVTYPE, m_instanceName, cfg ) != HNC_RESULT_SUCCESS )
    {
        std::cout << "ERROR: Could not save configuration." << std::endl;
        return HNSDD_RESULT_FAILURE;
    }
    std::cout << "Config saved" << std::endl;

    return HNSDD_RESULT_SUCCESS;
}

void
HNSlideDigitizerDevice::loopIteration()
{
    // If no current work then check for pending work
    if( m_curCapture == NULL )
    {
        // Check if any new captures are ready to execute
        m_curCapture = m_imageMgr.getNextPendingCapture();

        std::cout << "HNManagementDevice::loopIteration() - getNextPending: " << m_curCapture << std::endl;

        // Signal that it has become active
        if( m_curCapture != NULL )
        {
          std::cout << "HNManagementDevice::loopIteration() - New Active Capture: " << m_curCapture->getID() << std::endl;
          m_curCapture->makeActive();
        }
    }

    // If there is active work then see if there
    // are additional step to be taken
    if( m_curCapture != NULL )
    {
        HNSDCAP_ACTION_T nextStep;
  
        nextStep = m_curCapture->checkNextStep();

        std::cout << "HNManagementDevice::loopIteration() - Capture - Next Step: " << nextStep << std::endl;

        switch( nextStep )
        {
          case HNSDCAP_ACTION_WAIT:
          break;

          case HNSDCAP_ACTION_COMPLETE:
          {
            std::cout << "Current Capture complete" << std::endl;
            m_curCapture = NULL;
          }
          break;

          case HNSDCAP_ACTION_START_CAPTURE:
          {           
            //char idStr[64];

            // Create a new capture record.
            //sprintf( idStr, "hwop%u", m_nextOpID );
            //m_nextOpID += 1;

            std::cout << "HNManagementDevice::loopIteration() - Start hardware capture" << std::endl;

            m_activeHWOp = new HNSDHardwareOperation( m_curCapture->getID(), HNHW_OPTYPE_SINGLE_CAPTURE );

            CaptureRequest *crPtr = m_activeHWOp->getCaptureRequestPtr();

            crPtr->setImageFormat( CS_STILLMODE_YUV420, 4624, 3472 );

            crPtr->setFileAndPath( m_curCapture->registerNextFilename( "capture" ) );

            // Kick off the capture thread
            m_hardwareCtrl.startOperation( m_activeHWOp );

            // Indicate the requested action has started.
            m_curCapture->startedAction();
          }
          break;

          case HNSDCAP_ACTION_START_ADVANCE:
          break;
        }
    }

}

void
HNSlideDigitizerDevice::timeoutEvent()
{
    // std::cout << "HNManagementDevice::timeoutEvent() - entry" << std::endl;

}

void
HNSlideDigitizerDevice::fdEvent( int sfd )
{
    std::cout << "HNManagementDevice::fdEvent() - entry: " << sfd << std::endl;

    if( m_configUpdateTrigger.isMatch( sfd ) )
    {
        std::cout << "m_configUpdateTrigger - updating config" << std::endl;
        m_configUpdateTrigger.reset();
        updateConfig();
    }
    else if( m_hardwareNotifyTrigger.isMatch( sfd ) )
    {
        HNSD_HWSTATE_T opState = m_hardwareCtrl.getOperationState();

        m_hardwareNotifyTrigger.reset();

        std::cout << "=== HW notify state change - state: " << opState << std::endl;
        switch( opState )
        {
            case HNSD_HWSTATE_NOTSET:
            case HNSD_HWSTATE_INIT:
            break;

            case HNSD_HWSTATE_IDLE:
                // Check to see if another operation is queued.
            break;

            case HNSD_HWSTATE_CAPTURE_START:
            case HNSD_HWSTATE_CAPTURE_FOCUS_WAIT:
            case HNSD_HWSTATE_CAPTURE_FOCUS_FAILURE:
            case HNSD_HWSTATE_CAPTURE_POLL_REQUEST:
            case HNSD_HWSTATE_CAPTURE_WAIT_REQ:
            case HNSD_HWSTATE_CAPTURE_RAW_IMAGE_AQUIRED:
            case HNSD_HWSTATE_CAPTURE_POST_PROCESS:
            case HNSD_HWSTATE_CAPTURE_COMPLETE:
            case HNSD_HWSTATE_CAROSEL_CAPTURING:
            case HNSD_HWSTATE_CAROSEL_IMGPROC:
            case HNSD_HWSTATE_CAROSEL_ADVANCING:
            case HNSD_HWSTATE_OPERATION_START:
            break;

            case HNSD_HWSTATE_OPERATION_COMPLETE:
                m_hardwareCtrl.finishOperation();
                m_curCapture->completedAction();
                delete m_activeHWOp;
                m_activeHWOp = NULL;
            break;

            case HNSD_HWSTATE_OPERATION_FAILURE:
                m_hardwareCtrl.finishOperation();
                m_curCapture->completedAction();
                delete m_activeHWOp;
                m_activeHWOp = NULL;
            break;
        }
        
    }
    else if( sfd == m_userActionQueue.getEventFD() )
    {
        // Verify that we can handle a new action,
        // otherwise just spin.
        std::cout << "Action Queue: " << getState() << std::endl;
        //if( getState() != HNID_STATE_READY )
        //    return;

        // Start the new action
        startAction();
    }

}

void
HNSlideDigitizerDevice::fdError( int sfd )
{
    std::cout << "HNManagementDevice::fdError() - entry: " << sfd << std::endl;

}

void
HNSlideDigitizerDevice::hndnConfigChange( HNodeDevice *parent )
{
    std::cout << "HNManagementDevice::hndnConfigChange() - entry" << std::endl;
    m_configUpdateTrigger.trigger();
}

void
HNSlideDigitizerDevice::startAction()
{
    //HNSWDPacketClient packet;
    HNID_ACTBIT_T  actBits = HNID_ACTBIT_CLEAR;

    // Verify that we are in a state that will allow actions to start
//    if( getState() != HNID_STATE_READY )
//    {
//        return;
//    }

    // Pop the action from the queue
    m_curUserAction = ( HNSDAction* ) m_userActionQueue.aquireRecord();

    std::cout << "Action aquired - type: " << m_curUserAction->getType()  << "  thread: " << std::this_thread::get_id() << std::endl;

    switch( m_curUserAction->getType() )
    {
        case HNSD_AR_TYPE_START_SINGLE_CAPTURE:
        {
            m_imageMgr.createCaptures( 1, true );

            // Return the newly created capture id
            //m_curUserAction->setNewID( newOp->getID() );

            // Done with this request
            actBits = HNID_ACTBIT_COMPLETE;
        }
        break;

        case HNSD_AR_TYPE_GET_CAPTURE_LIST:
        {
            // Get the capture list json string
            // from the image manager and 
            // store it in the current action
            // so it can be returned to the requestor
            m_curUserAction->setResponseJSON( m_imageMgr.getCaptureListJSON() );

            // Done with this request
            actBits = HNID_ACTBIT_COMPLETE;
        }
        break;

        case HNSD_AR_TYPE_GET_CAPTURE_INFO:
        {
            // Check if the provided capture id is valid,
            // return error if not.

            // Get the capture list json string
            // from the image manager and 
            // store it in the current action
            // so it can be returned to the requestor
            m_curUserAction->setResponseJSON( m_imageMgr.getCaptureJSON( m_curUserAction->getRequestCaptureID() ) );

            // Done with this request
            actBits = HNID_ACTBIT_COMPLETE;
        }
        break;

        case HNSD_AR_TYPE_DELETE_CAPTURE:
        {
            // Tell the image manager to clean-up 
            // capture and associated data.
            m_imageMgr.deleteCapture( m_curUserAction->getRequestCaptureID() );

            // Done with this request
            actBits = HNID_ACTBIT_COMPLETE;
        }
        break;        
    }

    // The configuration was changed so commit
    // it to persistent copy
    if( actBits & HNID_ACTBIT_UPDATE )
    {
        // Commit config update
//        updateConfig();
    }

    // There was an error, complete with error
    if( actBits & HNID_ACTBIT_ERROR )
    {
        std::cout << "Failing action: " << m_curUserAction->getType() << "  thread: " << std::this_thread::get_id() << std::endl;

        // Signal failure
        m_curUserAction->complete( false );
        m_curUserAction = NULL;
        return;
    }

    // Request has been completed successfully
    if( actBits & HNID_ACTBIT_COMPLETE )
    {
        std::cout << "Completing action: " << m_curUserAction->getType() << "  thread: " << std::this_thread::get_id() << std::endl;

        // Done with this request
        m_curUserAction->complete( true );
        m_curUserAction = NULL;
    }
    
}

void 
HNSlideDigitizerDevice::dispatchEP( HNodeDevice *parent, HNOperationData *opData )
{
    HNSDAction action;

    std::cout << "HNSlideDigitizerDevice::dispatchEP() - entry" << std::endl;
    std::cout << "  dispatchID: " << opData->getDispatchID() << std::endl;
    std::cout << "  opID: " << opData->getOpID() << std::endl;
    std::cout << "  thread: " << std::this_thread::get_id() << std::endl;

    std::string opID = opData->getOpID();
          
    // GET "/hnode2/slide-digitizer/status"
    if( "getStatus" == opID )
    {
        std::cout << "=== Get Status Request ===" << std::endl;
    
        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );

        // Create a json status object
        pjs::Object jsRoot;
        jsRoot.set( "overallStatus", "OK" );

        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{ 
            pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }

        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
        return;
    }
    // GET "/hnode2/slide-digitizer/cameras"
    else if( "getCameraList" == opID )
    {
        std::cout << "=== Get Capture List Request ===" << std::endl;

        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );

        // Create a json root object
        pjs::Array jsRoot;

        // Get the list of camera ids
        std::vector< std::string > idList;

        m_hardwareCtrl.getCameraIDList( idList );

        for( std::vector< std::string >::iterator it = idList.begin(); it != idList.end(); it++ )
        {
            jsRoot.add( *it );
        }
          
        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{ 
            pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }
            
        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
        return;
    }
    // GET "/hnode2/slide-digitizer/cameras/{cameraid}"
    else if( "getCameraInfo" == opID )
    {
        std::string cameraID;

        if( opData->getParam( "cameraid", cameraID ) == true )
        {
            opData->responseSetStatusAndReason( HNR_HTTP_INTERNAL_SERVER_ERROR );
            opData->responseSend();
            return; 
        }

        std::cout << "=== Get Camera Info Request (id: " << cameraID << ") ===" << std::endl;

        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );
        
        // Create a json root object
        pjs::Array jsRoot;

        pjs::Object w1Obj;
        w1Obj.set( "id", cameraID );
        w1Obj.set( "color", "black" );
        jsRoot.add( w1Obj );
          
        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{ 
            pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }            
        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
        return;
    }
    // GET "/hnode2/slide-digitizer/cameras/{cameraid}/library-info"
    else if( "getCameraLibraryInfo" == opID )
    {
        std::string cameraID;

        if( opData->getParam( "cameraid", cameraID ) == true )
        {
            opData->responseSetStatusAndReason( HNR_HTTP_INTERNAL_SERVER_ERROR );
            opData->responseSend();
            return; 
        }

        std::cout << "=== Get Camera Library Info Request (id: " << cameraID << ") ===" << std::endl;

        // Lookup the camera
        //std::shared_ptr< Camera > camPtr = m_cameraMgr.lookupCameraByID( cameraID );

        if( m_hardwareCtrl.hasCameraWithID( cameraID ) == false )
        {
            opData->responseSetStatusAndReason( HNR_HTTP_NOT_FOUND );
            opData->responseSend();
            return; 
        }

        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );
        
        // Get the response json as a string.
        std::string jsonStr = m_hardwareCtrl.getCameraLibraryInfoJSONStr( cameraID );

        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{ 
            ostr << jsonStr;
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }

        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
        return;
    }
    // POST "/hnode2/slide-digitizer/captures"
    else if( "startCapture" == opID )
    {
        action.setType( HNSD_AR_TYPE_START_SINGLE_CAPTURE );

        std::istream& bodyStream = opData->requestBody();
        action.decodeStartCapture( bodyStream );
    }     
    // GET "/hnode2/slide-digitizer/captures"
    else if( "getCaptureList" == opID )
    {
        std::cout << "=== Get Capture List Request ===" << std::endl;
        action.setType( HNSD_AR_TYPE_GET_CAPTURE_LIST );
    }
    // GET "/hnode2/slide-digitizer/captures/{captureid}"
    else if( "getCaptureInfo" == opID )
    {
        std::string captureID;

        if( opData->getParam( "captureid", captureID ) == true )
        {
            opData->responseSetStatusAndReason( HNR_HTTP_INTERNAL_SERVER_ERROR );
            opData->responseSend();
            return; 
        }

        std::cout << "=== Get Capture Info Request (id: " << captureID << ") ===" << std::endl;
        action.setType( HNSD_AR_TYPE_GET_CAPTURE_INFO );
        action.setRequestCaptureID( captureID );
    }      
    // DELETE "/hnode2/slide-digitizer/captures/{captureid}"
    else if( "deleteCapture" == opID )
    {
        std::string captureID;

        // Make sure zoneid was provided
        if( opData->getParam( "captureid", captureID ) == true )
        {
            // captureid parameter is required
            opData->responseSetStatusAndReason( HNR_HTTP_BAD_REQUEST );
            opData->responseSend();
            return; 
        }

        std::cout << "=== Delete Capture Request (id: " << captureID << ") ===" << std::endl;

        action.setType( HNSD_AR_TYPE_DELETE_CAPTURE );
        action.setRequestCaptureID( captureID );
    }
    // GET "/hnode2/slide-digitizer/captures/{captureid}/image"
    else if( "getCaptureImage" == opID )
    {
        std::string captureID;

        if( opData->getParam( "captureid", captureID ) == true )
        {
            opData->responseSetStatusAndReason( HNR_HTTP_INTERNAL_SERVER_ERROR );
            opData->responseSend();
            return; 
        }

        std::cout << "=== Get Capture Image Request (id: " << captureID << ") ===" << std::endl;

        // Stat the image file to get its length
        struct stat statBuf;
        if( stat( "/tmp/tmp.jpg", &statBuf ) != 0 )
        {
            std::cout << "ERROR: Could not open image file" << std::endl;
            opData->responseSetStatusAndReason( HNR_HTTP_INTERNAL_SERVER_ERROR );
            opData->responseSend();
            return; 
        }

        std::cout << "Image File Length: " << statBuf.st_size << std::endl;

        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "image/jpeg" );
        
        // Open a file stream
        std::ifstream jpegIF;

        jpegIF.open( "/tmp/tmp.jpg" );

        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{
            Poco::StreamCopier::copyStream( jpegIF, ostr );
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }

        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );

        jpegIF.close();
        return;
    }    
    else
    {
        // Send back not implemented
        opData->responseSetStatusAndReason( HNR_HTTP_NOT_IMPLEMENTED );
        opData->responseSend();
        return;
    }

    std::cout << "Start Action - client: " << action.getType() << "  thread: " << std::this_thread::get_id() << std::endl;

    // Submit the action and block for response
    m_userActionQueue.postAndWait( &action );

    std::cout << "Finish Action - client" << "  thread: " << std::this_thread::get_id() << std::endl;

    // Determine what happened
    switch( action.getStatus() )
    {
        case HNRW_RESULT_SUCCESS:
        {
            std::string cType;
            std::string objID;

            // See if response content should be generated
            if( action.hasRspContent( cType ) )
            {
                // Set response content type
                opData->responseSetChunkedTransferEncoding( true );
                opData->responseSetContentType( cType );

                // Render any response content
                std::ostream& ostr = opData->responseSend();
            
                if( action.generateRspContent( ostr ) == true )
                {
                    opData->responseSetStatusAndReason( HNR_HTTP_INTERNAL_SERVER_ERROR );
                    opData->responseSend();
                    return;
                }
            }

            // Check if a new object was created.
            if( action.hasNewObject( objID ) )
            {
                // Object was created return info
                opData->responseSetCreated( objID );
                opData->responseSetStatusAndReason( HNR_HTTP_CREATED );
            }
            else
            {
                // Request was successful
                opData->responseSetStatusAndReason( HNR_HTTP_OK );
            }
        }
        break;

        case HNRW_RESULT_FAILURE:
            opData->responseSetStatusAndReason( HNR_HTTP_INTERNAL_SERVER_ERROR );
        break;

        case HNRW_RESULT_TIMEOUT:
            opData->responseSetStatusAndReason( HNR_HTTP_INTERNAL_SERVER_ERROR );
        break;
    }

    // Return to caller
    opData->responseSend();
}

const std::string g_HNode2TestRest = R"(
{
  "openapi": "3.0.0",
  "info": {
    "description": "",
    "version": "1.0.0",
    "title": ""
  },
  "paths": {
      "/hnode2/slide-digitizer/status": {
        "get": {
          "summary": "Get test device status.",
          "operationId": "getStatus",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "array"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      },

      "/hnode2/slide-digitizer/cameras": {
        "get": {
          "summary": "Return list of available camera IDs.",
          "operationId": "getCameraList",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      },

      "/hnode2/slide-digitizer/cameras/{cameraid}": {
        "get": {
          "summary": "Get information about a camera.",
          "operationId": "getCameraInfo",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      },

      "/hnode2/slide-digitizer/cameras/{cameraid}/library-info": {
        "get": {
          "summary": "Get underlying library information about a camera",
          "operationId": "getCameraLibraryInfo",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      },

      "/hnode2/slide-digitizer/captures": {
        "get": {
          "summary": "Return list of active captures.",
          "operationId": "getCaptureList",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        },

        "post": {
          "summary": "Start a new capture",
          "operationId": "startCapture",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      },

      "/hnode2/slide-digitizer/captures/{captureid}": {
        "get": {
          "summary": "Get information about a specific capture.",
          "operationId": "getCaptureInfo",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        },
        "delete": {
          "summary": "Delete a specific capture",
          "operationId": "deleteCapture",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      },

      "/hnode2/slide-digitizer/captures/{captureid}/image": {
        "get": {
          "summary": "Return the captured image.",
          "operationId": "getCaptureImage",
          "responses": {
            "200": {
              "description": "successful operation",
              "content": {
                "application/json": {
                  "schema": {
                    "type": "object"
                  }
                }
              }
            },
            "400": {
              "description": "Invalid status value"
            }
          }
        }
      }
    }
}
)";

