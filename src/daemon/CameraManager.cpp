#include <iostream>

#include "CameraManager.h"

Camera::Camera( CameraManager *parent, std::string id )
{

}

Camera::~Camera()
{

}

void
Camera::setLibraryObject( std::shared_ptr< libcamera::Camera > objPtr )
{
    m_camPtr = objPtr;
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

void
CameraManager::getCameraIDList( std::vector< std::string > &idList )
{
    idList.clear();

    for( std::vector< std::shared_ptr< Camera > >::iterator it = m_camList.begin(); it != m_camList.end(); it++ )
        idList.push_back( it->get()->getID() );
}
