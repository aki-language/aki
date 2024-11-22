#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace aki {

class FileWriter {
 public:
  static constexpr size_t kMaxQueueSize =
      1000;  // Adjust based on memory constraints
  FileWriter() : should_stop_(false) {
    writer_thread_ = std::thread(&FileWriter::WriterThread, this);
  }

  ~FileWriter() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      should_stop_ = true;
    }
    cv_.notify_one();
    if (writer_thread_.joinable()) {
      writer_thread_.join();
    }
  }

  void EnqueueWrite(const base::Path& path, base::StringRefU8 content) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return write_queue_.size() < kMaxQueueSize; });

    write_queue_.push(
        {path, std::string(reinterpret_cast<const char*>(content.data()),
                           content.size())});
    cv_.notify_one();
  }

 private:
  void WriterThread() {
    while (true) {
      WriteRequest request;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock,
                 [this] { return !write_queue_.empty() || should_stop_; });

        if (should_stop_ && write_queue_.empty()) {
          break;
        }

        request = std::move(write_queue_.front());
        write_queue_.pop();
      }
      cv_.notify_one();

      // Write to file
      base::WriteFile(request.path,
                      reinterpret_cast<const byte*>(request.content.data()),
                      request.content.size());
    }
  }

  struct WriteRequest {
    base::Path path;
    std::string content;
  };

  std::queue<WriteRequest> write_queue_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::thread writer_thread_;
  bool should_stop_;
};
}  // namespace aki
