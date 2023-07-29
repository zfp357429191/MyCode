#include "MyFFmpeg.hpp"

#include <iostream>

// C方式引入FFmpeg头文件
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavutil/version.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

MyFFmpeg::MyFFmpeg() {

}

MyFFmpeg::~MyFFmpeg() {

}

void MyFFmpeg::PrintFFmpegInfo() {
    std::cout << "FFmpeg info: " << av_version_info() << std::endl;
}

