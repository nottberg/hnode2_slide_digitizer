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
    {
		std::cerr << "camera manager failed to start, code " + std::to_string(-ret) << std::endl;
        return;
    }

	std::vector<std::shared_ptr<libcamera::Camera>> cameras = m_camMgr->cameras();
	// Do not show USB webcams as these are not supported in libcamera-apps!
	auto rem = std::remove_if(cameras.begin(), cameras.end(), [](auto &cam) { return cam->id().find("/usb") != std::string::npos; });
	cameras.erase(rem, cameras.end());
	std::sort(cameras.begin(), cameras.end(), [](auto l, auto r) { return l->id() > r->id(); });
	
	if (cameras.size() == 0)
    {
		std::cerr << "no cameras available" << std::endl;
        return;
    }

	//if (options_->camera >= cameras.size())
	//	throw std::runtime_error("selected camera is not available");

	std::string const &cam_id = cameras[0]->id();

    std::cout << "Cam 0 ID: " << cam_id << std::endl;

	std::shared_ptr<libcamera::Camera> camera = m_camMgr->get(cam_id);
	if( !camera )
    {
		std::cerr << "failed to find camera " + cam_id << std::endl;
        return;
    }

	if( camera->acquire() )
    {
		std::cerr << "failed to acquire camera " + cam_id << std::endl;
        return;
    }

	bool camera_acquired = true;

	std::cout << "Acquired camera " << cam_id << std::endl;

#if 0
	if (!options_->post_process_file.empty())
		post_processor_.Read(options_->post_process_file);
	// The queue takes over ownership from the post-processor.
	post_processor_.SetCallback(
		[this](CompletedRequestPtr &r) { this->msg_queue_.Post(Msg(MsgType::RequestComplete, std::move(r))); });

	if (options_->framerate)
	{
		std::unique_ptr<CameraConfiguration> config = camera_->generateConfiguration({ libcamera::StreamRole::Raw });
		const libcamera::StreamFormats &formats = config->at(0).formats();

		// Suppress log messages when enumerating camera modes.
		libcamera::logSetLevel("RPI", "ERROR");
		libcamera::logSetLevel("Camera", "ERROR");

		for (const auto &pix : formats.pixelformats())
		{
			for (const auto &size : formats.sizes(pix))
			{
				config->at(0).size = size;
				config->at(0).pixelFormat = pix;
				config->validate();
				camera_->configure(config.get());
				auto fd_ctrl = camera_->controls().find(&controls::FrameDurationLimits);
				sensor_modes_.emplace_back(size, pix, 1.0e6 / fd_ctrl->second.min().get<int64_t>());
			}
		}

		libcamera::logSetLevel("RPI", "INFO");
		libcamera::logSetLevel("Camera", "INFO");
	}
#endif

}
