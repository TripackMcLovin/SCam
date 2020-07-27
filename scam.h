#ifndef SCAM_H
#define SCAM_H

#include "scamlib.h"

#include <QObject>

#include <QSize>
#include <QVector>
#include <QMutex>
#include <QThread>

#include "imgproc.h"

/**
 * @brief The CamSettings class is a superset of all possible types of camera-setting from any vendor
 */
class CamSettings
{
public:
    CamSettings();
    ~CamSettings();

    int redGain;
    int greenGain;
    int blueGain;
    int masterGain;

    int pixelClock;
    int exposure;

    //from v4L2 cam -> capabilities and ranges
    bool capBrightness;
    bool capContrast;
    double brightness;
    int minBrightness;
    int maxBrightness;
    double contrast;
    int minContrast;
    int maxContrast;


};

class CamFmt{
public:
    CamFmt(int w, int h, QString name, int bpp=3);

    const unsigned int width;
    const unsigned int height;
    const unsigned int bytesPerPixel;
    const QString name;
    const int internalID;
};

class SCamWorker; //forward declaration for mutual dependent cam and worker


/**
 * @brief The SCam class
 *          is a virtual base class for all types of cameras to be accessible
 *          via a common and easy API
 *          where we only capture single images
 *          the workflow is
 *
 *              open -> activate(hold connection) -> triggerPreview/triggerImage
 *
 *          and get via the signal newPreview/newImage the feedback
 *          that the image is available
 *
 *          simple setups are setBrightness() setContrast()
 *
 */
class LIB_EXPORT SCam : public QObject
{
    Q_OBJECT

    friend class SCamWorker;

public:
    enum SC_STATE{
        NOT_AVAILABLE,
        CLOSED,
        OPEN,
        ACTIVE,
        CAPTURING,
        ERROR
    };

    explicit SCam(QString readableName, QString internalName, uint8_t numBufs, QObject *parent = nullptr);
    ~SCam();

    //internal: run by the worker, implemented by the specific child classes
private:
    virtual bool open_internal()=0;
    virtual bool activate_internal()=0;
    virtual bool deactivate_internal()=0;
    virtual bool close_internal()=0;

public:

    virtual bool isReady(); //==ACTIVE, ready to capture

    //implemented by the specific child classes - better: register them at m_imageProcessors!
    virtual QImage* getNewestImage();
    virtual ImgProc* getNewestIP();

    QSize imageSize(){return m_imageSize;}

    SC_STATE getStatus(){return m_status;}
    QString getStatusString();

    const QString readableName; //also the one to be searched with
    const QString internalName; //like a linux file descriptor e.g. /dev/video0

    const int m_numBuf;
    int m_index;

protected:

    //todo: get rid of the two modes, thats only troubling
    //QSize m_previewSize;
    QSize m_imageSize;
    SC_STATE m_status;
    CamSettings m_settings;

    //the buffers
    //QVector<QImage*> previews;
    //QVector<QImage*> images;
    QVector<ImgProc*> m_imgProcessors;

    void setState(SC_STATE newStatus);

    //the common thread parallel system by a worker class
    SCamWorker *m_worker;
    QThread *m_thread;
    QMutex m_opMutex;

    void activateWorker();
    void deactivateWorker();

protected: //only because of the external worker class
    //virtual bool capture(bool preview)=0;
    virtual bool capture_internal()=0; //the actual method grabbing an image

public:
    //QMap<QString,int> formats;
    QVector<CamFmt*> formats;

signals:
    //the actual API
    void stateChanged(SCam *sender);
    //void newPreview();
    //virtual void newImage(bool preview);
    void newImage();
    //void previewFailed(QString descr);
    //virtual void captureFailed(bool preview, QString descr);
    void captureFailed(QString descr);

    //object->worker
    void captureSig();
    void openSig();
    void activateSig();

public slots:

    //virtual void triggerPreview()=0;
    //virtual void captureRequest(bool preview);

    //the actual methods to work with any camera
    void open();
    void activate();
    void capture();
    void deactivate();
    void close();


    /**
     * @brief setBrightness
     * @param b brightness level between -1.0 and +1.0 while 0.0 is default
     * @return true if successfull
     */
    virtual bool setBrightness(double b)=0;
    /**
     * @brief setContrast
     * @param c contrast level between -1.0 and +1.0 while 0.0 is default
     * @return true if successfull
     */
    virtual bool setContrast(double c)=0;

    virtual void setFormat(int formatIndex)=0;

    bool isActive(); //m_status == ACTIVE);
    bool isCapturing(); //m_status == CAPTURING

};//SCam



/**
 * @brief The SCamWorker class the worker object for the thread-parallel capture processing
 */
class SCamWorker : public QObject
{
    Q_OBJECT
public:
    SCamWorker(SCam *cam);

private:
    SCam *m_cam;

private slots:
    //to be
    void opening();
    void activating();
    void capturing();

signals:
    //void newImage(bool preview);
    void newImage();
    //void captureFailed(bool preview, QString descr);
    void captureFailed(QString descr);
};

#endif // SCAM_H
