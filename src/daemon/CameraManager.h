#ifndef __CAMERA_MANAGER_H__
#define __CAMERA_MANAGER_H__

#include <any>
#include <map>
#include <mutex>
#include <string>
#include <memory>
#include <optional>

//#include <libcamera/base/span.h>
#include <libcamera/camera_manager.h>
#include <libcamera/camera.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/request.h>
#include <libcamera/property_ids.h>

// Forward declaration
class CameraManager;

typedef enum CameraManagerResultEnum
{
    CM_RESULT_SUCCESS,
    CM_RESULT_FAILURE
} CM_RESULT_T;

class CameraCapabilities
{
    public:
        CameraCapabilities();
       ~CameraCapabilities();

    private:

};

class Camera
{
    public:
        Camera( CameraManager *parent, std::string id );
       ~Camera();

        void setLibraryObject( std::shared_ptr< libcamera::Camera > objPtr );

        std::string getID();

    private:

        std::shared_ptr< libcamera::Camera > m_camPtr;

};

class CameraManager
{
    public:
         CameraManager();
        ~CameraManager();

        CM_RESULT_T start();
        CM_RESULT_T stop();

        CM_RESULT_T initCameraList();
        CM_RESULT_T cleanupCameraList();

        void getCameraIDList( std::vector< std::string > &idList );

    private:

        std::unique_ptr< libcamera::CameraManager > m_camMgr;

        std::vector< std::shared_ptr< Camera > > m_camList;
};

#endif //__CAMERA_MANAGER_H__
