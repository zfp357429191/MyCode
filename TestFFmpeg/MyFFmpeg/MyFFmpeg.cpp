#include "MyFFmpeg.hpp"

#include <iostream>
#include <atomic>

// C方式引入FFmpeg头文件
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavutil/version.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"

#include "SDL.h"
}

#define USE_NEW_FFMPEG 1

//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

std::atomic_bool bThreadExit{false};
std::atomic_bool bThreadPause{false};

int SdlRefreshThread(void* pOpaque) {
    bThreadExit = false;
    bThreadPause = false;

    while (!bThreadExit) {
        if (!bThreadPause) {
            SDL_Event event;
            event.type = SFM_REFRESH_EVENT;
            SDL_PushEvent(&event);
        }
        SDL_Delay(40);  // 延迟40ms
    }

    bThreadPause = false;
    bThreadExit = false;

    SDL_Event event;
    event.type = SFM_BREAK_EVENT;
    SDL_PushEvent(&event);

    return 0;
}


//=========================================================
MyFFmpeg::MyFFmpeg() {

}

MyFFmpeg::~MyFFmpeg() {

}

/**
***************************************************************
* @brief: 初始化注册（新版本无需）
***************************************************************
*/
void MyFFmpeg::InitFFmpeg() {
    av_register_all();                  // 注册DLL
    avformat_network_init();            // 注册网络
}


void MyFFmpeg::PrintFFmpegInfo() {
#if USE_NEW_FFMPEG
    std::cout << "FFmpeg info: " << av_version_info() << std::endl;
#endif
}


/**
***************************************************************
* @brief:     设置输出路径
* @param[in]: strPath   路径
***************************************************************
*/
void MyFFmpeg::SetOutputPath(const std::string& strPath) {
    m_strOutputPath = strPath;
}

/**
***************************************************************
* @brief: 转换视频生成YUV
***************************************************************
*/
void MyFFmpeg::ConvertVideoToYUV(const std::string& strMediaPath, const std::string& strFileName) {
#if USE_NEW_FFMPEG

    if (strMediaPath.empty() || strFileName.empty()) {
        std::cout << "video file is empty..." << std::endl;
        return;
    }
    std::string strFileInPath = strMediaPath + strFileName;
    std::string strOutFilePath = m_strOutputPath + strFileName.substr(0, strFileName.find_first_of(".")) + ".yuv";
    std::cout << "video in file path : " << strFileInPath << std::endl;
    std::cout << "yuv out file path : " << strOutFilePath << std::endl;

    // 注册
    InitFFmpeg();

    AVFormatContext* pFormatCtx = avformat_alloc_context();
    // 打开输入
    int nRet = avformat_open_input(&pFormatCtx, strFileInPath.c_str(), nullptr, nullptr);
    if (nRet != 0) {
        std::cout << "avformat_open_input failed..." << std::endl;
        return;
    }
    std::cout << "avformat_open_input success..." << std::endl;

    // 查找流信息
    nRet = avformat_find_stream_info(pFormatCtx, nullptr);
    if (nRet < 0) {
        std::cout << "avformat_find_stream_info failed..." << std::endl;
        return;
    }
    std::cout << "avformat_find_stream_info success..." << std::endl;

    // 获取视频时长、格式
    auto nTimes = pFormatCtx->duration / 1000000;   // 总时长（微秒->毫秒）
    auto nTimeH = nTimes / 3600;                    // 小时
    auto nTimeM = (nTimes % 3600) / 60;             // 分钟(抛开小时之后的)
    auto nTimeS = nTimes % 60;                      // 秒
    std::string strH = nTimeH > 10 ? std::to_string(nTimeH) : "0" + std::to_string(nTimeH);
    std::string strM = nTimeM > 10 ? std::to_string(nTimeM) : "0" + std::to_string(nTimeM);
    std::string strS = nTimeS > 10 ? std::to_string(nTimeS) : "0" + std::to_string(nTimeS);
    std::cout << "视频时长："<< strH << " : " << strM << " : " << strS << std::endl;
    std::cout << "视频格式：" << pFormatCtx->iformat->long_name << std::endl;
    std::cout << "*********************************************************" << std::endl;
    av_dump_format(pFormatCtx, 0, strFileInPath.c_str(), 0);
    std::cout << "*********************************************************" << std::endl;

    // 查找流
    int nVideoStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, NULL);

    AVCodecContext* pAVCodecCtx = pFormatCtx->streams[nVideoStream]->codec;
    if (!pAVCodecCtx) {
        std::cout << "get avcodecContext is nullptr..." << std::endl;
        return;
    }
    std::cout << "get avcodecContext is success..." << std::endl;

    // 查找解码器
    AVCodec* pAVCodec = avcodec_find_decoder(pAVCodecCtx->codec_id);
    if (!pAVCodec) {
        std::cout << "avcodec_find_decoder failed..." << std::endl;
        return;
    }
    std::cout << "avcodec_find_decoder success..." << std::endl;

    nRet = avcodec_open2(pAVCodecCtx, pAVCodec, NULL);
    if (nRet != 0) {
        std::cout << "avcodec_open2 failed..." << std::endl;
        return;
    }
    std::cout << "avcodec_open2 success..." << std::endl;

    // 输出yuv文件
    FILE* fYUV = fopen(strOutFilePath.c_str(), "wb+");

    // 开始解码视频，申请空间
    AVFrame* pFrame = av_frame_alloc();             // 指向原始帧数据
    AVFrame* pFrameYUV = av_frame_alloc();          // 指向转换后的YUV数据帧
    int nWidth = pAVCodecCtx->width;
    int nHeight = pAVCodecCtx->height;
    AVPixelFormat srcPixFmt = pAVCodecCtx->pix_fmt;
    AVPixelFormat destPixFmt = AV_PIX_FMT_YUV420P;
    std::cout << "video size: " << nWidth << "*" << nHeight << std::endl;

    // 分配空间，进行图像转换
    int nSize = avpicture_get_size(destPixFmt, nWidth, nHeight);
    uint8_t* pBuff = (uint8_t*)av_malloc(nSize);

    // 填充一帧图像
    avpicture_fill((AVPicture*)pFrameYUV, pBuff, destPixFmt, nWidth, nHeight);

    AVPacket* pPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
    
    // 转换上下文
    SwsContext* pSwsCtx = sws_getContext(nWidth,
                                         nHeight,
                                         srcPixFmt,
                                         nWidth,
                                         nHeight,
                                         destPixFmt,
                                         SWS_BICUBIC,
                                         nullptr,
                                         nullptr, 
                                         nullptr);

    // 读帧
    int nGot = 0;
    int nFrameCount = 0;
    while (av_read_frame(pFormatCtx, pPacket) >= 0) {
        // 判断流下标
        if (pPacket->stream_index == AVMEDIA_TYPE_VIDEO) {
            nRet = avcodec_decode_video2(pAVCodecCtx, pFrame, &nGot, pPacket);
            if (nRet < 0) {
                std::cout << "avcodec_decode_video2 failed..." << std::endl;
                return;
            }

            if (nGot) {
                sws_scale(pSwsCtx,
                          pFrame->data,
                          pFrame->linesize,
                          0,
                          nHeight,
                          pFrameYUV->data,
                          pFrameYUV->linesize);
                // 写入数据到yuv
                fwrite(pFrameYUV->data[0], 1, nWidth * nHeight, fYUV);        // Y分量
                fwrite(pFrameYUV->data[1], 1, nWidth* nHeight / 4, fYUV);     // U分量
                fwrite(pFrameYUV->data[2], 1, nWidth* nHeight / 4, fYUV);     // V分量

                std::cout << "frame write success [" << nFrameCount++ << "]" << std::endl;
            } else {
                std::cout << "frame write failed [" << nFrameCount++ << "]" << std::endl;
            }
        }
        av_free_packet(pPacket);
    }

    // 关闭文件
    fclose(fYUV);

    // 释放资源
    av_free(pBuff);
    sws_freeContext(pSwsCtx);
    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avformat_close_input(&pFormatCtx);

