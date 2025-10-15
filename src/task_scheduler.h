#pragma once

#include "async_downloader.h"
#include "image_processor.h"
#include "thread_pool.h"
#include <functional>
#include <memory>

namespace imgboost {

struct ProcessingResult {
  bool success;
  std::vector<uint8_t> output_data;
  std::string error_message;
  int http_status; // HTTP response
};

using ProcessingCallback = std::function<void(ProcessingResult)>;

class TaskScheduler {
public:
  TaskScheduler(size_t download_threads = 4, size_t processing_threads = 0)
      : downloader_(download_threads),
        processing_pool_(processing_threads == 0
                             ? std::thread::hardware_concurrency()
                             : processing_threads) {
    std::cout << "[TaskScheduler] Initialized with " << download_threads
              << " download threads and " << processing_pool_.size()
              << " processing threads" << std::endl;
  }

  // Asynchronous download => Synchronous conversion
  void process_image_async(const std::string &image_url,
                           const ImageOptions &options,
                           ProcessingCallback callback) {
    // Download
    downloader_.download_async(image_url, [this, options, callback](
                                              DownloadResult download_result) {
      // Process after download
      if (!download_result.success) {
        ProcessingResult result;
        result.success = false;
        result.error_message =
            "Download failed: " + download_result.error_message;
        result.http_status = download_result.status_code == 0
                                 ? 502
                                 : download_result.status_code;
        callback(result);
        return;
      }

      // Convert img in thread pool
      processing_pool_.enqueue([download_result, options, callback]() {
        ProcessingResult result;
        try {
          ImageProcessor processor;
          result.output_data = processor.process(download_result.data, options);
          result.success = true;
          result.http_status = 200;

          std::cout << "[TaskScheduler] Processing completed, output size: "
                    << result.output_data.size() << " bytes" << std::endl;
        } catch (const std::exception &e) {
          result.success = false;
          result.error_message = std::string("Processing failed: ") + e.what();
          result.http_status = 500;
        }
        callback(result);
      });
    });
  }

private:
  AsyncDownloader downloader_;
  ThreadPool processing_pool_;
};

} // namespace imgboost
