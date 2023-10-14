#ifndef __HNSD_ACTION_REQUEST_H__
#define __HNSD_ACTION_REQUEST_H__

#include <stdint.h>

#include <string>
#include <mutex>
#include <thread>
#include <sstream>

#include <hnode2/HNReqWaitQueue.h>

#include "CameraManager.h"

typedef enum HNSDActionRequestType 
{
    HNSD_AR_TYPE_NOTSET                = 0,
    HNSD_AR_TYPE_START_SINGLE_CAPTURE  = 1  
}HNSD_AR_TYPE_T;


#if 0
typedef enum HNSDActionZoneUpdateMaskEnum
{
    HNSD_ZU_FLDMASK_CLEAR  = 0x00000000,
    HNSD_ZU_FLDMASK_NAME   = 0x00000001,
    HNSD_ZU_FLDMASK_DESC   = 0x00000002,
    HNSD_ZU_FLDMASK_SPW    = 0x00000004,
    HNSD_ZU_FLDMASK_MAXSPC = 0x00000008,
    HNSD_ZU_FLDMASK_MINSPC = 0x00000010,
    HNSD_ZU_FLDMASK_SWLST  = 0x00000020
}HNSD_ZU_FLDMASK_T;
#endif

typedef enum HNSDActionRequestResult
{
    HNSD_AR_RESULT_SUCCESS,
    HNSD_AR_RESULT_FAILURE
}HNSD_AR_RESULT_T;

class HNSDAction : public HNReqWaitAction
{
    public:
        HNSDAction();
       ~HNSDAction();

        void setType( HNSD_AR_TYPE_T type );

        void setNewID( std::string id );
        
        bool decodeStartCapture( std::istream& bodyStream );

        HNSD_AR_TYPE_T getType();

        bool hasNewObject( std::string &newID );
        bool hasRspContent( std::string &contentType );
        bool generateRspContent( std::ostream &ostr );

    private:
        HNSD_AR_TYPE_T  m_type;

        std::string m_newID;

        std::stringstream m_rspStream;
};

#endif // __HNSD_ACTION_REQUEST_H__
