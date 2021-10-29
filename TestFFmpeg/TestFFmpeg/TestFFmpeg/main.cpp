#include <iostream>
#include <stdio.h>

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavfilter/avfilter.h"

#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avutil.h"


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
    
#if 0 // 旧版写法
    AVCodecContext* pCodecContext = pFormat->streams[nVideoStream]->codec;

#else // 新版写法
    AVCodecContext* pCodecContext = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(pCodecContext, pFormat->streams[nVideoStream]->codecpar);
#endif
    
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
    AVPixelFormat pixFmt = pCodecContext->pix_fmt;

    // 分配空间 进行图像转换
#if 0 // 旧版写法
    int nSize = avpicture_get_size(AV_PIX_FMT_YUV420P, nWidth, nHeight);
#else // 新版写法
    int nSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, nWidth, nHeight, 1);
#endif
    uint8_t* pBuff = (uint8_t*)av_malloc(nSize);

    // 一帧图像
#if 0 // 旧版本
    avpicture_fill((AVPicture*)pFrameYUV, pBuff, AV_PIX_FMT_YUV420P, nWidth, nHeight);
#else // 新版写法
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, pBuff, AV_PIX_FMT_YUV420P, nWidth, nHeight, 1);
#endif

    AVPacket* pPacket = (AVPacket*)av_malloc(sizeof(AVPacket*));

    // 转换上下文
    SwsContext* pswsContext =  sws_getContext(nWidth, nHeight, pixFmt, nWidth, nHeight,
                                              AV_PIX_FMT_YUV420P, SWS_BICUBIC, 
                                              nullptr, nullptr, nullptr);

    // 读帧
    int go = 0;
    int nFrameCount = 0;
    while (av_read_frame(pFormat, pPacket) >= 0) {
        // 判断stream_index
        if (AVMEDIA_TYPE_VIDEO == pPacket->stream_index) {
#if 0 // 旧版本
            ret = if (avcodec_decode_video2(pCodecContext, pFrame, &go, pPacket)) {
#else
            ret = avcodec_send_packet(pCodecContext, pPacket);
            ret = (avcodec_send_packet(pCodecContext, pPacket) && (go = avcodec_receive_frame(pCodecContext, pFrame)));
#endif
            if (ret) {
                std::cout << "avcodec_decode_video2 failed" << std::endl;
                return;
            }

            if (go) {
                sws_scale(pswsContext, (uint8_t**)pFrame->data, pFrame->linesize, 
                          0, nHeight, pFrameYUV->data, pFrameYUV->linesize);
                ++nFrameCount;
                std::cout << "frame index : " << nFrameCount << std::endl;
            }
        }
        //av_free_packet(pPacket);
    }
    sws_freeContext(pswsContext);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameYUV);

    // TODO


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

/*----------------------------------------------------------------
 * @brief: 水平翻转视频（镜像）
 *---------------------------------------------------------------*/
