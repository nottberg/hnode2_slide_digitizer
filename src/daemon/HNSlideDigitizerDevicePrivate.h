#ifndef __HN_SLIDE_DIGITIZER_DEVICE_PRIVATE_H__
#define __HN_SLIDE_DIGITIZER_DEVICE_PRIVATE_H__

#include <string>
#include <vector>

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/OptionSet.h"

#include <hnode2/HNodeDevice.h>
#include <hnode2/HNodeConfig.h>
#include <hnode2/HNEPLoop.h>
#include <hnode2/HNReqWaitQueue.h>

#include "CameraManager.h"
#include "HNSDAction.h"
#include "HNSDHardwareControl.h"

#define HNODE_SLIDE_DIGITIZER_DEVTYPE   "hnode2-slide-digitizer-device"

typedef enum HNSlideDigitizerDeviceResultEnum
{
  HNSDD_RESULT_SUCCESS,
  HNSDD_RESULT_FAILURE,
  HNSDD_RESULT_BAD_REQUEST,
  HNSDD_RESULT_SERVER_ERROR
}HNSDD_RESULT_T;

typedef enum HNSlideDigitizerDeviceStateEnum
{
  HNSD_DEVSTATE_NOTSET,
  HNSD_DEVSTATE_INIT,
  HNSD_DEVSTATE_IDLE,
  HNSD_DEVSTATE_CAPTURING,
  HNSD_DEVSTATE_IMGPROC,
  HNSD_DEVSTATE_CAROSEL_CAPTURING,
  HNSD_DEVSTATE_CAROSEL_IMGPROC,
  HNSD_DEVSTATE_CAROSEL_ADVANCING
}HNSD_DEVSTATE_T;

typedef enum HNIDStartActionBitsEnum
{
    HNID_ACTBIT_CLEAR     = 0x0000,
    HNID_ACTBIT_COMPLETE  = 0x0001,
    HNID_ACTBIT_UPDATE    = 0x0002,
//    HNID_ACTBIT_RECALCSCH = 0x0004,
    HNID_ACTBIT_ERROR     = 0x0008,
//    HNID_ACTBIT_SENDREQ   = 0x0010
} HNID_ACTBIT_T;

class HNSlideDigitizerDevice : public Poco::Util::ServerApplication, public HNDEPDispatchInf, public HNDEventNotifyInf, public HNEPLoopCallbacks 
{
    protected:
        // HNDevice REST callback
        virtual void dispatchEP( HNodeDevice *parent, HNOperationData *opData );

        // Notification for hnode device config changes.
        virtual void hndnConfigChange( HNodeDevice *parent );

        // Event loop functions
        virtual void loopIteration();
        virtual void timeoutEvent();
        virtual void fdEvent( int sfd );
        virtual void fdError( int sfd );

        // Current device state
        HNSD_DEVSTATE_T getState();

        // Poco funcions
        void defineOptions( Poco::Util::OptionSet& options );
        void handleOption( const std::string& name, const std::string& value );
        int main( const std::vector<std::string>& args );

    private:
        bool _helpRequested   = false;
        bool _debugLogging    = false;
        bool _instancePresent = false;

        std::string _instance; 
        std::string m_instanceName;

        HNodeDevice m_hnodeDev;

        HNEPTrigger m_configUpdateTrigger;

        HNEPLoop m_testDeviceEvLoop;

        CameraManager m_cameraMgr;

        HNSD_DEVSTATE_T  m_devState;

        HNSDHardwareControl m_hardwareCtrl;
        HNEPTrigger         m_hardwareNotifyTrigger;

        HNSDAction     *m_curUserAction;
        HNReqWaitQueue  m_userActionQueue;

        uint m_nextCaptureID;
        std::map< std::string, CaptureRequest* > m_captureMap;

        // Format string codes
        uint m_errStrCode;
        uint m_noteStrCode;

        // Health component ids
        //std::string m_hc1ID;
        //std::string m_hc2ID;
        //std::string m_hc3ID;

        // Keep track of health state change simulation
        //uint m_healthStateSeq;

        void displayHelp();

        bool configExists();
        HNSDD_RESULT_T initConfig();
        HNSDD_RESULT_T readConfig();
        HNSDD_RESULT_T updateConfig();

        void startAction();

        //void generateNewHealthState();
};

#endif // __HN_TEST_DEVICE_PRIVATE_H__
