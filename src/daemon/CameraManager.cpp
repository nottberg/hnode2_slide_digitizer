#include <iostream>

#include "CameraManager.h"

Camera::Camera( CameraManager *parent, std::string id )
{

}

Camera::~Camera()
{

}

void
Camera::printInfo()
{
    std::cout << "=== Camera: " << m_modelName << "  (id: " << m_id << ")" << std::endl;
}

CM_RESULT_T
Camera::setLibraryObject( std::shared_ptr< libcamera::Camera > objPtr )
{
    // Save away the library pointer
    m_camPtr = objPtr;

    if( m_camPtr == nullptr )
        return CM_RESULT_SUCCESS;

    // Get the unique id for this camera.
    m_id = m_camPtr->id();

    // Get a few well known camera properties info
    const libcamera::ControlList &cl = m_camPtr->properties();

    auto model = cl.get( libcamera::properties::Model );
    m_modelName = model ? *model : "";

	auto sensorSize = cl.get( libcamera::properties::PixelArrayActiveAreas );
	if( sensorSize && ( sensorSize.type() == libcamera::ControlType::ControlTypeRectangle ) )
    {
        std::cout << "SensorSize: " << sensorSize.toString() << std::endl;

        libcamera::Size size;
        //size = sensorSize
        //m_arrayWidth = sensorSize.
    }

#if 0
    const libcamera::ControlIdMap *cidMap = cl.idMap();

    for( libcamera::ControlList::const_iterator it = cl.begin(); it != cl.end(); it++ )
    {
        std::cout << "Property - name: " << cidMap->at(it->first)->name() << "  val: " << it->second.toString() << std::endl;
    }

    // Get controls info
    const libcamera::ControlInfoMap &cim = m_camPtr->controls();

    for( libcamera::ControlInfoMap::const_iterator it = cim.begin(); it != cim.end(); it++ )
    {
        std::cout << "Control - name: " << it->first->name() << "  val: " << it->second.toString() << std::endl;
    }

	// Disable any libcamera logging for this bit.
	//logSetTarget(LoggingTargetNone);
	m_camPtr->acquire();

	std::unique_ptr< libcamera::CameraConfiguration > config = m_camPtr->generateConfiguration( {libcamera::StreamRole::StillCapture} );
	
    if( !config )
    {
        std::cerr << "Could not generate camera configuration" << std::endl;
        return CM_RESULT_FAILURE;
    }

	const libcamera::StreamFormats &formats = config->at( 0 ).formats();

	if( !formats.pixelformats().size() )
    {
        std::cerr << "No camera formats" << std::endl;
        return CM_RESULT_FAILURE;
    }

//	auto cfa = m_camPtr->properties().get( properties::draft::ColorFilterArrangement );
//	if( cfa && cfa_map.count( *cfa ) )
//		sensor_props << cfa_map.at( *cfa ) << " ";

//		sensor_props.seekp(-1, sensor_props.cur);
//		sensor_props << "] (" << cam->id() << ")";
//		std::cout << sensor_props.str() << std::endl;

//		ControlInfoMap control_map;
//		Size max_size;
		//PixelFormat max_fmt;

	std::cout << "    Modes: ";
	unsigned int i = 0;
	for( const auto &pix : formats.pixelformats() )
	{
		if( i++ ) std::cout << "           ";
		std::string mode( "'" + pix.toString() + "' : " );
		std::cout << mode;

		unsigned int num = formats.sizes( pix ).size();
		for( const auto &size : formats.sizes( pix ) )
		{
		    std::cout << size.toString() << " ";

#if 0
				config->at(0).size = size;
				config->at(0).pixelFormat = pix;
				config->validate();

				m_camPtr->configure( config.get() );

				if( size > max_size )
				{
					control_map = cam->controls();
					max_fmt = pix;
					max_size = size;
				}

				auto fd_ctrl = cam->controls().find(&controls::FrameDurationLimits);
				auto crop_ctrl = cam->properties().get(properties::ScalerCropMaximum);
				double fps = fd_ctrl == cam->controls().end() ? NAN : (1e6 / fd_ctrl->second.min().get<int64_t>());
				std::cout << std::fixed << std::setprecision(2) << "[" << fps << " fps - " << crop_ctrl->toString() << " crop" << "]";
				if( --num )
				{
					std::cout << std::endl;
					for (std::size_t s = 0; s < mode.length() + 11; std::cout << " ", s++);
				}
#endif
		}
		std::cout << std::endl;
	}

	std::cout << std::endl;
	m_camPtr->release();
#endif

    return CM_RESULT_SUCCESS;
}

std::string
Camera::getID()
{
    return ( m_camPtr ? m_camPtr->id() : "" );
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
    // Cleanup and previous camera list
    cleanupCameraList();

    // Call the library to get a current list of cameras
	std::vector< std::shared_ptr< libcamera::Camera> > cameras = m_camMgr->cameras();

	// Do not show USB webcams as these are not supported in libcamera-apps!
	auto rem = std::remove_if( cameras.begin(), cameras.end(), [](auto &cam) { return cam->id().find("/usb") != std::string::npos; } );
	cameras.erase( rem, cameras.end() );
	std::sort( cameras.begin(), cameras.end(), [](auto l, auto r) { return l->id() > r->id(); } );

    // Build the local list of available cameras
    for( std::vector< std::shared_ptr< libcamera::Camera > >::iterator it = cameras.begin(); it != cameras.end(); it++ )
    {
    	std::string const &camID = it->get()->id();

        std::cout << "Adding camera - id: " << camID << std::endl;

        std::shared_ptr< Camera > camPtr = std::make_shared< Camera >( this, camID );
        
        camPtr->setLibraryObject( *it );

        m_camList.push_back( camPtr );
    }

    return CM_RESULT_SUCCESS;
}

CM_RESULT_T
CameraManager::cleanupCameraList()
{
    m_camList.clear();

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

    for( std::vector< std::shared_ptr< Camera > >::iterator it = m_camList.begin(); it != m_camList.end(); it++ )
        idList.push_back( it->get()->getID() );
}
