#include "v4l2cam.h"

V4L2Cam::V4L2Cam(QString filename) :
    SCam(filename.mid(filename.lastIndexOf("/")), filename, 0),
    devAvail(false),
    fd(0),
    yuv(Q_NULLPTR),
    rgb(Q_NULLPTR),
    rgbMat(Q_NULLPTR),
    img(Q_NULLPTR),
    opencvMode(true)
{
    m_settings.capBrightness=false;
    m_settings.capContrast=false;
    m_settings.brightness=0;
    m_settings.minBrightness=0;
    m_settings.maxBrightness=0;
    m_settings.contrast=0;
    m_settings.minContrast=0;
    m_settings.maxContrast=0;

    m_imageSize.setWidth(640);
    m_imageSize.setHeight(480);

    //m_previewSize.setWidth(640);
    //m_previewSize.setHeight(480);
    formats.append(new CamFmt(640,480,"640x480(VGA)"));
    formats.append(new CamFmt(640,480,"320x240(QVGA)"));

    qInfo() << "V4L2Cam::V4L2Cam("<<internalName<<")";

    QFile devfile(internalName);
    devAvail = devfile.exists();
    qInfo() << "    device file descriptor available="<<devAvail;

    m_status=CLOSED;

}

V4L2Cam::~V4L2Cam()
{
    qInfo() << "V4L2Cam::~V4L2Cam("<<internalName<<")";
    if (rgbMat!=NULL) delete rgbMat;

}

void V4L2Cam::errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    if (errno==25){
        //Inappropriate ioctl for device - maybe this is not a cam?!?
        devAvail=false; //try not to kill the application
    } else {
        exit(EXIT_FAILURE);
    }
}

int V4L2Cam::xioctl(int fh, int request, void *arg)
{
    int r;
    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);
    return r;
}

bool V4L2Cam::open_internal(){

    if (m_status == CLOSED) {
        qInfo() << "V4L2Cam::open() on "<<internalName;
        if (devAvail){
            struct stat st;

            if (-1 == stat(internalName.toLatin1().data(), &st)) {
                fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                         internalName.toLatin1().data(), errno, strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (!S_ISCHR(st.st_mode)) {
                fprintf(stderr, "%s is no device\n", internalName.toLatin1().data());
                exit(EXIT_FAILURE);
            }

            //this open() belongs to fcntl which is actually hidden by our parents class open()
            //fd = open(internalName.toLatin1().data(), O_RDWR /* required */ | O_NONBLOCK, 0);
            //but that worked

            //so we try to use the Qt's high level (not tested yet)
            QFile f(internalName);
            f.open(fd,QFile::ReadWrite);
            //todo: we must get rid of the fd access

            //if (-1 == fd) {
            if(!f.isOpen()) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n",
                         internalName.toLatin1().data(), errno, strerror(errno));
                //exit(EXIT_FAILURE);
            }
            init_device();

            //m_status = OPEN;
            setState(OPEN);

            return true;

        } else {
            qInfo() << "    rather don't, device was not available.";
            return false;
        }
    } else {
        return false;
    }
}



void V4L2Cam::init_device()
{
    qInfo() << "V4L2OpenCVCam::init_device()";

    if (devAvail){
        struct v4l2_capability cap;
        unsigned int min;

        //reading the capabilities (returns a 32bit vector, telling us, what we can do with the device)
        if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s is no V4L2 device\n",
                                 internalName.toLatin1().data());
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_QUERYCAP");
                }
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, "%s is no video capture device\n",
                         internalName.toLatin1().data());
                exit(EXIT_FAILURE);
        }

        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                fprintf(stderr, "%s does not support streaming i/o\n",
                         internalName.toLatin1().data());
                exit(EXIT_FAILURE);
        }


        check_control_caps();

        /* Select video input, video standard and tune here. */
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        CLEAR(cropcap);
        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; /* reset to default */

                if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                break;
                        default:
                                /* Errors ignored. */
                                break;
                        }
                }
        } else {
                /* Errors ignored. */
        }

        //set the capturing format
        struct v4l2_format fmt;
        CLEAR(fmt);
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width       = m_imageSize.width();
        fmt.fmt.pix.height      = m_imageSize.height();
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        fmt.fmt.pix.field       = V4L2_FIELD_ANY;

        if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
                errno_exit("VIDIOC_S_FMT");

        /* Note VIDIOC_S_FMT may change width and height. */

        /* Buggy driver paranoia. */
        min = fmt.fmt.pix.width * 2;
        if (fmt.fmt.pix.bytesperline < min)
                fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if (fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;

        init_mmap();
    } else {
        qInfo() << "    no camera device available, nothing to init";
    }

}


