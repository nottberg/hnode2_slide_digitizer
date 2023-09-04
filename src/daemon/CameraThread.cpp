#include <chrono>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <iostream>

//#include "core/frame_info.hpp"
//#include "core/libcamera_app.hpp"
//#include "core/still_options.hpp"

//#include "output/output.hpp"

//#include "image/image.hpp"

#include "CameraThread.h"

CameraThread::CameraThread()
{

}

CameraThread::~CameraThread()
{

}

void
CameraThread::test()
{
  	std::cout << "Opening camera..." << std::endl;

	m_camMgr = std::make_unique<libcamera::CameraManager>();

    std::cout << "libcamera version: " << m_camMgr->version() << std::endl;

	int ret = m_camMgr->start();
	if (ret)
		std::cerr << "camera manager failed to start, code " + std::to_string(-ret) << std::endl;

}