int TestFlipVideo() {

    int ret;
    AVFrame* frame_in;
    AVFrame* frame_out;
    unsigned char* frame_buffer_in;
    unsigned char* frame_buffer_out;

    AVFilterContext* buffersink_ctx;
    AVFilterContext* buffersrc_ctx;
    AVFilterGraph* filter_graph;
    //static int video_stream_index = -1;

    //Input YUV
    FILE* fp_in = fopen("../../MediaSource/t1002.yuv", "rb+");
    if (fp_in == NULL) {
        printf("Error open input file.\n");
        return -1;
    }
    int in_width = 320;
    int in_height = 640;

    //Output YUV
    FILE* fp_out = fopen("../../MediaSource/t5001.yuv", "wb+");
    if (fp_out == NULL) {
        printf("Error open output file.\n");
        return -1;
    }

    //const char *filter_descr = "lutyuv='u=128:v=128'";
    //const char* filter_descr = "boxblur";
    const char *filter_descr = "hflip";
    //const char *filter_descr = "hue='h=60:s=-3'";
    //const char *filter_descr = "crop=2/3*in_w:2/3*in_h";
    //const char *filter_descr = "drawbox=x=100:y=100:w=100:h=100:color=pink@0.5";
    //const char *filter_descr = "drawtext=fontfile=arial.ttf:fontcolor=green:fontsize=30:text='Lei Xiaohua'";

    //avfilter_register_all();

    char args[512];
    const AVFilter* buffersrc = avfilter_get_by_name("buffer");
    //const AVFilter* buffersink = avfilter_get_by_name("ffbuffersink");
    const AVFilter* buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut* outputs = avfilter_inout_alloc();
    AVFilterInOut* inputs = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    AVBufferSinkParams* buffersink_params;

    filter_graph = avfilter_graph_alloc();

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
        "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
        in_width, in_height, AV_PIX_FMT_YUV420P,
        1, 25, 1, 1);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
    if (ret < 0) {
        printf("Cannot create buffer source\n");
        return ret;
    }

    /* buffer video sink: to terminate the filter chain. */
    buffersink_params = av_buffersink_params_alloc();
    buffersink_params->pixel_fmts = pix_fmts;
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, buffersink_params, filter_graph);
    av_free(buffersink_params);
    if (ret < 0) {
        printf("Cannot create buffer sink\n");
        return ret;
    }

    /* Endpoints for the filter graph. */
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_descr, &inputs, &outputs, NULL)) < 0)
        return ret;

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        return ret;

    frame_in = av_frame_alloc();
    frame_buffer_in = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, in_width, in_height, 1));
    av_image_fill_arrays(frame_in->data, frame_in->linesize, frame_buffer_in, AV_PIX_FMT_YUV420P, in_width, in_height, 1);

    frame_out = av_frame_alloc();
    frame_buffer_out = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, in_width, in_height, 1));
    av_image_fill_arrays(frame_out->data, frame_out->linesize, frame_buffer_out, AV_PIX_FMT_YUV420P, in_width, in_height, 1);

    frame_in->width = in_width;
    frame_in->height = in_height;
    frame_in->format = AV_PIX_FMT_YUV420P;

    while (1) {

        if (fread(frame_buffer_in, 1, in_width * in_height * 3 / 2, fp_in) != in_width * in_height * 3 / 2) {
            break;
        }
        //input Y,U,V
        frame_in->data[0] = frame_buffer_in;
        frame_in->data[1] = frame_buffer_in + in_width * in_height;
        frame_in->data[2] = frame_buffer_in + in_width * in_height * 5 / 4;

        if (av_buffersrc_add_frame(buffersrc_ctx, frame_in) < 0) {
            printf("Error while add frame.\n");
            break;
        }

        /* pull filtered pictures from the filtergraph */
        ret = av_buffersink_get_frame(buffersink_ctx, frame_out);
        if (ret < 0)
            break;

        //output Y,U,V
        if (frame_out->format == AV_PIX_FMT_YUV420P) {
            for (int i = 0; i < frame_out->height; i++) {
                fwrite(frame_out->data[0] + frame_out->linesize[0] * i, 1, frame_out->width, fp_out);
            }
            for (int i = 0; i < frame_out->height / 2; i++) {
                fwrite(frame_out->data[1] + frame_out->linesize[1] * i, 1, frame_out->width / 2, fp_out);
            }
            for (int i = 0; i < frame_out->height / 2; i++) {
                fwrite(frame_out->data[2] + frame_out->linesize[2] * i, 1, frame_out->width / 2, fp_out);
            }
        }
        printf("Process 1 frame!\n");
        av_frame_unref(frame_out);
    }

    fclose(fp_in);
    fclose(fp_out);

    av_frame_free(&frame_in);
    av_frame_free(&frame_out);
    avfilter_graph_free(&filter_graph);

    return 0;
}