void V4L2Cam::init_mmap(){
    qInfo() << "V4L2Cam::init_mmap()";

    struct v4l2_requestbuffers req;
    CLEAR(req);
    req.count = 2;//why we need 4 of them???
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
            if (EINVAL == errno) {
                    fprintf(stderr, "%s does not support "
                             "memory mapping\n", internalName.toLatin1().data());
                    exit(EXIT_FAILURE);
            } else {
                    errno_exit("VIDIOC_REQBUFS");
            }
    }

    if (req.count < 2) {
            fprintf(stderr, "Insufficient buffer memory on %s\n",
                     internalName.toLatin1().data());
            exit(EXIT_FAILURE);
    }

    yuv = (buffer*)calloc(req.count, sizeof(buffer));

    if (!yuv) {
            fprintf(stderr, "Out of memory\n");
            exit(EXIT_FAILURE);
    }


    struct v4l2_buffer buf; //just a helper object?
    CLEAR(buf);
    buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory      = V4L2_MEMORY_MMAP;
    buf.index       = 0;


    if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            errno_exit("VIDIOC_QUERYBUF");

    yuv->length = buf.length;
    //this is the moment where the buffer object is mapped to the correct target
    yuv->start = mmap(NULL /* start anywhere */,
                  buf.length,
                  PROT_READ | PROT_WRITE /* required */,
                  MAP_SHARED /* recommended */,
                  fd, buf.m.offset);


    if ( MAP_FAILED == yuv->start )
                errno_exit("mmap");

    if (opencvMode){
        rgbMat=new cv::Mat(m_imageSize.height(),m_imageSize.width(),CV_8UC3);

        img = new QImage((unsigned char*)(rgbMat->data), m_imageSize.width(), m_imageSize.height(), QImage::Format_RGB888);
        /*
        memcpy((uchar*)scanImg.constBits(), scanCam->rgb->start, scanImg.byteCount());
        */

    } else {
        qInfo() << "    ...calloc( "<<req.count<<"," << sizeof(buffer)<<")";
        rgb = (buffer*)malloc(sizeof(buffer));
        rgb->length=m_imageSize.width()*m_imageSize.height()*3;
        qInfo() << "    ...malloc( "<<rgb->length<<")";
        rgb->start = malloc(rgb->length);

        img = new QImage((unsigned char*)(rgb->start), m_imageSize.width(), m_imageSize.height(), QImage::Format_RGB888);
    }

    qInfo() << "    ...done!";

}



