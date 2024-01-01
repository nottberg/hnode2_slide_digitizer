#ifndef __HNSD_PIPELINE_MANAGER_H__
#define __HNSD_PIPELINE_MANAGER_H__

#include <string>

class HNSDPipelineManager : public HNSDPipelineManagerIntf
{
    public:
        HNSDPipelineManager();
       ~HNSDPipelineManager();

        void setStorageManager( HNSDStorageManager *storageMgr );
        HNSDStorageManager* getStorageManager();

        virtual HNSDP_RESULT_T allocatePipeline( HNSDP_TYPE_T type, std::string ownerID, HNSDPipelineClientInterface *clientInf, HNSDPipeline **rtnPipeline );

        virtual HNSDP_RESULT_T submitPipelineForExecution( HNSDPipeline *pipeline );

        HNSDPipeline* getNextPendingPipeline();

        virtual void releasePipeline( HNSDPipeline **pipeline );

    private:

        HNSDStorageManager *m_storageMgr;

        std::list< HNSDPipeline* > m_pendingQueue;
};

#endif //__HNSD_PIPELINE_MANAGER_H__
