#ifndef __CAMERA_THREAD_H__
#define __CAMERA_THREAD_H__

#include <optional>

#include <libcamera/base/span.h>
#include <libcamera/camera_manager.h>
#include <libcamera/camera.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/framebuffer_allocator.h>

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