void V4L2Cam::check_control_caps()
{
    //check controls (we want to adjust brightness, contrast and saturation
    struct v4l2_queryctrl queryctrl;
    CLEAR(queryctrl);
    //check for brightness
    queryctrl.id = V4L2_CID_BRIGHTNESS;
    if ( EINVAL==xioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)){
        qInfo() << "problem: Brightness of this cam is not adjustable";
    } else {
        qInfo() << "brightness is adjustable";
        //it is described in integer, lets show some details;
        qInfo() << "    min="<<queryctrl.minimum;
        qInfo() << "    max="<<queryctrl.maximum;
        qInfo() << "    default="<<queryctrl.default_value;

        m_settings.brightness=queryctrl.default_value;
        m_settings.minBrightness = queryctrl.minimum;
        m_settings.maxBrightness = queryctrl.maximum;

        m_settings.capBrightness=true;
    }

    //check for contrast
    queryctrl.id = V4L2_CID_CONTRAST;
    if ( EINVAL==xioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)){
        qInfo() << "problem: Contrast of this cam is not adjustable";
    } else {
        qInfo() << "contrast is adjustable";
        //it is described in integer, lets show some details;
        qInfo() << "    min="<<queryctrl.minimum;
        qInfo() << "    max="<<queryctrl.maximum;
        qInfo() << "    default="<<queryctrl.default_value;

        m_settings.contrast=queryctrl.default_value;
        m_settings.minContrast=queryctrl.minimum;
        m_settings.maxContrast=queryctrl.maximum;

        m_settings.capContrast=true;
    }
}

bool V4L2Cam::activate_internal()
{
    if (m_status == CLOSED){
        open_internal();
    }

    if (m_status == OPEN){
        qInfo() << "V4L2Cam::activate()";

        if (devAvail){

            struct v4l2_buffer buf;
            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = 0;
            int ret=xioctl(fd, VIDIOC_QBUF, &buf);
            qInfo() << "    xioctl(fd, VIDIOC_QBUF, &buf) returned " << ret;
            if (ret == -1) errno_exit("VIDIOC_QBUF");

            enum v4l2_buf_type type;
            CLEAR(type);
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            ret = xioctl(fd, VIDIOC_STREAMON, &type);
            qInfo() << "    xioctl(fd, VIDIOC_STREAMON, &type) returned " << ret;
            if (ret == -1) errno_exit("VIDIOC_STREAMON");

        } else {
            qInfo() << "    ... no camera device available!";
        }

        qInfo() << "    ...done!";
        setState(ACTIVE);
    }
}

bool V4L2Cam::isReady()
{
    return true;
}

bool V4L2Cam::deactivate_internal()
{
    if (devAvail){
        enum v4l2_buf_type type;
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
                errno_exit("VIDIOC_STREAMOFF");
        setState(OPEN);
    } else {
        qInfo() << "    V4L2OpenCVCam::stop_capturing() : no camera device available";
    }
}

bool V4L2Cam::close_internal()
{
    setState(CLOSED);
    return false;
}

QImage *V4L2Cam::getNewestImage()
{
    return img;
}

ImgProc* V4L2Cam::getNewestIP()
{
    return nullptr;
}

/*
QImage *V4L2Cam::getNewestPreview()
{
    return img;
}
*/

int V4L2Cam::read_frame()
{
    //qInfo() << "    V4L2::read_frame()...";

    //why do we use here another buffer?
    struct v4l2_buffer buf;
    //CLEAR(buf); //do we really need that?
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
            case EAGAIN:
                    return 0;
            case EIO:
                    /* Could ignore EIO, see spec. */
                    /* fall through */
            default:
                    errno_exit("VIDIOC_DQBUF");
            }
    }

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");

    return 1;

}

/*
void V4L2Cam::triggerPreview()
{
    qInfo() << "V4L2Cam::triggerPreview()...";

    assert(isReady());

    if (trigger()) emit newPreview();
    else emit captureFailed("V4L2 Cam preview failed!");

}
*/

/*
void V4L2Cam::captureRequest(bool preview){
    qInfo() << "V4L2Cam::triggerImage()...";

    assert(isReady());

    //no difference in resolutions for now
    if (capture(preview)) emit newImage(preview);
    else emit captureFailed(preview, "V4L2 Cam trigger failed!");

}
*/