int TestFlipVideo2() {

    int ret = 0;

    // input yuv
    FILE* inFile = NULL;
    const char* inFileName = "../../MediaSource/t1002.yuv";
    fopen_s(&inFile, inFileName, "rb+");
    if (!inFile) {
        printf("Fail to open file\n");
        return -1;
    }

    int in_width = 320;
    int in_height = 640;

    // output yuv
    FILE* outFile = NULL;
    const char* outFileName = "../../MediaSource/hflip_output.yuv";
    fopen_s(&outFile, outFileName, "wb");
    if (!outFile) {
        printf("Fail to create file for output\n");
        return -1;
    }

    //avfilter_register_all();

    AVFilterGraph* filter_graph = avfilter_graph_alloc();
    if (!filter_graph) {
        printf("Fail to create filter graph!\n");
        return -1;
    }

    // source filter
    char args[512];
    _snprintf_s(args, sizeof(args),
        "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
        in_width, in_height, AV_PIX_FMT_YUV420P,
        1, 25, 1, 1);
    const AVFilter* bufferSrc = avfilter_get_by_name("buffer");
    AVFilterContext* bufferSrc_ctx;
    ret = avfilter_graph_create_filter(&bufferSrc_ctx, bufferSrc, "in", args, NULL, filter_graph);
    if (ret < 0) {
        printf("Fail to create filter bufferSrc\n");
        return -1;
    }

    // sink filter
    AVBufferSinkParams* bufferSink_params;
    AVFilterContext* bufferSink_ctx;
    const AVFilter* bufferSink = avfilter_get_by_name("buffersink");
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    bufferSink_params = av_buffersink_params_alloc();
    bufferSink_params->pixel_fmts = pix_fmts;
    ret = avfilter_graph_create_filter(&bufferSink_ctx, bufferSink, "out", NULL, bufferSink_params, filter_graph);
    if (ret < 0) {
        printf("Fail to create filter sink filter\n");
        return -1;
    }

    //// split filter
    //const const AVFilter* splitFilter = avfilter_get_by_name("split");
    //AVFilterContext* splitFilter_ctx;
    //ret = avfilter_graph_create_filter(&splitFilter_ctx, splitFilter, "split", "outputs=2", NULL, filter_graph);
    //if (ret < 0) {
    //    printf("Fail to create split filter\n");
    //    return -1;
    //}

    //// crop filter
    //const AVFilter* cropFilter = avfilter_get_by_name("crop");
    //AVFilterContext* cropFilter_ctx;
    //ret = avfilter_graph_create_filter(&cropFilter_ctx, cropFilter, "crop", "out_w=iw:out_h=ih/2:x=0:y=0", NULL, filter_graph);
    //if (ret < 0) {
    //    printf("Fail to create crop filter\n");
    //    return -1;
    //}

    // vflip filter
    const AVFilter* vflipFilter = avfilter_get_by_name("vflip");
    AVFilterContext* vflipFilter_ctx;
    ret = avfilter_graph_create_filter(&vflipFilter_ctx, vflipFilter, "vflip", NULL, NULL, filter_graph);
    if (ret < 0) {
        printf("Fail to create vflip filter\n");
        return -1;
    }

    //// overlay filter
    //const AVFilter* overlayFilter = avfilter_get_by_name("overlay");
    //AVFilterContext* overlayFilter_ctx;
    //ret = avfilter_graph_create_filter(&overlayFilter_ctx, overlayFilter, "overlay", "y=0:H/2", NULL, filter_graph);
    //if (ret < 0) {
    //    printf("Fail to create overlay filter\n");
    //    return -1;
    //}

    //// src filter to split filter
    //ret = avfilter_link(bufferSrc_ctx, 0, splitFilter_ctx, 0);
    //if (ret != 0) {
    //    printf("Fail to link src filter and split filter\n");
    //    return -1;
    //}
    //// split filter's first pad to overlay filter's main pad
    //ret = avfilter_link(splitFilter_ctx, 0, overlayFilter_ctx, 0);
    //if (ret != 0) {
    //    printf("Fail to link split filter and overlay filter main pad\n");
    //    return -1;
    //}
    //// split filter's second pad to crop filter
    //ret = avfilter_link(splitFilter_ctx, 1, cropFilter_ctx, 0);
    //if (ret != 0) {
    //    printf("Fail to link split filter's second pad and crop filter\n");
    //    return -1;
    //}
    //// crop filter to vflip filter
    //ret = avfilter_link(cropFilter_ctx, 0, vflipFilter_ctx, 0);
    //if (ret != 0) {
    //    printf("Fail to link crop filter and vflip filter\n");
    //    return -1;
    //}
    //// vflip filter to overlay filter's second pad
    //ret = avfilter_link(vflipFilter_ctx, 0, overlayFilter_ctx, 1);
    //if (ret != 0) {
    //    printf("Fail to link vflip filter and overlay filter's second pad\n");
    //    return -1;
    //}
    //// overlay filter to sink filter
    //ret = avfilter_link(overlayFilter_ctx, 0, bufferSink_ctx, 0);
    //if (ret != 0) {
    //    printf("Fail to link overlay filter and sink filter\n");
    //    return -1;
    //}

    //// check filter graph
    //ret = avfilter_graph_config(filter_graph, NULL);
    //if (ret < 0) {
    //    printf("Fail in filter graph\n");
    //    return -1;
    //}

    char* graph_str = avfilter_graph_dump(filter_graph, NULL);
    FILE* graphFile = NULL;
    fopen_s(&graphFile, "graphFile.txt", "w");
    fprintf(graphFile, "%s", graph_str);
    av_free(graph_str);

    AVFrame* frame_in = av_frame_alloc();
    unsigned char* frame_buffer_in = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, in_width, in_height, 1));
    av_image_fill_arrays(frame_in->data, frame_in->linesize, frame_buffer_in,
        AV_PIX_FMT_YUV420P, in_width, in_height, 1);

    AVFrame* frame_out = av_frame_alloc();
    unsigned char* frame_buffer_out = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, in_width, in_height, 1));
    av_image_fill_arrays(frame_out->data, frame_out->linesize, frame_buffer_out,
        AV_PIX_FMT_YUV420P, in_width, in_height, 1);

    frame_in->width = in_width;
    frame_in->height = in_height;
    frame_in->format = AV_PIX_FMT_YUV420P;

    while (1) {

        if (fread(frame_buffer_in, 1, in_width * in_height * 3 / 2, inFile) != in_width * in_height * 3 / 2) {
            break;
        }
        //input Y,U,V
        frame_in->data[0] = frame_buffer_in;
        frame_in->data[1] = frame_buffer_in + in_width * in_height;
        frame_in->data[2] = frame_buffer_in + in_width * in_height * 5 / 4;

        if (av_buffersrc_add_frame(bufferSrc_ctx, frame_in) < 0) {
            printf("Error while add frame.\n");
            break;
        }

        /* pull filtered pictures from the filtergraph */
        ret = av_buffersink_get_frame(bufferSink_ctx, frame_out);
        if (ret < 0)
            break;

        //output Y,U,V
        if (frame_out->format == AV_PIX_FMT_YUV420P) {
            for (int i = 0; i < frame_out->height; i++) {
                fwrite(frame_out->data[0] + frame_out->linesize[0] * i, 1, frame_out->width, outFile);
            }
            for (int i = 0; i < frame_out->height / 2; i++) {
                fwrite(frame_out->data[1] + frame_out->linesize[1] * i, 1, frame_out->width / 2, outFile);
            }
            for (int i = 0; i < frame_out->height / 2; i++) {
                fwrite(frame_out->data[2] + frame_out->linesize[2] * i, 1, frame_out->width / 2, outFile);
            }
        }
        printf("Process 1 frame!\n");
        av_frame_unref(frame_out);
    }

    fclose(inFile);
    fclose(outFile);

    av_frame_free(&frame_in);
    av_frame_free(&frame_out);
    avfilter_graph_free(&filter_graph);
    return 0;

    //avpicture_fill()
    //avpicture_get_size()
    //sws_getContext()

}

int main() {

    TestOpenLocalVideo();

    //TestNetworkVideo();

    //TestFlipVideo();

    //TestFlipVideo2();

    return 0;
}