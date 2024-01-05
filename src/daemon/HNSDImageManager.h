#ifndef __HNSD_IMAGE_MANAGER_H__
#define __HNSD_IMAGE_MANAGER_H__

#include <string>

#include "HNSDPipeline.h"

typedef enum HNSDImageManagerResultEnum
{
    IMM_RESULT_SUCCESS,
    IMM_RESULT_FAILURE
}IMM_RESULT_T;

typedef enum HNSDCaptureExecutionStateEnum
{
    HNSDCAP_EXEC_STATE_NOTSET,
    HNSDCAP_EXEC_STATE_PENDING,
    HNSDCAP_EXEC_STATE_ACTIVE,
    HNSDCAP_EXEC_STATE_COMPLETE
}HNSDCAP_EXEC_STATE_T;

typedef enum HNSDCaptureFileType
{
    HNSDCAP_FT_NOTSET,
    HNSDCAP_FT_JPEG
}HNSDCAP_FT_T;

#if 0
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

        std::string getFilename();
        std::string getPathAndFile();

        void getInfo( std::string &fileName, std::string &purpose, std::string &tsStr );

    private:
        HNSDCAP_FT_T m_type;

        std::string m_path;

        uint m_indexNum;

        std::string m_purpose;

        std::string m_timestampStr;

};
#endif

class HNSDIMRootInterface
{
    public:
        virtual std::string getStorageRootPath() = 0;
};

class HNSDCaptureRecord : public HNSDPipelineClientInterface
{
    public:
        HNSDCaptureRecord( HNSDIMRootInterface *infoIntf );
       ~HNSDCaptureRecord();

        void generateNewID( uint64_t timestamp, uint orderIndex );

        void setID( std::string id );
        void setOrderIndex( uint index );

        void setPipeline( HNSDPipeline *pipeline );

        std::string getID();
        uint getOrderIndex();

        virtual HNSDPipeline* getPipelinePtr();

        HNSDCAP_EXEC_STATE_T getState();
        std::string getStateAsStr();

        //uint getFileCount();
        //IMM_RESULT_T getFileInfo( uint fileIndex, std::string &fileName, std::string &purpose, std::string &tsStr );
        //IMM_RESULT_T getFilePath( uint fileIndex, std::string &rtnPath );

        bool isPending();

        //virtual std::string registerNextFilename( std::string purpose );

        //virtual std::string getLastOutputPathAndFile();

        virtual void makePending();
        virtual void makeActive();
        virtual void makeComplete();

    private:

        HNSDIMRootInterface *m_infoIntf;

        std::string  m_id;

        std::string m_timestampStr;

        uint m_orderIndex;

        //uint m_nextFileIndex;

        HNSDPipeline *m_pipeline;

        //std::vector< HNSDCaptureFile* > m_fileList;

        HNSDCAP_EXEC_STATE_T m_executionState;
};

class HNSDImageManager : public HNSDIMRootInterface
{
    public:
         HNSDImageManager();
        ~HNSDImageManager();

        IMM_RESULT_T start( HNSDStorageManager *storageMgr, HNSDPipelineManagerIntf *pipelineMgr );
        IMM_RESULT_T stop();

        virtual std::string getStorageRootPath();

        IMM_RESULT_T createCaptures( uint count, bool postAdvance );

        void deleteCapture( std::string capID );

        std::string getCaptureListJSON();

        std::string getCaptureJSON( std::string capID );

        //IMM_RESULT_T getCapturePathAndFile( std::string captureID, uint fileIndex, std::string &rstPath );

        std::string getDefaultCaptureParameterListJSON();

    private:

        // The root path for storing images
        std::string m_imageStoragePath;

        // A running capture index variable,
        // to keep capture requests in order
        uint m_nextOrderIndex;

        // Capture Records
        std::map< std::string, HNSDCaptureRecord* > m_captureRecordMap;

        // The image transformation pipeline
        // HNSDPipeline m_pipeline;

        // The storage manager
        HNSDStorageManager *m_storageMgr;

        // The pointer to the pipeline manager object
        HNSDPipelineManagerIntf *m_pipelineMgr;
};

#endif //__HNSD_IMAGE_MANAGER_H__