/*
bool V4L2Cam::capture(bool preview)
{

    assert(isReady());

    if (devAvail){
        //if (!autostart) start_capturing();

        for (;;) {
                fd_set fds;
                struct timeval tv;
                int r;
                FD_ZERO(&fds);
                FD_SET(fd, &fds);

                // Timeout
                tv.tv_sec = 5;
                tv.tv_usec = 0;

                r = select(fd + 1, &fds, NULL, NULL, &tv);

                if (-1 == r) {
                        qInfo() << "    ...select fd+1 returned -1";
                        if (EINTR == errno)
                                continue;
                        errno_exit("select");
                }
                if (0 == r) {
                        fprintf(stderr, "select timeout\n");
                        exit(EXIT_FAILURE);
                }
                if (read_frame()){
                        //qInfo() << "    ...read_frame() is on!";
                        break;
                }
        }

        //qInfo() << "    ...got new Image, convert it to a useable form!";

        //convert the yuv from sensor to useable rgb
        if (opencvMode){
            yuyv2openCVMat();
        } else {
            yuyv2rgb(); //manually through cpu
        }

        emit newImage(preview);
        m_opMutex.unlock();

        return true;

        //if (!autostart) stop_capturing();
    } else {
        qInfo() << "    no camera device available, nothing to capture";
        return false;
    }

}
*/

bool V4L2Cam::capture_internal()
{

    assert(isReady());

    if (devAvail){
        //if (!autostart) start_capturing();

        for (;;) {
                fd_set fds;
                struct timeval tv;
                int r;
                FD_ZERO(&fds);
                FD_SET(fd, &fds);

                // Timeout
                tv.tv_sec = 5;
                tv.tv_usec = 0;

                r = select(fd + 1, &fds, NULL, NULL, &tv);

                if (-1 == r) {
                        qInfo() << "    ...select fd+1 returned -1";
                        if (EINTR == errno)
                                continue;
                        errno_exit("select");
                }
                if (0 == r) {
                        fprintf(stderr, "select timeout\n");
                        exit(EXIT_FAILURE);
                }
                if (read_frame()){
                        //qInfo() << "    ...read_frame() is on!";
                        break;
                }
        }

        //qInfo() << "    ...got new Image, convert it to a useable form!";

        //convert the yuv from sensor to useable rgb
        if (opencvMode){
            yuyv2openCVMat();
        } else {
            yuyv2rgb(); //manually through cpu
        }

        //emit newImage(preview);
        m_opMutex.unlock();

        return true;

        //if (!autostart) stop_capturing();
    } else {
        qInfo() << "    no camera device available, nothing to capture";
        return false;
    }

}

bool V4L2Cam::setBrightness(double b)
{
    if (m_settings.capBrightness) {
        //clamping
        if (b>1.0)b=1.0;
        else if (b<-1.0) b=-1.0;
        //scale brightness from -1.0 .. +1.0 to min .. max
        int setB = m_settings.minBrightness + (m_settings.maxBrightness-m_settings.minBrightness)*(b+1.0)*0.5;

        //now apply that to the cam
        struct v4l2_control ctrl;
        ctrl.id = V4L2_CID_BRIGHTNESS;
        ctrl.value = setB;

        if ( EINVAL==xioctl(fd, VIDIOC_S_CTRL, &ctrl)){
            qInfo() << "error on brightness adjustment";
            return false;
        } else {
            qInfo() << "brightness adjusted to"<<setB;
            m_settings.brightness=b;
            return true;
        }

        return true;

    } else {
        qDebug() << "V4L2OpenCVCam::setBrightness() - brightness is not adjustable, stop doing that";
        return false;
    }
}

bool V4L2Cam::setContrast(double c)
{
    if (m_settings.capContrast) {
        //clamping
        if (c>1.0) c=1.0;
        else if (c<-1.0) c=-1.0;
        //scale brightness from -1.0 .. +1.0 to min .. max
        int setC = m_settings.minContrast +
                    (m_settings.maxContrast-m_settings.minContrast)*(c+1.0)*0.5;

        //now apply that to the cam
        struct v4l2_control ctrl;
        ctrl.id = V4L2_CID_CONTRAST;
        ctrl.value = setC;

        if ( EINVAL==xioctl(fd, VIDIOC_S_CTRL, &ctrl)){
            qInfo() << "error on contrast adjustment";
            return false;
        } else {
            qInfo() << "contrast adjusted to"<<setC;
            m_settings.contrast=c;
            return true;
        }

    } else {
        qDebug() << "V4L2OpenCVCam::setContrast() - contrast is not adjustable, stop doing that";
        return false;
    }


}

