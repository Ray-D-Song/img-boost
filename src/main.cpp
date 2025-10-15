#include "httplib.h"
#include "task_scheduler.h"
#include "utils.h"
#include <condition_variable>
#include <cstdlib>
#include <future>
#include <iostream>
#include <mutex>
#include <string>

using namespace imgboost;

int main(int argc, char *argv[]) {
  std::string host = "0.0.0.0";
  int port = 8080;

  if (const char *env_port = std::getenv("PORT")) {
    port = std::atoi(env_port);
  }
  if (argc >= 2) {
    port = std::atoi(argv[1]);
  }

  // Async task handler
  // By default, it uses 4 download threads and a number of processing threads
  // equal to the number of CPU cores
  TaskScheduler scheduler(4, 0);

  httplib::Server svr;

  svr.Get("/health", [](const httplib::Request &, httplib::Response &res) {
    res.set_content("OK", "text/plain");
  });

  svr.Get("/", [&scheduler](const httplib::Request &req,
                            httplib::Response &res) {
    try {
      auto src_param = req.get_param_value("src");
      if (src_param.empty()) {
        res.status = 400;
        res.set_content("Missing 'src' parameter", "text/plain");
        return;
      }

      int width = parse_int_param(req.get_param_value("width"), 0, 0, 8192);
      int height = parse_int_param(req.get_param_value("height"), 0, 0, 8192);
      int quality = parse_int_param(req.get_param_value("quality"), 80, 0, 100);

      std::string image_url;
      try {
        std::string decoded_src = url_decode(src_param);
        std::vector<uint8_t> url_bytes = base64_decode(decoded_src);
        image_url = std::string(url_bytes.begin(), url_bytes.end());
      } catch (const std::exception &e) {
        res.status = 400;
        res.set_content("Invalid base64 encoded URL", "text/plain");
        return;
      }

      std::cout << "[INFO] Processing request for: " << image_url << std::endl;

      std::promise<ProcessingResult> promise;
      std::future<ProcessingResult> future = promise.get_future();

      ImageOptions options(width, height, quality);

      // Submit async task
      scheduler.process_image_async(image_url, options,
                                    [&promise](ProcessingResult result) {
                                      promise.set_value(std::move(result));
                                    });

      // wait util task done
      ProcessingResult result = future.get();

      if (result.success) {
        res.set_content(
            reinterpret_cast<const char *>(result.output_data.data()),
            result.output_data.size(), "image/webp");
        res.set_header("Cache-Control", "public, max-age=31536000");
        std::cout << "[INFO] Request completed successfully" << std::endl;
      } else {
        res.status = result.http_status;
        res.set_content(result.error_message, "text/plain");
        std::cerr << "[ERROR] " << result.error_message << std::endl;
      }

    } catch (const std::exception &e) {
      std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
      res.status = 500;
      res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
    }
  });

  svr.Get("/info", [](const httplib::Request &, httplib::Response &res) {
    std::string info = R"(
img-boost - High-performance image processing server

Usage:
  /?src={base64_encoded_image_data}&width={width}&height={height}&quality={quality}

Parameters:
  - src: Base64 encoded image data (required)
  - width: Target width in pixels (optional, 0 = auto)
  - height: Target height in pixels (optional, 0 = auto)
  - quality: WebP quality 0-100 (optional, default = 80)

Notes:
  - If only width or height is specified, the other dimension is calculated to maintain aspect ratio
  - If neither width nor height is specified, original size is preserved
  - Output format is always WebP

Example:
  /?src={base64_data}&width=800&quality=85

Health check:
  /health
)";
    res.set_content(info, "text/plain");
  });

  std::cout << "img-boost server starting..." << std::endl;
  std::cout << "Listening on " << host << ":" << port << std::endl;
  std::cout << "Health check: http://" << host << ":" << port << "/health"
            << std::endl;
  std::cout << "API info: http://" << host << ":" << port << "/info"
            << std::endl;

  if (!svr.listen(host.c_str(), port)) {
    std::cerr << "Failed to start server on " << host << ":" << port
              << std::endl;
    return 1;
  }

  return 0;
}
