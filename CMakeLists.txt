cmake_minimum_required(VERSION 3.15)
project(img-boost VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_STATIC "Build with static libraries" ON)

if(BUILD_STATIC)
    if(WIN32)
        set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a)
        set(BUILD_SHARED_LIBS OFF)
        # Use static runtime on Windows
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
        set(BUILD_SHARED_LIBS OFF)
        # macOS not support -static-libgcc
        if(NOT APPLE)
            set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
        endif()
    endif()
endif()

set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

# OpenSSL for HTTPS support
find_package(OpenSSL REQUIRED)

# On Windows with vcpkg, use find_package for other libraries
if(WIN32 AND DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
    find_package(WebP CONFIG REQUIRED)
    find_package(JPEG REQUIRED)
    find_package(PNG REQUIRED)
    find_package(ZLIB REQUIRED)
    set(USE_VCPKG TRUE)
else()
    # Unix: use manually built libraries
    include_directories(
        ${CMAKE_SOURCE_DIR}/src
        ${THIRD_PARTY_DIR}/include
        ${OPENSSL_INCLUDE_DIR}
    )

    link_directories(
        ${THIRD_PARTY_DIR}/lib
    )
    set(USE_VCPKG FALSE)
endif()

if(USE_VCPKG)
    include_directories(
        ${CMAKE_SOURCE_DIR}/src
        ${THIRD_PARTY_DIR}/include
        ${OPENSSL_INCLUDE_DIR}
    )
endif()

set(SOURCES
    src/main.cpp
    src/image_processor.cpp
)

add_executable(img-boost ${SOURCES})

target_compile_definitions(img-boost PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

if(USE_VCPKG)
    # vcpkg libraries (Windows)
    target_link_libraries(img-boost
        WebP::webp
        WebP::webpdemux
        WebP::sharpyuv
        JPEG::JPEG
        PNG::PNG
        ZLIB::ZLIB
        OpenSSL::SSL
        OpenSSL::Crypto
    )
else()
    # Manually built libraries (Unix)
    target_link_libraries(img-boost
        webp
        webpdemux
        webpdecoder
        sharpyuv
        jpeg
        png16
        z
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif()

# Add pthread on Unix-like systems only
if(NOT WIN32)
    target_link_libraries(img-boost pthread)
endif()

# Windows specific libraries
if(WIN32)
    target_link_libraries(img-boost ws2_32 crypt32)
endif()

install(TARGETS img-boost DESTINATION bin)
