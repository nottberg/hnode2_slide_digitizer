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
    {
		std::cout << id->name() << ":  " << info.toString() << std::endl;
    }

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

    libcamera::ControlList ctrlList( libcamera::controls::controls );

	//libcamera::ControlList cl;
	//cl.set(libcamera::controls::AfMode, libcamera::controls::AfModeAuto);
	//cl.set(libcamera::controls::AfTrigger, libcamera::controls::AfTriggerCancel);
	// Following is app.SetControls(cl);
	//std::lock_guard<std::mutex> lock(control_mutex_);

	// Add new controls to the stored list. If a control is duplicated,
	// the value in the argument replaces the previously stored value.
	// These controls will be applied to the next StartCamera or request.
	//for (const auto &c : controls)
	//	ctrlList.set(c.first, c.second);



    // app.StartCamera();

	// This makes all the Request objects that we shall need.
	// makeRequests();
    std::vector< std::unique_ptr< libcamera::Request > > requests;

	auto free_buffers( frame_buffers );
	for( libcamera::StreamConfiguration &config : *configuration )
	{
		libcamera::Stream *stream = config.stream();
		if( stream == configuration->at(0).stream() )
		{
			if( free_buffers[ stream ].empty() )
			{
				std::cout << "Requests created" << std::endl;
				break;
			}

			std::unique_ptr< libcamera::Request > request = camera->createRequest();
			if( !request )
            {
                std::cerr << "failed to make request" << std::endl;
                return;
            }

			requests.push_back( std::move( request ) );
		}
		else if( free_buffers[ stream ].empty() )
        {
    		std::cerr << "concurrent streams need matching numbers of buffers" << std::endl;
            return;
        }

        libcamera::FrameBuffer *buffer = free_buffers[ stream ].front();
		free_buffers[ stream ].pop();
		if( requests.back()->addBuffer( stream, buffer ) < 0 )
        {
		    std::cerr << "failed to add buffer to request" << std::endl;
            return;
        }
	}

    std::cout << "Requests Complete" << std::endl;

