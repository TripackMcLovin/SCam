#include "ffmpeg.h"

/*
namespace {

    void* lock_frame(void *opaque, void **planes)
    {
      ImageData* buf = (ImageData*)(opaque);
      *planes = buf->raw.data();
      return buf->picture.data();
    }

    void unlock_frame(void *opaque, void *picture, void *const *planes)
    {
      ImageData* buf = (ImageData*)(opaque);
      // will be logic to announce new image


    }

    unsigned format_callback(void** opaque, char* chroma, unsigned *width, unsigned *height, unsigned *pitches, unsigned *lines)
    {
      ImageData* buf = (ImageData*)(opaque);


      buf->owner->fmtCallback(*width,*height,3,0);

      qDebug() << "chroma:" << QString(chroma) << "width:" << *width << ", height:" << *height;

      *pitches= (*width) * 3;
      *lines = *height;

      return 1;
    }

} // namespace
*/


SCamVideo::SCamVideo(QString dir, QString fname, QObject *parent) :
        SCam(fname,//readable name
             dir.append("/").append(fname),//internal name
             2, parent),
    m_filename(internalName), //which is basically the the internal name
    m_fileFramerate(1.0),
    m_timeStamp(),
    m_formatCtx(Q_NULLPTR),
    m_codecCtxOrig(Q_NULLPTR),
    m_codecCtx(Q_NULLPTR),
    m_codec(Q_NULLPTR),
    m_frame(Q_NULLPTR),
    m_frameRGB(Q_NULLPTR),
    m_videoStream(-1),
    m_buffer(Q_NULLPTR),
    m_numBytes(-1)
    //m_imgDat(3,0,0)
    //m_capture(Q_NULLPTR)
{
    //m_imgDat.owner=this;

    if (QFile(m_filename).exists()) {
        setState(SCam::CLOSED);
        qInfo() << "SCamVideo for"<<m_filename<<" created!";
    } else {
        setState(SCam::ERROR);
        qInfo() << "failed to create SCamVideo for "<<m_filename<<"!";
    }
}

SCamVideo::~SCamVideo()
{

}

/*
QString SCamVideo::getID()
{
    return m_filename;
}
*/

//this one is already done in scam, nevertheless, this might be a different kind of
//buffer around to choose from
/*
QImage *SCamVideo::getNewestImage()
{
    //qInfo() << "getLastImage:videosource" << m_index;
    return m_images.at(m_index);
}
*/

/*
QString SCamVideo::getFrameID()
{
    //frameTimestamp is in ms, we need to reformat it
    return m_timeStamp.toString("hh:mm:ss.zzz");

}
*/

bool SCamVideo::open_internal()
{
    qInfo() << "SCamVideo::open()for "<< m_filename;

    //assert(m_capture==Q_NULLPTR);
    //want to make sure the capture object is created in the new thread

    if (m_status==CLOSED){

        assert(QFile(m_filename).exists());

        const char *fname = m_filename.toLatin1();

        av_register_all();

        int ret=avformat_open_input(&m_formatCtx, fname, Q_NULLPTR, Q_NULLPTR);

        if (ret){
            char cerr[64];
            av_strerror(ret, cerr, 64);
            qInfo() << "failed to open video - avformat_open_input() returned "<< ret << cerr;
            return false;
        }

        ret = avformat_find_stream_info(m_formatCtx, Q_NULLPTR);
        if (ret<0){
            char cerr[64];
            av_strerror(ret, cerr, 64);
            qInfo() << "failed to get stream information"<< ret << cerr;
            return false;
        }

        //we dont need all the rest of register all() anymore
        av_dump_format(m_formatCtx, 0, m_filename.toStdString().c_str(), 0);

        //Now m_formatCtx->streams is just an array of pointers, of size m_formatCtx->nb_streams,
        //so let's walk through it until we find a video stream.
        int i;
        m_codecCtxOrig = Q_NULLPTR;
        m_codecCtx = Q_NULLPTR;

        // Find the first video stream (there could be also an audio as also another video stream)
        m_videoStream=-1;
        for(i=0; i<m_formatCtx->nb_streams; i++)
          if(m_formatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            m_videoStream=i;
            break;
          }

        if(m_videoStream==-1){
          qInfo() << "Didn't find a video stream";
          return false; // Didn't find a video stream
        }

        // Get a pointer to the codec context for the video stream
        m_codecCtx=m_formatCtx->streams[m_videoStream]->codec;

        AVRational frr=m_codecCtx->framerate;

        m_fileFramerate = frr.den==0?0.0: static_cast<double>(frr.num)/frr.den;

        qInfo() << "m_fileFramerate="<<m_fileFramerate;

        //The stream's information about the codec is in what we call the "codec context."
        //This contains all the information about the codec that the stream is using, and now we have a pointer to it.
        //But we still have to find the actual codec and open it:

        // Find the decoder for the video stream
        m_codec=avcodec_find_decoder(m_codecCtx->codec_id);
        if(m_codec==Q_NULLPTR) {
          qInfo() << "Codec not found or unsupported codec!";
          return false;
        }

        // Copy context
        /*
        m_codecCtxOrig = avcodec_alloc_context3(m_codec);
        if(avcodec_copy_context(m_codecCtx, m_codecCtxOrig) != 0) {
          qInfo() << "Couldn't copy codec context";
          return false; // Error copying codec context
        }
        */

        // Open codec
        if(avcodec_open2(m_codecCtx, m_codec, Q_NULLPTR)<0) {
          qInfo() << "Couldn't open codec";
          return false;
        }

        //Note that we must not use the AVCodecContext from the video stream directly!
        //So we have to use avcodec_copy_context() to copy the context to a new location (after allocating memory for it, of course).

        // Allocate video frame

        m_frame=av_frame_alloc();
        if (m_frame==Q_NULLPTR){
            qInfo() << "failed to allocate frame";
            return false;
        }


        // Allocate an AVFrame structure
        m_frameRGB=av_frame_alloc();
        if(m_frameRGB==Q_NULLPTR){
            qInfo() << "failed to allocate RGB frame";
            return false;
        }


        m_imageSize.setWidth( m_codecCtx->width );
        m_imageSize.setHeight( m_codecCtx->height );
                    //m_imageSize.setHeight( static_cast<int>(m_capture->get(CV_CAP_PROP_FRAME_HEIGHT)));

        m_numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, m_codecCtx->width, m_codecCtx->height);
        m_buffer = (uint8_t*)av_malloc(m_numBytes*sizeof (uint8_t));

        for (int i=0;i<m_numBuf; i++){
            m_imgProcessors.append( new ImgProc(m_buffer,
                                         m_imageSize.width(),
                                         m_imageSize.height(),
                                         QImage::Format_RGB888) );
        }



        avpicture_fill((AVPicture *)m_frameRGB, m_buffer, AV_PIX_FMT_RGB24,
                        m_codecCtx->width, m_codecCtx->height);


        m_timeStamp.setHMS(0,0,0);

        setState(OPEN);
        return true;
    } else {
        qInfo() << "failed to open video";

        return false;
    }
}

