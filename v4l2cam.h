#ifndef V4L2CAM_H
#define V4L2CAM_H

#include "scam.h"
#include <QFile>
#include <QDir>

#include <linux/videodev2.h>
//#include <libv4lconvert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>

//theye where not available at Raspbian (20170904 - check that)
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <QDebug>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer {
        void   *start;
        size_t  length;
        uint    width;
        uint    height;
};


class V4L2Cam : public SCam
{
public:
    explicit V4L2Cam(QString filename);
    ~V4L2Cam();

    bool open_internal() override;
    bool activate_internal() override;

    bool isReady() override; //ACTIVE status

    bool deactivate_internal() override;
    bool close_internal() override;

    QImage* getNewestImage() override;
    ImgProc* getNewestIP() override;

private:
    bool            devAvail;
    int             fd;      //file descriptor

    buffer          *yuv;    //our buffer containing the pointer to the memory section of the image
    buffer          *rgb;    //reformatted to rgb
    cv::Mat         *rgbMat; //a opencv-Mat output option
    QImage          *img;

    bool            opencvMode;

    void errno_exit(const char *s);
    int xioctl(int fh, int request, void *arg);
    void process_image(const void *p, int size);
    int read_frame();

    void init_device();

    void init_mmap();

    void check_control_caps();
    void uninit_device();

    //internal conversion
    void yuyv2rgb();
    void yuyv2openCVMat();

    //bool capture(bool preview) override;
    bool capture_internal() override;

public:
    static bool checkForCam(QString file);
    static QList<SCam *> loadCams();

public slots:
    bool setBrightness(double b) override;
    bool setContrast(double c) override;
    void setFormat(int formatIndex) override;

};

#endif // SLABSV4L2CAM_H
