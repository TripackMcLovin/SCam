#ifndef SLABSIDSCAM_H
#define SLABSIDSCAM_H

#include "scam.h"
#include "ueye.h"

#include <QVector>
#include <QDebug>
#include <QImage>


#define IDS_USB 1<<1
#define IDS_ETH 1<<2

class IDSCam : public SCam
{
    Q_OBJECT
public:
    explicit IDSCam(uint8_t numBuf);
    ~IDSCam();

    bool open_internal() override;
    bool activate_internal() override;
    bool deactivate_internal() override;
    bool close_internal() override;

    bool capture_internal() override; //to be used by the worker

    static QList<SCam *> loadCams(int subSysFlag=IDS_USB|IDS_ETH);
    static int sICount;

private:
    int ret;

    CamSettings *settings;

    HIDS hCam;

    int nColorMode;
    int nBitsPerPixel;
    int nMaxWidth;
    int nMaxHeight;

    CAMINFO mCamInfo;

    SENSORINFO mSensorInfo;

    QVector<char*> ppcImgMem;
    QVector<int>  pid;


    INT initCamera(HIDS *hCam);
    void LoadParameter();
    void setResolution(QSize &size);


public slots:
    //void captureRequest(bool preview) override;
    //void openingRequest();
    //void activateRequest();
    bool setBrightness(double b) override;
    bool setContrast(double c) override;

    void setFormat(int formatIndex) override;

};

#endif // SLABSIDSCAM_H
