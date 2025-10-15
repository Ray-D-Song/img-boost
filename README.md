# img-boost

High-performance image processing server that converts images of any format to WebP with optional resizing.

## Features

- **Asynchronous Architecture**: Separates image downloading and processing with dedicated thread pools
  - 4 download threads for concurrent HTTP requests
  - CPU-core-count processing threads for image conversion
- **Non-blocking Pipeline**: Download and processing operations run in parallel across multiple requests
- **Format Support**: JPEG, PNG, WebP input formats
- **Smart Resizing**: Maintains aspect ratio when only one dimension is specified
- **WebP Output**: Always outputs optimized WebP format

## Architecture

```
Request → Async Download (Thread Pool) → Async Processing (Thread Pool) → Response
```

Each request goes through a two-stage pipeline:
1. **Download Stage**: Fetches the image from URL asynchronously
2. **Processing Stage**: Decodes, resizes, and encodes to WebP in CPU thread pool

## Usage

```
http://img-boost.your-server.com?src={base64_encoded_url}&width=64&height=64&quality=80
```

### Parameters

* `src`: Source image URL (Base64 encoded) - **required**
* `width`: Target image width in pixels - optional
* `height`: Target image height in pixels - optional
* `quality`: WebP compression quality, range 0-100 - optional, default 80

### Examples

```bash
# Resize to 800px width, maintain aspect ratio
/?src={base64_url}&width=800

# Resize to specific dimensions
/?src={base64_url}&width=800&height=600

# Custom quality
/?src={base64_url}&width=800&quality=90

# Keep original size, just convert to WebP
/?src={base64_url}
```

## Notes

* Both width and height parameters are optional
* When only width or height is specified, the other dimension is calculated automatically to maintain aspect ratio
* When neither width nor height is specified, original dimensions are preserved
* Output format is always WebP

## Building

### Prerequisites

```bash
# Install dependencies (macOS)
brew install cmake openssl

# Build third-party libraries
./build_deps.sh
```

### Compile

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Run

```bash
./build/img-boost [port]

# Example
./build/img-boost 8080
```

## API Endpoints

* `/` - Main image processing endpoint
* `/health` - Health check endpoint (returns "OK")
* `/info` - API information and usage guide

## Author

[Ray-D-Song](https://github.com/ray-d-song)