#if 0
	// Build a list of initial controls that we must set in the camera before starting it.
	// We don't overwrite anything the application may have set before calling us.
	if (!ctrlList.get(controls::ScalerCrop) && options_->roi_width != 0 && options_->roi_height != 0)
	{
		Rectangle sensor_area = *camera_->properties().get(properties::ScalerCropMaximum);
		int x = options_->roi_x * sensor_area.width;
		int y = options_->roi_y * sensor_area.height;
		int w = options_->roi_width * sensor_area.width;
		int h = options_->roi_height * sensor_area.height;
		Rectangle crop(x, y, w, h);
		crop.translateBy(sensor_area.topLeft());
		LOG(2, "Using crop " << crop.toString());
		ctrlList.set(controls::ScalerCrop, crop);
	}

	if (!ctrlList.get(controls::AfWindows) && !ctrlList.get(controls::AfMetering) && options_->afWindow_width != 0 &&
		options_->afWindow_height != 0)
	{
		Rectangle sensor_area = *camera_->properties().get(properties::ScalerCropMaximum);
		int x = options_->afWindow_x * sensor_area.width;
		int y = options_->afWindow_y * sensor_area.height;
		int w = options_->afWindow_width * sensor_area.width;
		int h = options_->afWindow_height * sensor_area.height;
		Rectangle afwindows_rectangle[1];
		afwindows_rectangle[0] = Rectangle(x, y, w, h);
		afwindows_rectangle[0].translateBy(sensor_area.topLeft());
		LOG(2, "Using AfWindow " << afwindows_rectangle[0].toString());
		//activate the AfMeteringWindows
		ctrlList.set(controls::AfMetering, controls::AfMeteringWindows);
		//set window
		ctrlList.set(controls::AfWindows, afwindows_rectangle);
	}

	// Framerate is a bit weird. If it was set programmatically, we go with that, but
	// otherwise it applies only to preview/video modes. For stills capture we set it
	// as long as possible so that we get whatever the exposure profile wants.
	if (!ctrlList.get(controls::FrameDurationLimits))
	{
		if (StillStream())
			ctrlList.set(controls::FrameDurationLimits,
						  libcamera::Span<const int64_t, 2>({ INT64_C(100), INT64_C(1000000000) }));
		else if (!options_->framerate || options_->framerate.value() > 0)
		{
			int64_t frame_time = 1000000 / options_->framerate.value_or(DEFAULT_FRAMERATE); // in us
			ctrlList.set(controls::FrameDurationLimits,
						  libcamera::Span<const int64_t, 2>({ frame_time, frame_time }));
		}
	}

	if (!ctrlList.get(controls::ExposureTime) && options_->shutter)
		ctrlList.set(controls::ExposureTime, options_->shutter.get<std::chrono::microseconds>());
	if (!ctrlList.get(controls::AnalogueGain) && options_->gain)
		ctrlList.set(controls::AnalogueGain, options_->gain);
	if (!ctrlList.get(controls::AeMeteringMode))
		ctrlList.set(controls::AeMeteringMode, options_->metering_index);
	if (!ctrlList.get(controls::AeExposureMode))
		ctrlList.set(controls::AeExposureMode, options_->exposure_index);
	if (!ctrlList.get(controls::ExposureValue))
		ctrlList.set(controls::ExposureValue, options_->ev);
	if (!ctrlList.get(controls::AwbMode))
		ctrlList.set(controls::AwbMode, options_->awb_index);
	if (!ctrlList.get(controls::ColourGains) && options_->awb_gain_r && options_->awb_gain_b)
		ctrlList.set(controls::ColourGains,
					  libcamera::Span<const float, 2>({ options_->awb_gain_r, options_->awb_gain_b }));
	if (!ctrlList.get(controls::Brightness))
		ctrlList.set(controls::Brightness, options_->brightness);
	if (!ctrlList.get(controls::Contrast))
		ctrlList.set(controls::Contrast, options_->contrast);
	if (!ctrlList.get(controls::Saturation))
		ctrlList.set(controls::Saturation, options_->saturation);
	if (!ctrlList.get(controls::Sharpness))
		ctrlList.set(controls::Sharpness, options_->sharpness);

	// AF Controls, where supported and not already set
	if (!ctrlList.get(controls::AfMode) && camera_->controls().count(&controls::AfMode) > 0)
	{
		int afm = options_->afMode_index;
		if (afm == -1)
		{
			// Choose a default AF mode based on other options
			if (options_->lens_position || options_->set_default_lens_position || options_->af_on_capture)
				afm = controls::AfModeManual;
			else
				afm = camera_->controls().at(&controls::AfMode).max().get<int>();
		}
		ctrlList.set(controls::AfMode, afm);
	}
	if (!ctrlList.get(controls::AfRange) && camera_->controls().count(&controls::AfRange) > 0)
		ctrlList.set(controls::AfRange, options_->afRange_index);
	if (!ctrlList.get(controls::AfSpeed) && camera_->controls().count(&controls::AfSpeed) > 0)
		ctrlList.set(controls::AfSpeed, options_->afSpeed_index);

	if (ctrlList.get(controls::AfMode).value_or(controls::AfModeManual) == controls::AfModeAuto)
	{
		// When starting a viewfinder or video stream in AF "auto" mode,
		// trigger a scan now (but don't move the lens when capturing a still).
		// If an application requires more control over AF triggering, it may
		// override this behaviour with prior settings of AfMode or AfTrigger.
		if (!StillStream() && !ctrlList.get(controls::AfTrigger))
			ctrlList.set(controls::AfTrigger, controls::AfTriggerStart);
	}
	else if ((options_->lens_position || options_->set_default_lens_position) &&
			 camera_->controls().count(&controls::LensPosition) > 0 && !ctrlList.get(controls::LensPosition))
	{
		float f;
		if (options_->lens_position)
			f = options_->lens_position.value();
		else
			f = camera_->controls().at(&controls::LensPosition).def().get<float>();
		LOG(2, "Setting LensPosition: " << f);
		ctrlList.set(controls::LensPosition, f);
	}

	if (options_->flicker_period && !ctrlList.get(controls::AeFlickerMode) &&
		camera_->controls().find(&controls::AeFlickerMode) != camera_->controls().end() &&
		camera_->controls().find(&controls::AeFlickerPeriod) != camera_->controls().end())
	{
		ctrlList.set(controls::AeFlickerMode, controls::FlickerManual);
		ctrlList.set(controls::AeFlickerPeriod, options_->flicker_period.get<std::chrono::microseconds>());
	}
