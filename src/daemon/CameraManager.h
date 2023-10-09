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

class CaptureRequest
{
    public:
        CaptureRequest();
       ~CaptureRequest();

        void setImageFormat( CS_STILLMODE_T mode, uint width, uint height );

        std::string getPlatformName();
        std::string getCameraModel();

        CS_STILLMODE_T getRawFormat();

        size_t getStreamWidth();
	    size_t getStreamHeight();
        size_t getStreamStride();

        uint8_t *getImageBufPtr();

        size_t getThumbnailWidth();
	    size_t getThumbnailHeight();

        size_t getOutputWidth();
	    size_t getOutputHeight();

        uint getOutputQuality();
        
        std::string getRawFilename();

        friend class Camera;

    protected:

        CM_RESULT_T initAfterConfigSet();
        CM_RESULT_T initAfterRequestSet();

        CS_STILLMODE_T m_mode;

        uint m_width;
        uint m_height;

        std::shared_ptr< libcamera::CameraConfiguration > m_cfgObj;

        std::shared_ptr< libcamera::Request > m_reqObj;

        uint8_t *m_imgBufPtr;
        size_t   m_imgBufLength;
};

typedef enum CameraEventTypeEnum
{
    CR_EVTYPE_REQ_CANCELED,
    CR_EVTYPE_REQ_FOCUSING,
    CR_EVTYPE_REQ_COMPLETE
}CR_EVTYPE_T;

class CameraEventInf
{
    public:
        virtual void requestEvent( CR_EVTYPE_T event ) = 0;
};

class Camera
{
    public:
        Camera( CameraManager *parent, std::string id );
       ~Camera();

        CM_RESULT_T initFromLibrary();

        //CM_RESULT_T setStillCaptureMode( CS_STILLMODE_T mode, uint width, uint height );

        CM_RESULT_T acquire( CaptureRequest *request, CameraEventInf *callback );

        CM_RESULT_T configureForCapture();

        CM_RESULT_T start();

        CM_RESULT_T queueAutofocusRequest();

        CM_RESULT_T queueRequest();

        CM_RESULT_T stop();

        CM_RESULT_T release();

        std::string getID();

        std::string getLibraryInfoJSONStr();

        void printInfo();

        friend class CameraManager;

    protected:

        // Request complete callback
        void requestComplete( libcamera::Request *request );

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
        //CS_STILLMODE_T m_captureMode;
        //uint m_captureWidth;
        //uint m_captureHeight;

        // Event callback interface
        CameraEventInf *m_eventCB;

        // Current Capture Request
        CaptureRequest *m_capReq;

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

};

#endif //__CAMERA_MANAGER_H__
