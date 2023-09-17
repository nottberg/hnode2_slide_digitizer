#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include <iostream>
#include <sstream>
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

#include "CameraThread.h"
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

    m_hnodeDev.setDeviceType( HNODE_TEST_DEVTYPE );
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

    // Start accepting device notifications
    m_hnodeDev.setNotifySink( this );

    // Start up the hnode device
    m_hnodeDev.start();

    // Start event processing loop
    m_testDeviceEvLoop.run();

    waitForTerminationRequest();

    return Application::EXIT_OK;
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

    return cfgFile.configExists( HNODE_TEST_DEVTYPE, m_instanceName );
}

HNSDD_RESULT_T
HNSlideDigitizerDevice::initConfig()
{
    HNodeConfigFile cfgFile;
    HNodeConfig     cfg;

    m_hnodeDev.initConfigSections( cfg );

    cfg.debugPrint(2);
    
    std::cout << "Saving config..." << std::endl;
    if( cfgFile.saveConfig( HNODE_TEST_DEVTYPE, m_instanceName, cfg ) != HNC_RESULT_SUCCESS )
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

    if( cfgFile.loadConfig( HNODE_TEST_DEVTYPE, m_instanceName, cfg ) != HNC_RESULT_SUCCESS )
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
    if( cfgFile.saveConfig( HNODE_TEST_DEVTYPE, m_instanceName, cfg ) != HNC_RESULT_SUCCESS )
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
    // std::cout << "HNManagementDevice::loopIteration() - entry" << std::endl;

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
HNSlideDigitizerDevice::dispatchEP( HNodeDevice *parent, HNOperationData *opData )
{
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
    }
    // GET "/hnode2/slide-digitizer/captures"
    else if( "getCaptureList" == opID )
    {
        std::cout << "=== Get Capture List Request ===" << std::endl;

        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );

        // Create a json root object
        pjs::Array jsRoot;

        pjs::Object w1Obj;
        w1Obj.set( "id", "w1" );
        w1Obj.set( "color", "red" );
        jsRoot.add( w1Obj );

        pjs::Object w2Obj;
        w2Obj.set( "id", "w2" );
        w2Obj.set( "color", "green" );
        jsRoot.add( w2Obj );

        pjs::Object w3Obj;
        w3Obj.set( "id", "w3" );
        w3Obj.set( "color", "blue" );
        jsRoot.add( w3Obj );
          
        // Render response content
        std::ostream& ostr = opData->responseSend();
        try{ 
            pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
        } catch( ... ) {
            std::cout << "ERROR: Exception while serializing comment" << std::endl;
        }
            
        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
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

        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );
        
        // Create a json root object
        pjs::Array jsRoot;

        pjs::Object w1Obj;
        w1Obj.set( "id", widgetID );
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

    }
    // POST "/hnode2/slide-digitizer/captures"
    else if( "startCapture" == opID )
    {
        std::istream& rs = opData->requestBody();
        std::string body;
        Poco::StreamCopier::copyToString( rs, body );
        
        std::cout << "=== Start Capture Post Data ===" << std::endl;
        std::cout << body << std::endl;

        CameraThread tstCam;

        tstCam.test();

        // Object was created return info
        opData->responseSetCreated( "w1" );
        opData->responseSetStatusAndReason( HNR_HTTP_CREATED );
    }       
    // DELETE "/hnode2/slide-digitizer/captures/{captureid}"
    else if( "deleteCapture" == opID )
    {
        std::string captureID;

        // Make sure zoneid was provided
        if( opData->getParam( "captureid", captureID ) == true )
        {
            // widgetid parameter is required
            opData->responseSetStatusAndReason( HNR_HTTP_BAD_REQUEST );
            opData->responseSend();
            return; 
        }

        std::cout << "=== Delete Capture Request (id: " << captureID << ") ===" << std::endl;

        // Request was successful
        opData->responseSetStatusAndReason( HNR_HTTP_OK );
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

        // Set response content type
        opData->responseSetChunkedTransferEncoding( true );
        opData->responseSetContentType( "application/json" );
        
        // Create a json root object
        pjs::Array jsRoot;

        pjs::Object w1Obj;
        w1Obj.set( "id", widgetID );
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

    }    
    else
    {
        // Send back not implemented
        opData->responseSetStatusAndReason( HNR_HTTP_NOT_IMPLEMENTED );
        opData->responseSend();
        return;
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

