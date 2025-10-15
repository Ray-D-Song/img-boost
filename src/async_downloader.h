#pragma once

#include "httplib.h"
#include "thread_pool.h"
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace imgboost {

struct DownloadResult {
  bool success;
  int status_code;
  std::vector<uint8_t> data;
  std::string error_message;
};

class AsyncDownloader {
public:
  explicit AsyncDownloader(size_t num_threads = 4)
      : thread_pool_(num_threads) {}

  std::future<DownloadResult> download_async(const std::string &url) {
    return thread_pool_.enqueue(
        [url]() -> DownloadResult { return download_sync(url); });
  }

  void download_async(const std::string &url,
                      std::function<void(DownloadResult)> callback) {
    thread_pool_.enqueue([url, callback]() {
      DownloadResult result = download_sync(url);
      callback(result);
    });
  }

private:
  static DownloadResult download_sync(const std::string &url) {
    DownloadResult result;
    result.success = false;
    result.status_code = 0;

    try {
      std::string protocol, host, path;
      size_t protocol_end = url.find("://");
      if (protocol_end == std::string::npos) {
        result.error_message = "Invalid URL format";
        return result;
      }

      protocol = url.substr(0, protocol_end);
      size_t host_start = protocol_end + 3;
      size_t path_start = url.find('/', host_start);

      if (path_start == std::string::npos) {
        host = url.substr(host_start);
        path = "/";
      } else {
        host = url.substr(host_start, path_start - host_start);
        path = url.substr(path_start);
      }

      std::cout << "[AsyncDownloader] Downloading from host: " << host
                << ", path: " << path << std::endl;

      httplib::Client cli(protocol + "://" + host);
      cli.set_follow_location(true);
      cli.set_connection_timeout(10, 0); // 10 seconds
      cli.set_read_timeout(30, 0);       // 30 seconds

      auto download_res = cli.Get(path.c_str());

      if (!download_res) {
        result.error_message = "Failed to connect or download";
        return result;
      }

      result.status_code = download_res->status;

      if (download_res->status != 200) {
        result.error_message =
            "HTTP status: " + std::to_string(download_res->status);
        return result;
      }

      result.data.assign(download_res->body.begin(), download_res->body.end());
      result.success = true;

      std::cout << "[AsyncDownloader] Downloaded " << result.data.size()
                << " bytes" << std::endl;

    } catch (const std::exception &e) {
      result.error_message = std::string("Exception: ") + e.what();
    }

    return result;
  }

  ThreadPool thread_pool_;
};

} // namespace imgboost
