#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace imgboost {

struct ImageOptions {
  // 0 == auto
  int width = 0;
  int height = 0;
  int quality = 80; // WebP quality (0-100)

  ImageOptions() = default;
  ImageOptions(int w, int h, int q) : width(w), height(h), quality(q) {}
};

class ImageProcessor {
public:
  ImageProcessor() = default;
  ~ImageProcessor() = default;

  std::vector<uint8_t> process(const std::vector<uint8_t> &input_data,
                               const ImageOptions &options);

private:
  // Detect format
  enum class ImageFormat { UNKNOWN, JPEG, PNG, WEBP };

  ImageFormat detect_format(const std::vector<uint8_t> &data);

  // Decode image
  bool decode_jpeg(const std::vector<uint8_t> &data,
                   std::vector<uint8_t> &rgba_data, int &width, int &height);

  bool decode_png(const std::vector<uint8_t> &data,
                  std::vector<uint8_t> &rgba_data, int &width, int &height);

  bool decode_webp(const std::vector<uint8_t> &data,
                   std::vector<uint8_t> &rgba_data, int &width, int &height);

  void resize_image(const std::vector<uint8_t> &src_rgba, int src_width,
                    int src_height, std::vector<uint8_t> &dst_rgba,
                    int dst_width, int dst_height);

  void calculate_dimensions(int src_width, int src_height, int req_width,
                            int req_height, int &dst_width, int &dst_height);

  std::vector<uint8_t> encode_webp(const std::vector<uint8_t> &rgba_data,
                                   int width, int height, int quality);
};

} // namespace imgboost
