#include <iostream>

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"

}

// 打开本地音视频文件
void TestOpenLocalVideo() {

    AVFormatContext* pFormat = nullptr;
    std::string strVideoPath = "../../MediaSource/t1002.mp4";

    // 打开输入
    int ret = avformat_open_input(&pFormat, strVideoPath.c_str(), nullptr, nullptr);
    if (ret) {
        std::cout << "avformat_open_input failed" << std::endl;
        return;
    }
    std::cout << "avformat_open_input success" << std::endl;

    // 查找流信息
    ret = avformat_find_stream_info(pFormat, nullptr);
    if (ret) {
        std::cout << "avformat_find_stream_info failed" << std::endl;
        return;
    }
    std::cout << "avformat_find_stream_info success" << std::endl;

    // 获取视频时长
    auto sTime = pFormat->duration;
    int sMin = static_cast<int>((sTime / 1000000) / 60);
    int sSec = static_cast<int>((sTime / 1000000) % 60);

    std::cout << "视频总时长：" << sMin << "分" << sSec << "秒" << std::endl;
    // 输出视频信息
    av_dump_format(pFormat, 0, strVideoPath.c_str(), 0);

    std::cout << "***************************************************" << std::endl;

    // 寻找流
    int nVideoStream = av_find_best_stream(pFormat, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, NULL);
    AVCodecContext* pCodecContext = pFormat->streams[nVideoStream]->codec;
    AVCodec* pCodec = avcodec_find_decoder(pCodecContext->codec_id);
    if (!pCodec) {
        std::cout << "avcodec_find_decoder failed" << std::endl;
        return;
    }
    std::cout << "avcodec_find_decoder success" << std::endl;

    ret = avcodec_open2(pCodecContext, pCodec, nullptr);
    if (ret) {
        std::cout << "avcodec_open2 failed" << std::endl;
        return;
    }
    std::cout << "avcodec_open2 success" << std::endl;

    // 解码视频
    // 申请原始空间 --> 创建帧空间
    AVFrame* pFrame = av_frame_alloc();
    AVFrame* pFrameYUV = av_frame_alloc();
    int nWidth = pCodecContext->width;
    int nHeight = pCodecContext->height;

    // 分配空间 进行图像转换
    int nSize = avpicture_get_size(AV_PIX_FMT_YUV420P, nWidth, nHeight);
    uint8_t* pBuff = (uint8_t*)av_malloc(nSize);



}

// 打开网络流视频
void TestNetworkVideo() {

    // 初始化网络设置
    avformat_network_init();

    AVFormatContext* pFormat = nullptr;
    std::string strVideoPath = "http://ivi.bupt.edu.cn/hls/cctv3hd.m3u8";

    AVDictionary* opt = nullptr;
    av_dict_set(&opt, "rtsp_transport", "tcp", 0);
    av_dict_set(&opt, "max_delay", "550", 0);

    // 打开输入
    int ret = avformat_open_input(&pFormat, strVideoPath.c_str(), nullptr, &opt);
    if (ret) {
        std::cout << "avformat_open_input failed" << std::endl;
        return;
    }

    std::cout << "avformat_open_input success" << std::endl;

    // 查找流信息
    ret = avformat_find_stream_info(pFormat, nullptr);
    if (ret) {
        std::cout << "avformat_find_stream_info failed" << std::endl;
        return;
    }

    std::cout << "avformat_find_stream_info success" << std::endl;

    //// 获取视频时长
    //auto sTime = pFormat->duration;
    //int sMin = static_cast<int>((sTime / 1000000) / 60);
    //int sSec = static_cast<int>((sTime / 1000000) % 60);

    //std::cout << "视频总时长：" << sMin << "分" << sSec << "秒" << std::endl;
    // 输出视频信息
    av_dump_format(pFormat, 0, strVideoPath.c_str(), 0);

    std::cout << "***************************************************" << std::endl;

}

int main() {

    TestOpenLocalVideo();

    //TestNetworkVideo();

    return 0;
}