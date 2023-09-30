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

#include <hnode2/HNReqWaitQueue.h>

// Forward declaration
class CameraManager;

typedef enum CameraManagerResultEnum
{
    CM_RESULT_SUCCESS,
    CM_RESULT_FAILURE
} CM_RESULT_T;

class CameraFormat
{
    public:
        CameraFormat();
       ~CameraFormat();

    private:

        // The format id string.
        std::string m_id;

        // A list of supported frame sizes.
        std::vector< std::pair< uint, uint > > m_frameSizes;
};

class CameraControl
{
    public:
        CameraControl();
       ~CameraControl();

    private:

        uint        m_id;
        std::string m_name;

        std::string m_possibleValStr;
};

typedef enum CameraSupportedStillCaptureModeEnum
{
    CS_STILLMODE_NOTSET,
    CS_STILLMODE_DEFAULT,
    CS_STILLMODE_YUV420,
    CS_STILLMODE_YUYV
} CS_STILLMODE_T;

class Camera
{
    public:
        Camera( CameraManager *parent, std::string id );
       ~Camera();

        CM_RESULT_T setLibraryObject( std::shared_ptr< libcamera::Camera > objPtr );

        CM_RESULT_T setStillCaptureMode( CS_STILLMODE_T mode, uint width, uint height );

        std::string getID();

        std::string getLibraryInfoJSONStr();

        void printInfo();

    private:

        // The Camera Manager parent
        CameraManager *m_parent;

        // The unique id
        std::string m_id;

        // Pointer to the libcamera library object
        std::shared_ptr< libcamera::Camera > m_camPtr;

        // The library camera id
        std::string m_libID;

        // Camera properties
        std::string m_modelName;

        uint m_arrayWidth;
        uint m_arrayHeight;

        // Selected mode info
        CS_STILLMODE_T m_captureMode;
        uint m_captureWidth;
        uint m_captureHeight;

        // Array of possible camera controls
        std::vector< CameraControl > m_controlList;

        // Array of camera formats
        std::vector< CameraFormat > m_formatList;
};

typedef enum CameraCaptureStateEnum
{
    CCSTATE_NOTSET,
    CCSTATE_CAPTURE_RUNNING,
    CCSTATE_CAPTURE_COMPLETE
} CCSTATE_T;

class CameraCapture : public HNReqWaitAction
{
    public:
        CameraCapture( std::string id );
       ~CameraCapture();

        CM_RESULT_T startCapture( std::shared_ptr< Camera > camera );

    private:

        // The capture id
        std::string m_id;

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

        std::string getCameraLibraryVersion();

        void getCameraIDList( std::vector< std::string > &idList );

        std::shared_ptr< Camera > lookupCameraByID( std::string id );

    private:

        std::unique_ptr< libcamera::CameraManager > m_camMgr;

        std::map< std::string, std::shared_ptr< Camera > > m_camMap;

        std::map< std::string, std::shared_ptr< CameraCapture > > m_captureMap;

};

#endif //__CAMERA_MANAGER_H__
