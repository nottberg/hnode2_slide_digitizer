#ifndef __CAMERA_THREAD_H__
#define __CAMERA_THREAD_H__

#include <optional>

#include <libcamera/camera_manager.h>
#include <libcamera/camera.h>

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
