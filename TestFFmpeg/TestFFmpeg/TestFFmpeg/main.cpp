#include <iostream>

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

}

void TestCode1() {

    AVFormatContext* pFormat = nullptr;
    std::string strVideoPath = "../../MediaSource/t1001.mp4";

    // ������
    int ret = avformat_open_input(&pFormat, strVideoPath.c_str(), nullptr, nullptr);
    if (ret) {
        std::cout << "avformat_open_input failed" << std::endl;
    }

    // ��������Ϣ
    ret = avformat_find_stream_info(pFormat, nullptr);
    if (ret) {
        std::cout << "avformat_find_stream_info failed" << std::endl;
    }

    // ��ȡ��Ƶʱ��
    auto sTime = pFormat->duration;
    int sMin = static_cast<int>((sTime / 1000000) / 60);
    int sSec = static_cast<int>((sTime / 1000000) % 60);

    std::cout << "��Ƶ��ʱ����" << sMin << "��" << sSec << "��" << std::endl;
    // �����Ƶ��Ϣ
    av_dump_format(pFormat, 0, strVideoPath.c_str(), 0);




}

int main() {

    TestCode1();

    return 0;
}