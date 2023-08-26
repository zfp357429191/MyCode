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
    * @brief: 打开本地视频文件
    ***************************************************************
    */
    void OpenLocalVideo(const std::string& strFilePath, const std::string& strFileName);

    /**
    ***************************************************************
    * @brief:     设置输出路径
    * @param[in]: strPath   路径
    ***************************************************************
    */
    void SetOutputPath(const std::string& strPath);

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