bool SCamVideo::activate_internal()
{

    qInfo() << "SCamVideo::activate()...";

    qInfo() << "    status ="<<this->getStatusString();

    if (m_status==CLOSED){
        this->open();
        qInfo() << "    status ="<<this->getStatusString();
    }

    m_opMutex.lock();

    if( m_status==OPEN) {

        setState(ACTIVE);

        m_opMutex.unlock();
        return true;

    } else {

        m_opMutex.unlock();
        return false;
    }

}

/*
bool SCamVideo::isReady()
{
    return m_status==ACTIVE;
}
*/

bool SCamVideo::deactivate_internal()
{
    //m_status=OPEN;
    setState(SCam::OPEN);
    return true;
}

bool SCamVideo::close_internal()
{

    m_status=CLOSED;
    return true;
}

/*
double SCamVideo::getFPS()
{
    return m_fileFramerate;
}
*/

bool SCamVideo::capture_internal()
{
    //qInfo() << "SCamVideo::capture()";

    if (m_opMutex.tryLock()){
        if(m_status==ACTIVE){

            m_index++;
            if (m_index>=m_numBuf) m_index=0;

            struct SwsContext *sws_ctx = Q_NULLPTR;
            int frameFinished;
            AVPacket packet;
            // initialize SWS context for software scaling
            sws_ctx = sws_getContext(m_codecCtx->width,
                                    m_codecCtx->height,
                                    m_codecCtx->pix_fmt,
                                    m_codecCtx->width,
                                    m_codecCtx->height,
                                    AV_PIX_FMT_RGB24,
                                    SWS_BILINEAR,
                                    Q_NULLPTR,
                                    Q_NULLPTR,
                                    Q_NULLPTR
                );

            //returning the next frame of a stream. The information is stored as a packet in packet.
            if (av_read_frame(m_formatCtx, &packet)>=0) {

              // Is this a packet from the video stream we found at opening?
              if(packet.stream_index==m_videoStream) {
                // Decode the video frame
                avcodec_decode_video2(m_codecCtx, m_frame, &frameFinished, &packet);

                // Did we get a video frame?
                if(frameFinished) {
                // Convert the image from its native format to RGB
                    sws_scale(sws_ctx,
                                (uint8_t const * const *)m_frame->data,
                                m_frame->linesize,
                                0,
                                m_codecCtx->height,
                                m_frameRGB->data,
                                m_frameRGB->linesize);


                }
              }

              // Free the packet that was allocated by av_read_frame
              av_free_packet(&packet);
            } else {
                av_seek_frame(m_formatCtx, 0, 0, 0);
                m_timeStamp.setHMS(0,0,0);
            }

            m_timeStamp = m_timeStamp.addMSecs(static_cast<int>(1000.0/m_fileFramerate));

            emit newImage();
            m_opMutex.unlock();

            //this->thread()->wait(2000); //should be handled by the caller regarding the getFPS() method, not in this special class

            return true;
        } else {
            qInfo() << "capture while not active...";
            m_opMutex.unlock();
        }

        //emit processedImage(img);
    } else{
        qInfo() << "capture while locked...";
    }
    return false;
}

QList<SCam *> SCamVideo::loadCams(QStringList &directoryList)
{
    qInfo() << "SCamVideo::loadCams() from a list of "<< directoryList.length()<<"directorys";
    QList<SCam *> ret;
    for(QString str:directoryList){
        QList<SCam*> scl=loadCams(str);
        if(!scl.isEmpty()) ret.append(scl);
    }

    return QList<SCam *>(ret);
}

QList<SCam *> SCamVideo::loadCams(QString &directory)
{
    QList<SCam*> ret;

    qInfo() << "SCamVideo::loadCams() loading videos from directory "<<directory;
    //1.make a list of suitable files in the directory
    QDir dir(directory);
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Name);

    QFileInfoList list = dir.entryInfoList();
    QStringList files;
    for(QFileInfo fi:list){
        //qInfo() << qPrintable(QString("%1 bytes in %2").arg(fi.size(), 10).arg(fi.fileName()));
        if (fi.fileName().endsWith("mp4")) {
            ret.append(new SCamVideo(directory,fi.fileName()));
        }
    }

    if (ret.isEmpty()) qInfo() << "     no files found!";

    return ret;
}
