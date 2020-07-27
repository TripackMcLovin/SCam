#ifndef DUMMYCAM_H
#define DUMMYCAM_H

#include "scam.h"

class DummyCam : public SCam
{
public:
    explicit DummyCam();
    ~DummyCam();

    bool open_internal() override;
    bool activate_internal() override;

    bool isReady() override; //ACTIVE status

    bool deactivate_internal() override;
    bool close_internal() override;

    QImage* getNewestImage() override;
    ImgProc* getNewestIP() override;

    bool capture_internal() override; //the actual method grabbing an image, called by worker


    bool setBrightness(double b) override;
    bool setContrast(double c) override;
    void setFormat(int formatIndex) override;

private:
    ImgProc *m_ip;

    static int instanceCount;
};

#endif // DUMMYCAM_H
