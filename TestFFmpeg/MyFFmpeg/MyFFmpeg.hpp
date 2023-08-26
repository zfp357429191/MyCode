#ifndef __MY_FFMPEG_HPP__
#define __MY_FFMPEG_HPP__

#include <string>

class MyFFmpeg {
public:
    MyFFmpeg();

    virtual ~MyFFmpeg();


    void PrintFFmpegInfo();

    /**
    ***************************************************************
    * @brief: 转换视频生成YUV
    ***************************************************************
    */
    void ConvertVideoToYUV(const std::string& strMediaPath, const std::string& strFileName);

    /**
    ***************************************************************
    * @brief:     设置输出路径
    * @param[in]: strPath   路径
    ***************************************************************
    */
    void SetOutputPath(const std::string& strPath);

    /**
    ***************************************************************
    * @brief:     利用SDL播放视频
    ***************************************************************
    */
    void PlayVideoBySDL(const std::string& strMediaPath);

private:
    /**
    ***************************************************************
    * @brief: 初始化注册（新版本无需）
    ***************************************************************
    */
    void InitFFmpeg();


private:
    std::string m_strOutputPath;
};


#endif