#endif
}

/**
***************************************************************
* @brief:     利用SDL播放视频
***************************************************************
*/
void MyFFmpeg::PlayVideoBySDL(const std::string& strMediaPath) {
#if USE_NEW_FFMPEG
    if (strMediaPath.empty()) {
        std::cout << "video file is empty..." << std::endl;
        return;
    }
    std::cout << "video in file path : " << strMediaPath << std::endl;

    // 注册
    InitFFmpeg();

    AVFormatContext* pFormatCtx = avformat_alloc_context();
    // 打开输入
    int nRet = avformat_open_input(&pFormatCtx, strMediaPath.c_str(), nullptr, nullptr);
    if (nRet != 0) {
        std::cout << "avformat_open_input failed..." << std::endl;
        return;
    }
    std::cout << "avformat_open_input success..." << std::endl;

    // 查找流信息
    nRet = avformat_find_stream_info(pFormatCtx, nullptr);
    if (nRet < 0) {
        std::cout << "avformat_find_stream_info failed..." << std::endl;
        return;
    }
    std::cout << "avformat_find_stream_info success..." << std::endl;

    // 查找流
    int nVideoStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, NULL);

    AVCodecContext* pAVCodecCtx = pFormatCtx->streams[nVideoStream]->codec;
    if (!pAVCodecCtx) {
        std::cout << "get avcodecContext is nullptr..." << std::endl;
        return;
    }
    std::cout << "get avcodecContext is success..." << std::endl;

    // 查找解码器
    AVCodec* pAVCodec = avcodec_find_decoder(pAVCodecCtx->codec_id);
    if (!pAVCodec) {
        std::cout << "avcodec_find_decoder failed..." << std::endl;
        return;
    }
    std::cout << "avcodec_find_decoder success..." << std::endl;

    nRet = avcodec_open2(pAVCodecCtx, pAVCodec, NULL);
    if (nRet != 0) {
        std::cout << "avcodec_open2 failed..." << std::endl;
        return;
    }
    std::cout << "avcodec_open2 success..." << std::endl;

    // 开始解码视频，申请空间
    AVFrame* pFrame = av_frame_alloc();             // 指向原始帧数据
    AVFrame* pFrameYUV = av_frame_alloc();          // 指向转换后的YUV数据帧
    int nWidth = pAVCodecCtx->width;
    int nHeight = pAVCodecCtx->height;
    AVPixelFormat srcPixFmt = pAVCodecCtx->pix_fmt;
    AVPixelFormat destPixFmt = AV_PIX_FMT_YUV420P;
    std::cout << "video size: " << nWidth << "*" << nHeight << std::endl;

    // 分配空间，进行图像转换
    int nSize = avpicture_get_size(destPixFmt, nWidth, nHeight);
    uint8_t* pBuff = (uint8_t*)av_malloc(nSize);

    // 填充一帧图像
    avpicture_fill((AVPicture*)pFrameYUV, pBuff, destPixFmt, nWidth, nHeight);

    AVPacket* pPacket = (AVPacket*)av_malloc(sizeof(AVPacket));

    // 转换上下文
    SwsContext* pSwsCtx = sws_getContext(nWidth,
                                         nHeight,
                                         srcPixFmt,
                                         nWidth,
                                         nHeight,
                                         destPixFmt,
                                         SWS_BICUBIC,
                                         nullptr,
                                         nullptr,
                                         nullptr);

    // 初始化SDL
    nRet = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
    if (nRet) {
        std::cout << "SDL init falied..." << std::endl;
        return;
    }

    int nPlayerWidth = pAVCodecCtx->width;
    int nPlayerHeight = pAVCodecCtx->height;

    // 构建SDL窗口
    SDL_Window* pPlayerWindow = SDL_CreateWindow("My Test Player Window",
                                                 SDL_WINDOWPOS_UNDEFINED,
                                                 SDL_WINDOWPOS_UNDEFINED,
                                                 nPlayerWidth,
                                                 nPlayerHeight,
                                                 SDL_WINDOW_OPENGL);

    if (!pPlayerWindow) {
        std::cout << "Create SDL Window Failed :" << SDL_GetError() << std::endl;
        return;
    }

    // 构建渲染器
    SDL_Renderer* pPlayerRender = SDL_CreateRenderer(pPlayerWindow,
                                                     -1,
                                                     0);
    if (!pPlayerRender) {
        std::cout << "Create SDL Render Failed : " << SDL_GetError() << std::endl;
        return;
    }

    // 构建纹理
    SDL_Texture* pPlayerTexture = SDL_CreateTexture(pPlayerRender,
                                                    SDL_PIXELFORMAT_IYUV,
                                                    SDL_TEXTUREACCESS_STREAMING,
                                                    pAVCodecCtx->width,
                                                    pAVCodecCtx->height);
    if (!pPlayerTexture) {
        std::cout << "Create SDL Texture Failed : " << SDL_GetError() << std::endl;
        return;
    }

    // 显示区域
    SDL_Rect playerRect;
    playerRect.x = 0;
    playerRect.y = 0;
    playerRect.w = nPlayerWidth;
    playerRect.h = nPlayerHeight;

    // 渲染线程
    SDL_Thread* pPlayerThread = SDL_CreateThread(SdlRefreshThread, NULL, NULL);


    // 事件循环开始读帧
    SDL_Event event;
    int nGot;
    for (;;) {
        // Wait
        SDL_WaitEvent(&event);
        if (event.type == SFM_REFRESH_EVENT) {
            // 读帧
            while (1) {
                if (av_read_frame(pFormatCtx, pPacket) < 0) {
                    bThreadExit = true;
                }
                if (pPacket->stream_index == AVMEDIA_TYPE_VIDEO) {
                    // 读到视频帧，送去解码
                    break;
                }
            }

            // 开始解码
            nRet = avcodec_decode_video2(pAVCodecCtx, pFrame, &nGot, pPacket);
            if (nRet < 0) {
                std::cout << "avcodec_decode_video2 failed..." << std::endl;
                return;
            }
            if (nGot) {
                // 数据转换
                sws_scale(pSwsCtx,
                          pFrame->data,
                          pFrame->linesize,
                          0,
                          nHeight,
                          pFrameYUV->data,
                          pFrameYUV->linesize);

                // SDL处理
                SDL_UpdateTexture(pPlayerTexture,
                                  NULL,
                                  pFrameYUV->data[0],
                                  pFrameYUV->linesize[0]);
                SDL_RenderClear(pPlayerRender);
                SDL_RenderCopy(pPlayerRender,
                               pPlayerTexture,
                               /*&playerRect*/NULL,
                               /*&playerRect*/NULL);
                SDL_RenderPresent(pPlayerRender);
            }
            av_free_packet(pPacket);
        } else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_SPACE) {
                // 暂停
                bThreadPause = !bThreadPause;
            }
        } else if (event.type == SDL_QUIT) {
            bThreadExit = true;
        } else if (event.type == SFM_BREAK_EVENT) {
            break;
        }
    }

    // 释放资源
    sws_freeContext(pSwsCtx);
    SDL_Quit();

    av_free(pBuff);
    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_close(pAVCodecCtx);
    avformat_close_input(&pFormatCtx);

#endif
}