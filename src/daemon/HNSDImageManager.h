#ifndef __HNSD_IMAGE_MANAGER_H__
#define __HNSD_IMAGE_MANAGER_H__

#include <any>
#include <map>
#include <mutex>
#include <string>
#include <memory>

#include <hnode2/HNReqWaitQueue.h>

typedef enum HNSDImageManagerResultEnum
{
    IMM_RESULT_SUCCESS,
    IMM_RESULT_FAILURE
}IMM_RESULT_T;

typedef enum HNSDCaptureExecutionStateEnum
{
    HNSDCAP_EXEC_STATE_NOTSET,
    HNSDCAP_EXEC_STATE_PENDING,
    HNSDCAP_EXEC_STATE_CAPTURE,
    HNSDCAP_EXEC_STATE_CAPTURE_WAIT,
    HNSDCAP_EXEC_STATE_MOVE,
    HNSDCAP_EXEC_STATE_MOVE_WAIT,
    HNSDCAP_EXEC_STATE_IMAGE_PROCESS,
    HNSDCAP_EXEC_STATE_IMAGE_PROCESS_WAIT,
    HNSDCAP_EXEC_STATE_COMPLETE
}HNSDCAP_EXEC_STATE_T;

typedef enum HNSDCaptureActionEnum
{
    HNSDCAP_ACTION_WAIT,
    HNSDCAP_ACTION_START_CAPTURE,
    HNSDCAP_ACTION_START_ADVANCE,
    HNSDCAP_ACTION_COMPLETE
}HNSDCAP_ACTION_T;

typedef enum HNSDCaptureFileType
{
    HNSDCAP_FT_NOTSET,
    HNSDCAP_FT_JPEG
}HNSDCAP_FT_T;

class HNSDCaptureFile
{
    public:
        HNSDCaptureFile();
       ~HNSDCaptureFile();

        void setType( HNSDCAP_FT_T type );
        void setPath( std::string path );
        void setIndex( uint index );
        void setPurpose( std::string purpose );
        void setTimestampStr( std::string tsStr );

        std::string getPathAndFile();

    private:
        HNSDCAP_FT_T m_type;

        std::string m_path;

        uint m_indexNum;

        std::string m_purpose;

        std::string m_timestampStr;

};

class HNSDCaptureInfoInterface
{
    public:
        virtual std::string getStorageRootPath() = 0;
};

class HNSDCaptureRecord
{
    public:
        HNSDCaptureRecord( HNSDCaptureInfoInterface *infoIntf );
       ~HNSDCaptureRecord();

        void generateNewID( uint64_t timestamp, uint orderIndex );

        void setID( std::string id );
        void setOrderIndex( uint index );

        std::string getID();
        uint getOrderIndex();

        std::string registerNextFilename( std::string purpose );
        
        HNSDCAP_ACTION_T checkNextStep();

        void makeActive();

        void startedAction();

        void completedAction();

    private:
        HNSDCaptureInfoInterface *m_infoIntf;

        std::string  m_id;

        uint m_orderIndex;

        uint m_nextFileIndex;

        std::vector< HNSDCaptureFile > m_fileList;

        HNSDCAP_EXEC_STATE_T m_executionState;
};

class HNSDImageManager : public HNSDCaptureInfoInterface
{
    public:
         HNSDImageManager();
        ~HNSDImageManager();

        IMM_RESULT_T start();
        IMM_RESULT_T stop();

        virtual std::string getStorageRootPath();

        IMM_RESULT_T createCaptures( uint count, bool postAdvance );

        void deleteCapture( std::string capID );

        std::string getCaptureListJSON();

        std::string getCaptureJSON( std::string capID );

        HNSDCaptureRecord *getNextPendingCapture();

    private:

        // The root path for storing images
        std::string m_imageStoragePath;

        // A running capture index variable,
        // to keep capture requests in order
        uint m_nextOrderIndex;

        // Capture Records
        std::map< std::string, HNSDCaptureRecord > m_captureRecordMap;

};

#endif //__HNSD_IMAGE_MANAGER_H__
