#include "dummycam.h"

int DummyCam::instanceCount =0;

DummyCam::DummyCam() : SCam("Dummy Cam "+QString::number(instanceCount),"dummy_"+QString::number(instanceCount),1)
{
    m_ip=new ImgProc(640,480,CV_8UC3,"dc_buf_"+QString::number(instanceCount));
    m_imgProcessors.append(m_ip);
    instanceCount++;
}

DummyCam::~DummyCam()
{
    //since m_ip is a true member, its destructor will be called automatically
}

bool DummyCam::open_internal()
{
    return true;
}

bool DummyCam::activate_internal()
{
    m_ip->rainbowFill();
    return true;
}

bool DummyCam::isReady()
{
    return true;
}

bool DummyCam::deactivate_internal()
{
    return true;
}

bool DummyCam::close_internal()
{
    return true;
}

QImage *DummyCam::getNewestImage()
{
    m_ip->rainbowFill();
    return m_ip->getImage();
}

ImgProc *DummyCam::getNewestIP()
{
    m_ip->rainbowFill();
    return m_ip;
}

bool DummyCam::capture_internal()
{
    m_opMutex.unlock();
    emit newImage();
    return true;
}

bool DummyCam::setBrightness(double b)
{
    Q_UNUSED(b);
    return true;
}

bool DummyCam::setContrast(double c)
{
    Q_UNUSED(c);
    return true;
}

void DummyCam::setFormat(int formatIndex)
{
    Q_UNUSED(formatIndex);
    return;
}
