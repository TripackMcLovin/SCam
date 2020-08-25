#ifndef SOURCEVIDEO_H
#define SOURCEVIDEO_H

#include <QObject>
#include <QImage>
#include "scam.h"

#include <cstring>

#include <QImage>
#include <QRgb>
#include <opencv2/imgproc.hpp>
#include <QFile>
#include <QTime>

//You need to have ffmpeg-devel packages installed!!!
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <ffmpeg/libavformat/avformat.h>
#include <ffmpeg/libswscale/swscale.h>
//#include <libavutil/pixfmt.h>
#ifdef __cplusplus
}
#endif

//class SourceVideo;

/*
struct ImageData
{
  QVector<unsigned char> raw;
  QVector<unsigned char> picture;

  SourceVideo *owner;

  ImageData(int bpp, int w, int h)
  {
    raw.resize(bpp*w*h);
    picture.resize(bpp*w*h);
  }
};
*/


class SCamVideo : public SCam
{
    //Q_OBJECT
public:
    explicit SCamVideo(QString fname, QObject *parent = nullptr);
    ~SCamVideo() override;

    //QString getID() override;

    QImage *getNewestImage() override;
    //QString getFrameID() override;

    bool open_internal() override;
    bool activate_internal() override;

    //bool isReady() override; //==ACTIVE, ready to capture

    bool deactivate_internal() override;
    bool close_internal() override;

    //double getFPS() override;

    bool capture_internal() override;

private:
    QString m_filename;
    //cv::VideoCapture *m_capture;
    double m_fileFramerate;

    //double m_frameTimeStamp;
    //better:
    QTime m_timeStamp;

    //int m_width;
    //int m_height;

    //int it;

    /*
    libvlc_instance_t *m_vlcInstance;
    libvlc_media_player_t *m_vlcPlayer;
    ImageData m_imgDat;
    */

    AVFormatContext *m_formatCtx;
    AVCodecContext *m_codecCtxOrig;
    AVCodecContext *m_codecCtx;
    AVCodec *m_codec;
    AVFrame *m_frame;
    AVFrame *m_frameRGB;
    int m_videoStream;
    uint8_t *m_buffer;
    int m_numBytes;

public:
    //void fmtCallback(int w, int h, int bpp, int planes);

signals:
    //void newImage() override;

public slots:

};

#endif // SOURCEVIDEO_H
