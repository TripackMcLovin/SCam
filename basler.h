#ifndef BASLER_H
#define BASLER_H

#include "scamlib.h"
#include "scam.h"
#include <pylon/PylonIncludes.h>

#include <QDebug>

//
// Anlegen list mit pointern zu basler buffer (zyklisch)
// smartpointer->"dump"-pointer
// un8 pointer  = rueckgabe zu baserler buffer (static cast)
// damit image prozessor
// cv::Mat reassing data pointer? assinging data to cv mat



class LIB_EXPORT Basler : public SCam, public Pylon::IBufferFactory
{
public:
    explicit Basler(Pylon::CDeviceInfo &deviceInfo);
    ~Basler();

    bool open_internal() override;
    bool activate_internal() override;

    bool isReady() override; //ACTIVE status

    bool deactivate_internal() override;
    bool close_internal() override;



    //for the IBufferFactory Interface
    //this class is its won buffer factory
    virtual void AllocateBuffer( size_t bufferSize, void** pCreatedBuffer, intptr_t& bufferContext) override;
    virtual void FreeBuffer( void* pCreatedBuffer, intptr_t bufferContext) override;
    virtual void DestroyBufferFactory() override;
    static long s_instanceCount;
    long m_bufferContext;


private:
    //bool capture(bool preview) override;
    bool capture_internal() override;

    Pylon::CDeviceInfo m_deviceInfo;
    Pylon::CInstantCamera *m_CInstantCamera;

    QImage *imgQT;
    cv::Mat *imgCV;

public:
    static QList<SCam *> loadCams();

    /**
     * @brief Get the current temperature state and DeviceTemperature
     * @param temp
     * @param tempState
     * @return
     */
    bool getDeviceTemperature(double &temp, std::string &tempState);

    /**
     * @brief setPixelFormat
     * @param pixelFormat
     * @return
     */
    bool setPixelFormat(Pylon::String_t pixelFormat);

public slots:

    bool setBrightness(double b) override;
    bool setContrast(double c) override;
    void setFormat(int formatIndex) override;

};

#endif // BASLER_H