#endif

    std::cout << "Pre Camera Start" << std::endl;

	if( camera->start( &ctrlList ) )
    {
		std::cerr << "failed to start camera" << std::endl;
        return;
    }

	ctrlList.clear();

	bool camera_started = true;
	uint last_timestamp = 0;

	// post_processor_.Start();

    std::cout << "Pre requestCompleted.connect" << std::endl;

	camera->requestCompleted.connect( this, &CameraThread::requestComplete );

	for( std::unique_ptr< libcamera::Request > &request : requests )
	{
        std::cout << "Queueing request" << std::endl;

		if( camera->queueRequest( request.get() ) < 0 )
        {
			std::cerr << "Failed to queue request" << std::endl;
            return;
        }
	}

	std::cout << "Camera started!" << std::endl;



	//	app.StopCamera();
	//{
		// We don't want QueueRequest to run asynchronously while we stop the camera.
	//	std::lock_guard<std::mutex> lock(camera_stop_mutex_);
	//	if (camera_started_)
	//	{
			if( camera->stop() )
            {
				std::cout << "failed to stop camera" << std::endl;
            }

			//post_processor_.Stop();

			camera_started = false;
		//}
	//}

	if( camera )
		camera->requestCompleted.disconnect( this, &CameraThread::requestComplete );

	// An application might be holding a CompletedRequest, so queueRequest will get
	// called to delete it later, but we need to know not to try and re-queue it.
	//completed_requests.clear();

	//msg_queue_.Clear();

	requests.clear();

	ctrlList.clear(); // no need for mutex here

	std::cout << "Camera stopped!" << std::endl;



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

void
CameraThread::requestComplete( libcamera::Request *request )
{
    std::cout << "requestComplete - start" << std::endl;

	if( request->status() == libcamera::Request::RequestCancelled )
	{
		// If the request is cancelled while the camera is still running, it indicates
		// a hardware timeout. Let the application handle this error.
        std::cerr << "RequestCancelled, hardware timeout" << std::endl;
		return;
	}

	CompletedRequest *r = new CompletedRequest( seqCnt++, request );
//	libcamera::CompletedRequestPtr payload( r, [this]( libcamera::CompletedRequest *cr ) { this->queueRequest( cr ); });
//	{
//		std::lock_guard<std::mutex> lock( completed_requests_mutex_ );
//		completed_requests_.insert(r);
//	}

	// We calculate the instantaneous framerate in case anyone wants it.
	// Use the sensor timestamp if possible as it ought to be less glitchy than
	// the buffer timestamps.
//	auto ts = payload->metadata.get(controls::SensorTimestamp);
//	uint64_t timestamp = ts ? *ts : payload->buffers.begin()->second->metadata().timestamp;
//	if (last_timestamp_ == 0 || last_timestamp_ == timestamp)
//		payload->framerate = 0;
//	else
//		payload->framerate = 1e9 / (timestamp - last_timestamp_);
//	last_timestamp_ = timestamp;

//	post_processor_.Process(payload); // post-processor can re-use our shared_ptr
}
