#include <chrono>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <iostream>
#include <sys/mman.h>
#include <map>
#include <queue>

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
    // Open Camera
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




	// app.ConfigureStill(still_flags);


	std::cout << "Configuring still capture..." << std::endl;

	// Always request a raw stream as this forces the full resolution capture mode.
	// (options_->mode can override the choice of camera mode, however.)
	std::vector<libcamera::StreamRole> stream_roles = { libcamera::StreamRole::StillCapture, libcamera::StreamRole::Raw };
	std::unique_ptr< libcamera::CameraConfiguration > configuration = camera->generateConfiguration( stream_roles );
	if( !configuration )
    {
		std::cerr << "failed to generate still capture configuration" << std::endl;
        return;
    }

	// Now we get to override any of the default settings from the options_->
#if 0    
	if (flags & FLAG_STILL_BGR)
		configuration->at(0).pixelFormat = libcamera::formats::BGR888;
	else if (flags & FLAG_STILL_RGB)
		configuration->at(0).pixelFormat = libcamera::formats::RGB888;
	else
		configuration->at(0).pixelFormat = libcamera::formats::YUV420;
	if ((flags & FLAG_STILL_BUFFER_MASK) == FLAG_STILL_DOUBLE_BUFFER)
		configuration->at(0).bufferCount = 2;
	else if ((flags & FLAG_STILL_BUFFER_MASK) == FLAG_STILL_TRIPLE_BUFFER)
		configuration->at(0).bufferCount = 3;
	else if (options->buffer_count > 0)
		configuration->at(0).bufferCount = options->buffer_count;
	if (options->width)
		configuration->at(0).size.width = options->width;
	if (options->height)
		configuration->at(0).size.height = options->height;
	configuration->at(0).colorSpace = libcamera::ColorSpace::Sycc;
	configuration->transform = options->transform;

	post_processor.AdjustConfig("still", &configuration->at(0));

	if (options->mode.bit_depth)
	{
		configuration->at(1).size = options->mode.Size();
		configuration->at(1).pixelFormat = mode_to_pixel_format(options->mode);
	}
	configuration->at(1).bufferCount = configuration->at(0).bufferCount;

	configureDenoise(options->denoise == "auto" ? "cdn_hq" : options->denoise);
#endif
	
    // setupCapture();

	// First finish setting up the configuration.
#if 0    
	libcamera::CameraConfiguration::Status validation = configuration->validate();
	if (validation == CameraConfiguration::Invalid)
		throw std::runtime_error("failed to valid stream configurations");
	else if (validation == CameraConfiguration::Adjusted)
		LOG(1, "Stream configuration adjusted");
#endif

	if( camera->configure( configuration.get() ) < 0 )
    	{
		std::cerr << "failed to configure streams" << std::endl;
        	return;
    	}

	std::cout << "Camera streams configured" << std::endl;

	std::cout << "=== Available controls ===" << std::endl;
	for( auto const &[id, info] : camera->controls() )
		std::cout << id->name() << ":  " << info.toString() << std::endl;

	// Next allocate all the buffers we need, mmap them and store them on a free list.

    std::map< libcamera::FrameBuffer *, std::vector< libcamera::Span<uint8_t> > > mapped_buffers;
    std::map< libcamera::Stream *, std::queue< libcamera::FrameBuffer * > > frame_buffers;

	libcamera::FrameBufferAllocator *allocator = new libcamera::FrameBufferAllocator( camera );
	for( libcamera::StreamConfiguration &config : *configuration )
	{
		libcamera::Stream *stream = config.stream();

		if( allocator->allocate( stream ) < 0 )
        {
			std::cerr << "failed to allocate capture buffers" << std::endl;
            return;
        }

		for( const std::unique_ptr<libcamera::FrameBuffer> &buffer : allocator->buffers( stream ) )
		{
			// "Single plane" buffers appear as multi-plane here, but we can spot them because then
			// planes all share the same fd. We accumulate them so as to mmap the buffer only once.
			size_t buffer_size = 0;
			for( unsigned i = 0; i < buffer->planes().size(); i++ )
			{
				const libcamera::FrameBuffer::Plane &plane = buffer->planes()[i];
				buffer_size += plane.length;
				if( i == buffer->planes().size() - 1 || plane.fd.get() != buffer->planes()[i + 1].fd.get() )
				{
					void *memory = mmap( NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, plane.fd.get(), 0 );
					mapped_buffers[ buffer.get() ].push_back(	libcamera::Span<uint8_t>( static_cast<uint8_t *>(memory), buffer_size ) );
					buffer_size = 0;
				}
			}
			frame_buffers[ stream ].push( buffer.get() );
		}
	}
	
    std::cout << "Buffers allocated and mapped" << std::endl;

	// startPreview();

	// The requests will be made when StartCamera() is called.

#if 0
	streams_["still"] = configuration->at(0).stream();
	streams_["raw"] = configuration->at(1).stream();

	post_processor.Configure();
#endif

	std::cout << "Still capture setup complete" << std::endl;


	//libcamera::ControlList cl;
	//cl.set(libcamera::controls::AfMode, libcamera::controls::AfModeAuto);
	//cl.set(libcamera::controls::AfTrigger, libcamera::controls::AfTriggerCancel);
	//app.SetControls(cl);
	
    // app.StartCamera();

	//else if (app.StillStream())
	//{
	//	app.StopCamera();
	//	LOG(1, "Still capture image received");
	//	save_images(app, completed_request);
	//	save_metadata(options, completed_request->metadata);
    //}


    // Close camera
	//preview_.reset();

	if( camera_acquired )
		camera->release();
	camera_acquired = false;

	camera.reset();

	m_camMgr.reset();

	//if (!options_->help)
	std::cout << "Camera closed" << std::endl;

}
