#ifndef __CAMERA_THREAD_H__
#define __CAMERA_THREAD_H__

#include <any>
#include <map>
#include <mutex>
#include <string>
#include <memory>
#include <optional>

#include <libcamera/base/span.h>
#include <libcamera/camera_manager.h>
#include <libcamera/camera.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/request.h>
#include <libcamera/property_ids.h>

class Metadata
{
    public:
        Metadata() = default;

        Metadata(Metadata const &other)
        {
                std::scoped_lock other_lock(other.mutex_);
                data_ = other.data_;
        }

        Metadata(Metadata &&other)
        {
                std::scoped_lock other_lock(other.mutex_);
                data_ = std::move(other.data_);
                other.data_.clear();
        }

        template <typename T>
        void Set(std::string const &tag, T &&value)
        {
                std::scoped_lock lock(mutex_);
                data_.insert_or_assign(tag, std::forward<T>(value));
        }

        template <typename T>
        int Get(std::string const &tag, T &value) const
        {
                std::scoped_lock lock(mutex_);
                auto it = data_.find(tag);
                if (it == data_.end())
                        return -1;
                value = std::any_cast<T>(it->second);
                return 0;
        }

        void Clear()
        {
                std::scoped_lock lock(mutex_);
                data_.clear();
        }

        Metadata &operator=(Metadata const &other)
        {
                std::scoped_lock lock(mutex_, other.mutex_);
                data_ = other.data_;
                return *this;
        }

        Metadata &operator=(Metadata &&other)
        {
                std::scoped_lock lock(mutex_, other.mutex_);
                data_ = std::move(other.data_);
                other.data_.clear();
                return *this;
        }

        void Merge(Metadata &other)
        {
                std::scoped_lock lock(mutex_, other.mutex_);
                data_.merge(other.data_);
        }

        template <typename T>
        T *GetLocked(std::string const &tag)
        {
                // This allows in-place access to the Metadata contents,
                // for which you should be holding the lock.
                auto it = data_.find(tag);
                if (it == data_.end())
                        return nullptr;
                return std::any_cast<T>(&it->second);
        }

        template <typename T>
        void SetLocked(std::string const &tag, T &&value)
        {
                // Use this only if you're holding the lock yourself.
                data_.insert_or_assign(tag, std::forward<T>(value));
        }

        // Note: use of (lowercase) lock and unlock means you can create scoped
        // locks with the standard lock classes.
        // e.g. std::lock_guard<RPiController::Metadata> lock(metadata)
        void lock() { mutex_.lock(); }
        void unlock() { mutex_.unlock(); }

    private:
        mutable std::mutex mutex_;
        std::map<std::string, std::any> data_;
};

struct CompletedRequest
{
        CompletedRequest(unsigned int seq, libcamera::Request *r)
                : sequence(seq), buffers(r->buffers()), metadata(r->metadata()), request(r)
        {
                r->reuse();
        }
        unsigned int sequence;
        libcamera::Request::BufferMap buffers;
        libcamera::ControlList metadata;
        libcamera::Request *request;
        float framerate;
        Metadata post_process_metadata;
};

//using CompletedRequestPtr = std::shared_ptr<CompletedRequest>;

typedef enum CameraThreadCaptureStateEnum
{
    CTC_STATE_IDLE,
    CTC_STATE_FOCUS,
    CTC_STATE_CAPTURED
} CTC_STATE_T;

class CameraThread
{
    public:
         CameraThread();
        ~CameraThread();

        void test();

        void requestComplete( libcamera::Request *request );

    private:

        uint64_t seqCnt;

        std::mutex  m_captureStateMutex;
        CTC_STATE_T m_captureState;

        std::unique_ptr<libcamera::CameraManager> m_camMgr;
};

#endif //__CAMERA_THREAD_H__
