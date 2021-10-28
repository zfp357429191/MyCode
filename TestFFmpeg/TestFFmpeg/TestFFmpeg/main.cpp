#include <iostream>

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"

}

// �򿪱�������Ƶ�ļ�
void TestOpenLocalVideo() {

    AVFormatContext* pFormat = nullptr;
    std::string strVideoPath = "../../MediaSource/t1002.mp4";

    // ������
    int ret = avformat_open_input(&pFormat, strVideoPath.c_str(), nullptr, nullptr);
    if (ret) {
        std::cout << "avformat_open_input failed" << std::endl;
        return;
    }
    std::cout << "avformat_open_input success" << std::endl;

    // ��������Ϣ
    ret = avformat_find_stream_info(pFormat, nullptr);
    if (ret) {
        std::cout << "avformat_find_stream_info failed" << std::endl;
        return;
    }
    std::cout << "avformat_find_stream_info success" << std::endl;

    // ��ȡ��Ƶʱ��
    auto sTime = pFormat->duration;
    int sMin = static_cast<int>((sTime / 1000000) / 60);
    int sSec = static_cast<int>((sTime / 1000000) % 60);

    std::cout << "��Ƶ��ʱ����" << sMin << "��" << sSec << "��" << std::endl;
    // �����Ƶ��Ϣ
    av_dump_format(pFormat, 0, strVideoPath.c_str(), 0);

    std::cout << "***************************************************" << std::endl;

    // Ѱ����
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

    // ������Ƶ
    // ����ԭʼ�ռ� --> ����֡�ռ�
    AVFrame* pFrame = av_frame_alloc();
    AVFrame* pFrameYUV = av_frame_alloc();
    int nWidth = pCodecContext->width;
    int nHeight = pCodecContext->height;

    // ����ռ� ����ͼ��ת��
    int nSize = avpicture_get_size(AV_PIX_FMT_YUV420P, nWidth, nHeight);
    uint8_t* pBuff = (uint8_t*)av_malloc(nSize);



}

// ����������Ƶ
void TestNetworkVideo() {

    // ��ʼ����������
    avformat_network_init();

    AVFormatContext* pFormat = nullptr;
    std::string strVideoPath = "http://ivi.bupt.edu.cn/hls/cctv3hd.m3u8";

    AVDictionary* opt = nullptr;
    av_dict_set(&opt, "rtsp_transport", "tcp", 0);
    av_dict_set(&opt, "max_delay", "550", 0);

    // ������
    int ret = avformat_open_input(&pFormat, strVideoPath.c_str(), nullptr, &opt);
    if (ret) {
        std::cout << "avformat_open_input failed" << std::endl;
        return;
    }

    std::cout << "avformat_open_input success" << std::endl;

    // ��������Ϣ
    ret = avformat_find_stream_info(pFormat, nullptr);
    if (ret) {
        std::cout << "avformat_find_stream_info failed" << std::endl;
        return;
    }

    std::cout << "avformat_find_stream_info success" << std::endl;

    //// ��ȡ��Ƶʱ��
    //auto sTime = pFormat->duration;
    //int sMin = static_cast<int>((sTime / 1000000) / 60);
    //int sSec = static_cast<int>((sTime / 1000000) % 60);

    //std::cout << "��Ƶ��ʱ����" << sMin << "��" << sSec << "��" << std::endl;
    // �����Ƶ��Ϣ
    av_dump_format(pFormat, 0, strVideoPath.c_str(), 0);

    std::cout << "***************************************************" << std::endl;

}

int main() {

    TestOpenLocalVideo();

    //TestNetworkVideo();

    return 0;
}