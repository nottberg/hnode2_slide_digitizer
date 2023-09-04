#ifndef __CAMERA_THREAD_H__
#define __CAMERA_THREAD_H__

#include <libcamera/camera_manager.h>

class CameraThread
{
    public:
         CameraThread();
        ~CameraThread();

        void test();

    private:

        std::unique_ptr<libcamera::CameraManager> m_camMgr;
};

#endif //__CAMERA_THREAD_H__
