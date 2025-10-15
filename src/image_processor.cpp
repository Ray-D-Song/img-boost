#include "image_processor.h"
#include <algorithm>
#include <csetjmp>
#include <cstring>
#include <jpeglib.h>
#include <png.h>
#include <stdexcept>
#include <webp/decode.h>
#include <webp/encode.h>

namespace imgboost {

struct jpeg_error_mgr_ext {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

static void jpeg_error_exit(j_common_ptr cinfo) {
  jpeg_error_mgr_ext *err = (jpeg_error_mgr_ext *)cinfo->err;
  longjmp(err->setjmp_buffer, 1);
}

struct PNGReadState {
  const uint8_t *data;
  size_t size;
  size_t offset;
};

static void png_read_callback(png_structp png_ptr, png_bytep data,
                              png_size_t length) {
  PNGReadState *state = (PNGReadState *)png_get_io_ptr(png_ptr);
  if (state->offset + length > state->size) {
    png_error(png_ptr, "Read error");
    return;
  }
  memcpy(data, state->data + state->offset, length);
  state->offset += length;
}

ImageProcessor::ImageFormat
ImageProcessor::detect_format(const std::vector<uint8_t> &data) {
  if (data.size() < 12)
    return ImageFormat::UNKNOWN;

  // JPEG: FF D8 (JPEG SOI marker)
  if (data[0] == 0xFF && data[1] == 0xD8) {
    return ImageFormat::JPEG;
  }

  // PNG: 89 50 4E 47 0D 0A 1A 0A
  if (data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E &&
      data[3] == 0x47 && data[4] == 0x0D && data[5] == 0x0A &&
      data[6] == 0x1A && data[7] == 0x0A) {
    return ImageFormat::PNG;
  }

  // WebP: RIFF ???? WEBP
  if (data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F' &&
      data[8] == 'W' && data[9] == 'E' && data[10] == 'B' && data[11] == 'P') {
    return ImageFormat::WEBP;
  }

  return ImageFormat::UNKNOWN;
}

bool ImageProcessor::decode_jpeg(const std::vector<uint8_t> &data,
                                 std::vector<uint8_t> &rgba_data, int &width,
                                 int &height) {
  struct jpeg_decompress_struct cinfo;
  jpeg_error_mgr_ext jerr;

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpeg_error_exit;

  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    return false;
  }

  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, data.data(), data.size());
  jpeg_read_header(&cinfo, TRUE);

  cinfo.out_color_space = JCS_RGB;
  jpeg_start_decompress(&cinfo);

  width = cinfo.output_width;
  height = cinfo.output_height;
  int channels = cinfo.output_components;

  std::vector<uint8_t> row_buffer(width * channels);
  rgba_data.resize(width * height * 4);

  int row = 0;
  while (cinfo.output_scanline < cinfo.output_height) {
    uint8_t *row_ptr = row_buffer.data();
    jpeg_read_scanlines(&cinfo, &row_ptr, 1);

    for (int x = 0; x < width; ++x) {
      rgba_data[(row * width + x) * 4 + 0] = row_buffer[x * channels + 0];
      rgba_data[(row * width + x) * 4 + 1] = row_buffer[x * channels + 1];
      rgba_data[(row * width + x) * 4 + 2] = row_buffer[x * channels + 2];
      rgba_data[(row * width + x) * 4 + 3] = 255; // Alpha
    }
    row++;
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  return true;
}

bool ImageProcessor::decode_png(const std::vector<uint8_t> &data,
                                std::vector<uint8_t> &rgba_data, int &width,
                                int &height) {
  png_structp png_ptr =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_ptr)
    return false;

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, nullptr, nullptr);
    return false;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    return false;
  }

  PNGReadState state = {data.data(), data.size(), 0};
  png_set_read_fn(png_ptr, &state, png_read_callback);

  png_read_info(png_ptr, info_ptr);

  width = png_get_image_width(png_ptr, info_ptr);
  height = png_get_image_height(png_ptr, info_ptr);
  png_byte color_type = png_get_color_type(png_ptr, info_ptr);
  png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

  // Convert to RGBA
  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png_ptr);

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png_ptr);

  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  png_read_update_info(png_ptr, info_ptr);

  rgba_data.resize(width * height * 4);
  std::vector<png_bytep> row_pointers(height);
  for (int y = 0; y < height; y++) {
    row_pointers[y] = rgba_data.data() + y * width * 4;
  }

  png_read_image(png_ptr, row_pointers.data());
  png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

  return true;
}

bool ImageProcessor::decode_webp(const std::vector<uint8_t> &data,
                                 std::vector<uint8_t> &rgba_data, int &width,
                                 int &height) {
  uint8_t *decoded = WebPDecodeRGBA(data.data(), data.size(), &width, &height);
  if (!decoded)
    return false;

  rgba_data.assign(decoded, decoded + width * height * 4);
  WebPFree(decoded);

  return true;
}

