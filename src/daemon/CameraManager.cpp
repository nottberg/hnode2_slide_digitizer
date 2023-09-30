#include <iostream>
#include <sstream>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>

#include "CameraManager.h"

namespace pjs = Poco::JSON;


CameraControl::CameraControl()
{

}

CameraControl::~CameraControl()
{

}

CameraFormat::CameraFormat()
{

}

CameraFormat::~CameraFormat()
{

}

CameraCapture::CameraCapture( std::string id )
{
    m_id = id;
}

CameraCapture::~CameraCapture()
{

}

Camera::Camera( CameraManager *parent, std::string id )
{
    m_id = id;
    m_parent = parent;

    m_captureMode = CS_STILLMODE_DEFAULT;
}

Camera::~Camera()
{

}

void
Camera::printInfo()
{
    std::cout << "=== Camera: " << m_modelName << "  (id: " << m_libID << ")" << std::endl;
}

CM_RESULT_T
Camera::setLibraryObject( std::shared_ptr< libcamera::Camera > objPtr )
{
    // Save away the library pointer
    m_camPtr = objPtr;

    if( m_camPtr == nullptr )
        return CM_RESULT_SUCCESS;

    // Get the unique id for this camera.
    m_libID = m_camPtr->id();

    // Get a few well known camera properties info
    const libcamera::ControlList &cl = m_camPtr->properties();

    auto model = cl.get( libcamera::properties::Model );
    m_modelName = model ? *model : "";

	std::optional< libcamera::Span< const libcamera::Rectangle > > sensorSize = cl.get( libcamera::properties::PixelArrayActiveAreas );
	if( sensorSize ) // && ( sensorSize.type() == libcamera::ControlType::ControlTypeRectangle ) )
    {
        m_arrayHeight = (*sensorSize)[0].height;
        m_arrayWidth = (*sensorSize)[0].width;

        std::cout << "SensorSize: " << m_arrayWidth << "x" << m_arrayHeight << std::endl;
    }

    return CM_RESULT_SUCCESS;
}

CM_RESULT_T
Camera::setStillCaptureMode( CS_STILLMODE_T mode, uint width, uint height )
{
    m_captureMode   = mode;
    m_captureWidth  = width;
    m_captureHeight = height;

    return CM_RESULT_SUCCESS;
}

std::string
Camera::getID()
{
    return m_id;
}

std::string
Camera::getLibraryInfoJSONStr()
{
    // Create a json root object
    pjs::Object jsRoot;
    pjs::Object jsCamera;
    pjs::Array jsProperties;
    pjs::Array jsControls;
    pjs::Array jsStreams;

    if( m_camPtr == nullptr )
        return "";

    jsRoot.set( "library", "libcamera" );
    jsRoot.set( "library-version", m_parent->getCameraLibraryVersion() );

    jsCamera.set( "id", m_id );
    jsCamera.set( "libID", m_libID );
    jsCamera.set( "model", m_modelName );

	// Disable any libcamera logging for this bit.
	//logSetTarget(LoggingTargetNone);

    // Aquire the camera while we interrogate it
	m_camPtr->acquire();

    // Get camera properties info
    const libcamera::ControlList &cl = m_camPtr->properties();

    const libcamera::ControlIdMap *cidMap = cl.idMap();

    for( libcamera::ControlList::const_iterator it = cl.begin(); it != cl.end(); it++ )
    {
        pjs::Object jsProperty;
        jsProperty.set( "id", it->first );
        jsProperty.set( "name", cidMap->at(it->first)->name() );
        jsProperty.set( "descriptive-value", it->second.toString() );

        jsProperties.add( jsProperty );
    }
    jsCamera.set( "properties", jsProperties );

    // Get camera controls info
    const libcamera::ControlInfoMap &cim = m_camPtr->controls();

    for( libcamera::ControlInfoMap::const_iterator it = cim.begin(); it != cim.end(); it++ )
    {
        pjs::Object jsControl;
        jsControl.set( "id", it->first->id() );
        jsControl.set( "type", (uint) it->first->type() );
        jsControl.set( "name", it->first->name() );
        jsControl.set( "descriptive-value", it->second.toString() );

        jsControls.add( jsControl );
    }
    jsCamera.set( "controls", jsControls );

    // Get Raw stream info
    pjs::Object jsRawStream;
    pjs::Array jsRSFormats;

    jsRawStream.set( "type", (uint) libcamera::StreamRole::Raw );
    jsRawStream.set( "typeStr", "Raw" );

	std::unique_ptr< libcamera::CameraConfiguration > rconfig = m_camPtr->generateConfiguration( {libcamera::StreamRole::Raw} );
	
    if( rconfig )
    {
	    const libcamera::StreamFormats &formats = rconfig->at( 0 ).formats();

	    if( formats.pixelformats().size() )
        {
	        for( const auto &pix : formats.pixelformats() )
	        {
                pjs::Object jsFormat;

                jsFormat.set( "mode", pix.toString() );

                pjs::Array jsFrameSizes;

		        unsigned int num = formats.sizes( pix ).size();
		        for( const auto &size : formats.sizes( pix ) )
		        {
                    pjs::Object jsFrameSize;
                    jsFrameSize.set( "width", size.width );
                    jsFrameSize.set( "height", size.height );
                    jsFrameSizes.add( jsFrameSize );
                }
                jsFormat.set( "frameSizes", jsFrameSizes );

                jsRSFormats.add( jsFormat );
            }
        }

    }

    jsRawStream.set( "formats", jsRSFormats );

    jsStreams.add( jsRawStream );

    // Get Raw stream info
    pjs::Object jsStillStream;
    pjs::Array jsStillFormats;

    jsStillStream.set( "type", (uint) libcamera::StreamRole::StillCapture );
    jsStillStream.set( "typeStr", "StillCapture" );

	std::unique_ptr< libcamera::CameraConfiguration > sconfig = m_camPtr->generateConfiguration( {libcamera::StreamRole::StillCapture} );
	
    if( sconfig )
    {
	    const libcamera::StreamFormats &formats = sconfig->at( 0 ).formats();

	    if( formats.pixelformats().size() )
        {
	        for( const auto &pix : formats.pixelformats() )
	        {
                pjs::Object jsFormat;

                jsFormat.set( "mode", pix.toString() );

                pjs::Array jsFrameSizes;

		        unsigned int num = formats.sizes( pix ).size();
		        for( const auto &size : formats.sizes( pix ) )
		        {
                    pjs::Object jsFrameSize;
                    jsFrameSize.set( "width", size.width );
                    jsFrameSize.set( "height", size.height );
                    jsFrameSizes.add( jsFrameSize );
                }
                jsFormat.set( "frameSizes", jsFrameSizes );

                jsStillFormats.add( jsFormat );
            }
        }

    }

    jsStillStream.set( "formats", jsStillFormats );

    jsStreams.add( jsStillStream );

    jsCamera.set( "streams", jsStreams );

    // Add the camera object to the returned data
    jsRoot.set( "camera", jsCamera );

    // Done interrogating camera
	m_camPtr->release();

    // Render response content
    std::stringstream ostr;
    try{ 
        pjs::Stringifier::stringify( jsRoot, ostr, 1 ); 
    } catch( const std::exception &err ) {
        std::cerr << "ERROR: Exception while serializing library info: " << err.what() << std::endl;
        return "";
    } 

    return ostr.str();
}

