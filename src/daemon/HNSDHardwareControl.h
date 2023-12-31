#ifndef __HNSD_HARDWARE_CONTROL_H__
#define __HNSD_HARDWARE_CONTROL_H__

#include <stdint.h>

#include <string>
#include <mutex>
#include <thread>
#include <sstream>

#include <hnode2/HNEPLoop.h>

#include "GPIOManager.h"
#include "CameraManager.h"
#include "HNSDPipeline.h"

typedef enum HNSDHardwareControlResult
{
    HNSD_HC_RESULT_SUCCESS,
    HNSD_HC_RESULT_FAILURE,
    HNSD_HC_RESULT_BUSY
}HNSD_HC_RESULT_T;

typedef enum HNSDHardwareStateEnum
{
    HNSD_HWSTATE_NOTSET = 0,
    HNSD_HWSTATE_INIT = 1,
    HNSD_HWSTATE_IDLE = 2,
    HNSD_HWSTATE_CAPTURE_START = 3,
    HNSD_HWSTATE_CAPTURE_FOCUS_WAIT = 4,
    HNSD_HWSTATE_CAPTURE_FOCUS_FAILURE = 5,
    HNSD_HWSTATE_CAPTURE_POLL_REQUEST = 6,
    HNSD_HWSTATE_CAPTURE_WAIT_REQ = 7,
    HNSD_HWSTATE_CAPTURE_RAW_IMAGE_AQUIRED = 8,
    HNSD_HWSTATE_CAPTURE_POST_PROCESS = 9,
    HNSD_HWSTATE_CAPTURE_COMPLETE = 10,
    HNSD_HWSTATE_CAROSEL_CAPTURING = 11,
    HNSD_HWSTATE_CAROSEL_IMGPROC = 12,
    HNSD_HWSTATE_CAROSEL_ADVANCING = 13,
    HNSD_HWSTATE_OPERATION_START = 14,
    HNSD_HWSTATE_OPERATION_COMPLETE = 15,
    HNSD_HWSTATE_OPERATION_FAILURE = 16
}HNSD_HWSTATE_T;

typedef enum HNSDHardwareOperationTypeEnum
{
    HNHW_OPTYPE_NOTSET,
    HNHW_OPTYPE_SINGLE_CAPTURE,
    HNHW_OPTYPE_MOVE
}HNHW_OPTYPE_T;

typedef enum HNSDHardwareMovementDirectionEnum
{
    HNHW_MDIR_FORWARD,
    HNHW_MDIR_BACKWARD
}HNHW_MDIR_T;

typedef enum HNSDHardwareOperationStateEnum
{
    HNHW_OPSTATE_PENDING,
    HNHW_OPSTATE_ACTIVE,
    HNHW_OPSTATE_COMPLETE,
    HNHW_OPSTATE_ERROR
}HNHW_OPSTATE_T;

class HNSDHardwareOperation
{
    public:
        HNSDHardwareOperation( std::string id, HNHW_OPTYPE_T type );
       ~HNSDHardwareOperation();

        std::string getID();

        HNHW_OPTYPE_T getType();

        void setState( HNHW_OPSTATE_T newState );

        HNHW_OPSTATE_T getState();

        CaptureRequest* getCaptureRequestPtr();

        void setMoveParameters( HNHW_MDIR_T direction, uint stepCnt );
        HNHW_MDIR_T getMoveDirection();
        uint getMoveCyclesRequired();
        void recordMoveCycleComplete();

    private:
        // The object id.
        std::string m_id;

        // The type of operation
        HNHW_OPTYPE_T m_opType;

        // The current state of the operation
        HNHW_OPSTATE_T m_opState;

        // If the operation involves a capture,
        // this is the config object for that.
        CaptureRequest m_captureRequest;

        // Carousel Movement Tracking
        // Desired direction of movement
        HNHW_MDIR_T m_moveDir;

        // Target number of movement steps
    	uint m_moveStepCnt;

        // Track the number of completed steps
        uint m_moveCompletedStepCnt;
};

class HNSDHardwareControl : public CameraEventInf
{
    public:
        HNSDHardwareControl();
       ~HNSDHardwareControl();

        void init( HNEPTrigger *notifyTrigger );

        HNSD_HWSTATE_T getOperationState();

        void getCameraIDList( std::vector< std::string > &idList );

        bool hasCameraWithID( std::string cameraID );
        
        std::string getCameraLibraryInfoJSONStr( std::string cameraID );

        HNSD_HWSTATE_T getState();

        HNSD_HC_RESULT_T startOperation( HNSDHardwareOperation *opObj );

        void cancelOperation();

        void finishOperation();

        virtual void requestEvent( CR_EVTYPE_T event );

    private:
        CameraManager m_cameraMgr;

        HNEPTrigger *m_notifyTrigger;

        std::thread *m_opThread;

        HNSDHardwareOperation *m_activeOp;

        std::mutex  m_opStateMutex;
        HNSD_HWSTATE_T m_opState;

        GPIOManager m_gpioMgr;

        void updateOperationState( HNSD_HWSTATE_T newState );

        static void captureThread( HNSDHardwareControl *ctrl );
        void runCapture();

        static void carouselMoveThread( HNSDHardwareControl *ctrl );
        void runCarouselMove();

        //void requestComplete( libcamera::Request *request );
};

class HNSDPSHardwareSingleCapture : public HNSDPipelineStepBase
{
    public:
        HNSDPSHardwareSingleCapture( std::string instance, std::string ownerID );
       ~HNSDPSHardwareSingleCapture();

    private:

        virtual HNSD_PSTEP_TYPE_T getType();

        virtual void initSupportedParameters( HNSDPipelineParameterMap *paramMap );

        virtual bool doesStepApply( HNSDPipelineParameterMap *paramMap );

        virtual HNSDP_RESULT_T completeStep( HNSDPipelineParameterMap *params, HNSDStorageManager *fileMgr );

        virtual HNSDP_RESULT_T createHardwareOperation( HNSDPipelineParameterMap *params, HNSDStorageManager *fileMgr, HNSDHardwareOperation **rtnPtr );

        virtual HNSDP_RESULT_T completedHardwareOperation( HNSDPipelineParameterMap *params, HNSDStorageManager *fileMgr, HNSDHardwareOperation **rtnPtr );
};

class HNSDPSHardwareMove : public HNSDPipelineStepBase
{
    public:
        HNSDPSHardwareMove( std::string instance, std::string ownerID );
       ~HNSDPSHardwareMove();

    private:

        virtual HNSD_PSTEP_TYPE_T getType();

        virtual void initSupportedParameters( HNSDPipelineParameterMap *paramMap );

        virtual bool doesStepApply( HNSDPipelineParameterMap *paramMap );

        virtual HNSDP_RESULT_T completeStep( HNSDPipelineParameterMap *params, HNSDStorageManager *fileMgr );

        virtual HNSDP_RESULT_T createHardwareOperation( HNSDPipelineParameterMap *params, HNSDStorageManager *fileMgr, HNSDHardwareOperation **rtnPtr );

        virtual HNSDP_RESULT_T completedHardwareOperation( HNSDPipelineParameterMap *params, HNSDStorageManager *fileMgr, HNSDHardwareOperation **rtnPtr );
};

#endif // __HNSD_HARDWARE_CONTROL_H__