void ImageProcessor::calculate_dimensions(int src_width, int src_height,
                                          int req_width, int req_height,
                                          int &dst_width, int &dst_height) {
  if (req_width == 0 && req_height == 0) {
    // Keep proportion
    dst_width = src_width;
    dst_height = src_height;
  } else if (req_width == 0) {
    dst_height = req_height;
    dst_width = (src_width * req_height) / src_height;
  } else if (req_height == 0) {
    dst_width = req_width;
    dst_height = (src_height * req_width) / src_width;
  } else {
    dst_width = req_width;
    dst_height = req_height;
  }

  if (dst_width < 1)
    dst_width = 1;
  if (dst_height < 1)
    dst_height = 1;
}

void ImageProcessor::resize_image(const std::vector<uint8_t> &src_rgba,
                                  int src_width, int src_height,
                                  std::vector<uint8_t> &dst_rgba, int dst_width,
                                  int dst_height) {
  dst_rgba.resize(dst_width * dst_height * 4);

  float x_ratio = static_cast<float>(src_width - 1) / dst_width;
  float y_ratio = static_cast<float>(src_height - 1) / dst_height;

  for (int y = 0; y < dst_height; y++) {
    for (int x = 0; x < dst_width; x++) {
      float gx = x * x_ratio;
      float gy = y * y_ratio;
      int gxi = static_cast<int>(gx);
      int gyi = static_cast<int>(gy);

      float c00[4], c10[4], c01[4], c11[4];
      for (int c = 0; c < 4; c++) {
        c00[c] = src_rgba[(gyi * src_width + gxi) * 4 + c];
        c10[c] = (gxi + 1 < src_width)
                     ? src_rgba[(gyi * src_width + gxi + 1) * 4 + c]
                     : c00[c];
        c01[c] = (gyi + 1 < src_height)
                     ? src_rgba[((gyi + 1) * src_width + gxi) * 4 + c]
                     : c00[c];
        c11[c] = (gxi + 1 < src_width && gyi + 1 < src_height)
                     ? src_rgba[((gyi + 1) * src_width + gxi + 1) * 4 + c]
                     : c00[c];
      }

      float x_weight = gx - gxi;
      float y_weight = gy - gyi;

      for (int c = 0; c < 4; c++) {
        float val = c00[c] * (1 - x_weight) * (1 - y_weight) +
                    c10[c] * x_weight * (1 - y_weight) +
                    c01[c] * (1 - x_weight) * y_weight +
                    c11[c] * x_weight * y_weight;
        dst_rgba[(y * dst_width + x) * 4 + c] = static_cast<uint8_t>(val);
      }
    }
  }
}

std::vector<uint8_t>
ImageProcessor::encode_webp(const std::vector<uint8_t> &rgba_data, int width,
                            int height, int quality) {
  uint8_t *output = nullptr;
  size_t output_size = WebPEncodeRGBA(rgba_data.data(), width, height,
                                      width * 4, quality, &output);

  if (output_size == 0 || !output) {
    throw std::runtime_error("WebP encoding failed");
  }

  std::vector<uint8_t> result(output, output + output_size);
  WebPFree(output);

  return result;
}

std::vector<uint8_t>
ImageProcessor::process(const std::vector<uint8_t> &input_data,
                        const ImageOptions &options) {
  // Detect format
  ImageFormat format = detect_format(input_data);
  if (format == ImageFormat::UNKNOWN) {
    throw std::runtime_error("Unknown image format");
  }

  // 2. Decode
  std::vector<uint8_t> rgba_data;
  int width = 0, height = 0;
  bool success = false;

  switch (format) {
  case ImageFormat::JPEG:
    success = decode_jpeg(input_data, rgba_data, width, height);
    break;
  case ImageFormat::PNG:
    success = decode_png(input_data, rgba_data, width, height);
    break;
  case ImageFormat::WEBP:
    success = decode_webp(input_data, rgba_data, width, height);
    break;
  default:
    break;
  }

  if (!success) {
    throw std::runtime_error("Failed to decode image");
  }

  // Compute target size
  int dst_width, dst_height;
  calculate_dimensions(width, height, options.width, options.height, dst_width,
                       dst_height);

  std::vector<uint8_t> final_rgba;
  if (dst_width != width || dst_height != height) {
    resize_image(rgba_data, width, height, final_rgba, dst_width, dst_height);
  } else {
    final_rgba = std::move(rgba_data);
  }

  return encode_webp(final_rgba, dst_width, dst_height, options.quality);
}

} // namespace imgboost