CameraManager::CameraManager()
{

}

CameraManager::~CameraManager()
{

}

CM_RESULT_T
CameraManager::start()
{
    // Open Camera
  	std::cout << "Opening camera..." << std::endl;

	m_camMgr = std::make_unique< libcamera::CameraManager >();

    std::cout << "libcamera version: " << m_camMgr->version() << std::endl;

	int ret = m_camMgr->start();
	if( ret )
    {
		std::cerr << "camera manager failed to start, code " + std::to_string(-ret) << std::endl;
        return CM_RESULT_FAILURE;
    }

    return CM_RESULT_SUCCESS;
}

CM_RESULT_T
CameraManager::stop()
{
    // Reset the camera manager object
	m_camMgr.reset();

    return CM_RESULT_SUCCESS;
}

CM_RESULT_T
CameraManager::initCameraList()
{
    char tmpID[64];

    // Cleanup and previous camera list
    cleanupCameraList();

    // Call the library to get a current list of cameras
	std::vector< std::shared_ptr< libcamera::Camera> > cameras = m_camMgr->cameras();

	// Do not show USB webcams as these are not supported in libcamera-apps!
	auto rem = std::remove_if( cameras.begin(), cameras.end(), [](auto &cam) { return cam->id().find("/usb") != std::string::npos; } );
	cameras.erase( rem, cameras.end() );
	std::sort( cameras.begin(), cameras.end(), [](auto l, auto r) { return l->id() > r->id(); } );

    // Build the local list of available cameras
    uint camIndx = 0;
    for( std::vector< std::shared_ptr< libcamera::Camera > >::iterator it = cameras.begin(); it != cameras.end(); it++ )
    {
        sprintf( tmpID, "cam%u", camIndx );

        std::cout << "Adding camera - id: " << tmpID << std::endl;

        std::shared_ptr< Camera > camPtr = std::make_shared< Camera >( this, tmpID );
        
        camPtr->setLibraryObject( *it );

        m_camMap.insert( std::pair< std::string, std::shared_ptr< Camera > >( tmpID, camPtr ) );

        camIndx += 1;
    }

    return CM_RESULT_SUCCESS;
}

CM_RESULT_T
CameraManager::cleanupCameraList()
{
    m_camMap.clear();

    return CM_RESULT_SUCCESS;
}

std::string
CameraManager::getCameraLibraryVersion()
{
    return ( m_camMgr ? m_camMgr->version() : "" );
}

void
CameraManager::getCameraIDList( std::vector< std::string > &idList )
{
    idList.clear();

    for( std::map< std::string, std::shared_ptr< Camera > >::iterator it = m_camMap.begin(); it != m_camMap.end(); it++ )
        idList.push_back( it->first );
}

std::shared_ptr< Camera >
CameraManager::lookupCameraByID( std::string id )
{
    // Try to lookup the bamera by id
    std::map< std::string, std::shared_ptr< Camera > >::iterator it = m_camMap.find( id );

    // If not found, return null
    if( it == m_camMap.end() )
        return nullptr;

    return it->second;
}