void V4L2Cam::setFormat(int formatIndex)
{

}

/**
 * @brief V4L2Cam::checkForCam
 * @param file
 * @return true if the file is pointing to a genuine v4l2 capture device
 *          should be used before creating a V4L2Cam
 */
bool V4L2Cam::checkForCam(QString file)
{
    //TODO: use ioctl to look for camera capabilities
    return true;
}

QList<SCam*> V4L2Cam::loadCams()
{
    QList<SCam*> ret;

    //classical v4l2 dev integr vial /dev/video0..video'n'
    QDir dir("/dev/v4l/by-id");
    dir.setFilter(QDir::Files);

    QStringList fltlst;
    fltlst << "Video";
    QStringList devs = dir.entryList();
    qInfo("V4L2Cam::loadCams() found following items:");
    for (QString str: devs){
        qInfo() << "    "<<str;
        ret.append(new V4L2Cam(dir.path()+"/"+str));
    }



    return ret;
}



void V4L2Cam::yuyv2rgb()
{
    //qInfo() <<"V4L2Cam::yuyv2rgb()...";

    uchar *yuyv=(uchar*)yuv->start;
    //this->rgb->start=rgbMat->data;

    //avoid over and over to de dereferencing
    uchar *rgb=(uchar*)this->rgb->start;

    int y;
    int cr;
    int cb;

    double r;
    double g;
    double b;

    int width=m_imageSize.width();
    int heigth=m_imageSize.height();

    for(int i=0, j=0; i<(int)width*(int)heigth*3; i+=6, j+=4){

        //first pixel
        y = yuyv[j];
        cb = yuyv[j+1];
        cr = yuyv[j+3];

        r = y + (1.4065 * (cr - 128));
        g = y - (0.3455 * (cb - 128)) - (0.7169 * (cr - 128));
        b = y + (1.7790 * (cb - 128));

        if (r < 0) r = 0;
            else if (r > 255) r = 255;
        if (g < 0) g = 0;
            else if (g > 255) g = 255;
        if (b < 0) b = 0;
            else if (b > 255) b = 255;


        rgb[i] = (unsigned char)r;
        rgb[i+1] = (unsigned char)g;
        rgb[i+2] = (unsigned char)b;

        //second pixel
        y = yuyv[j+2];
        cb = yuyv[j+1];
        cr = yuyv[j+3];

        r = y + (1.4065 * (cr - 128));
        g = y - (0.3455 * (cb - 128)) - (0.7169 * (cr - 128));
        b = y + (1.7790 * (cb - 128));

        if (r < 0) r = 0;
            else if (r > 255) r = 255;
        if (g < 0) g = 0;
            else if (g > 255) g = 255;
        if (b < 0) b = 0;
            else if (b > 255) b = 255;

        rgb[i+3] = (unsigned char)r;
        rgb[i+4] = (unsigned char)g;
        rgb[i+5] = (unsigned char)b;
    }

    //qInfo() << "    ...done";

}

void V4L2Cam::yuyv2openCVMat()
{

#ifndef CV_YUV2RGB_YUYV
#define CV_YUV2RGB_YUYV cv::COLOR_YUV2RGB_YUYV
#endif
    //qInfo() <<  "V4L2Cam::yuyv2openCVMat()...";
    cv::Mat yuyvmat(480,640,CV_8UC2,yuv->start);
    cv::cvtColor(yuyvmat, *rgbMat, CV_YUV2RGB_YUYV);
    //qInfo() << "    ...done";

}



