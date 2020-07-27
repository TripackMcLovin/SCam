#ifndef IMGPROC_H
#define IMGPROC_H

#include "scamlib.h"

#include <QObject>
#include <QDebug>
#include <QMap>
#include <QImage>
#include <QRgb>
#include <QGraphicsScene>

#include <opencv2/core/core.hpp>
//#include "opencv2/core.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

#include "util/colorspaces.h"
#include "imgelement.h"

//forward declaration
class ImgElement;
class IPRegistry;


/**
 * @brief The ImgProcessor class is a combined representation for our analytic work within opencv
 *          and the need for proper visual representation
 *        depending on the constructor we internally save the raw data from where it is comming from
 *        and make it accessible to both systems (QT and openCV) via their API
 *        so any ImgProcessor can be used in all environments
 *        and to make it more comfortable to see what the processor has internally saved and knew about its
 *        content, it also offers the capability to be drawn to a graphic scene
 *
 *        also we'd like to work only with 3 different formats:
 *
 *
 *         description           OpenCV      QTs QImage-Format
 *
 *        8bit grayscale        CV_8U       QImage::Format_Grayscale8
 *        24bit RGB (or HSV)    CV_8UC3     QImage::Format_RGB888
 *        32bit ARGB            CV_8UC4*    QImage::Format_ARGB32
 *
 *        *carefull, we don't know how cv is working with this alpha
 *
 *        todo: we need a format map for both directions
 *
 *        and channelwise lists of 8 bit (Stacks) ????
 *        QVector<QImage> or QVector<cv::Mat>
 *
 *        for performance reasons and since most accesses will appear in the cv scope,
 *        internally the image will be kept as a cv::Mat and the
 *        outer access to QImage will be through the cv-pointer,
 *        so the cv::Mat parameterd constructor is prefered
 *
 */
//moc assumes that the first inheritated object is a QObject
//aaaand: QImage is NOT an QObject... damit!!!
class LIB_EXPORT ImgProc : public QObject, public cv::Mat
{
    enum IP_format{
        IPM_GRAY8,
        IPM_RED,
        IPM_GREEN,
        IPM_BLUE,
        IPM_HUE,
        IPM_SAT,
        IPM_VALUE,
        IPM_RGB,
        IPM_HSV,
        IPM_RGB_STACK,
        IPM_HSV_STACK,
        IPM_GRAY_STACK,
        IPM_BINARY_MASK,
        IPM_BINARY_MASK_STACK,
        IPM_INDEX_MASK
    };
    Q_OBJECT
public:

    ImgProc(QImage &img, bool deepCopy=false, QString name="");
    ImgProc(cv::Mat &mat, bool deepCopy=false, QString name="");
    ImgProc(uchar* dataPtr, int width, int height, int cvfmt , QString name="");
    ImgProc(int width, int height, int cvfmt, QString name=""); //allocation constructor, an empty Image
    ImgProc(ImgProc *that, QString name);
    ImgProc(QString filepath, bool rgb, bool deepCopy =false, QString name ="");

    ~ImgProc();

    //avoid getting removed by registry
    /**
     * @brief setPersistent is a protection to avoid to get removed by registries or managers
     *          mainly for instances that represents the memory for cameras or open files
     */
    void setPersistent();
    bool isPersistent();
    void setDescription(QString descr);

    void setImage(QImage &img);
    void setImage(QString path);
    QImage *getImage();

    void copyImageFrom(ImgProc *that);

private:
    static uint32_t instanceCounter;
    static QMap<int,int> formatMap;

    IP_format m_ipFormat;
    QString m_name;
    QString m_description;
    QList<QString> m_history;

    double m_scalePixToMM;

    bool m_isHSV;

    bool m_isPersistent;

    //an "image-stack" like system
    QVector<ImgProc*> m_subProcessors; //for the instances that holds/represents the stack
    bool m_isHidden; //for the instance that is part of the stack (should not be seen/shown by the registry)

    //blocking processing and viewing
    bool m_isReady;

    //analytical and structural content
    QMap<QString,ImgElement*> m_imgElements;
    QMap<QString,cv::Mat*>  m_auxMats; //a list of auxillary mats that are strongly bound to this ie (e.g. intermediate results and masks)
    QString matToDraw;

    //internal pointer to our structure as QImage
    QImage *m_thisAsQImg;


public:
    void drawToScene(QGraphicsScene *scene, bool drawElems=true);

    void setMatToDraw(QString matName=""); //set the mat that should be used to drawn as background, if not set, this processors mat is used

    int width();
    int height();
    IP_format getFormat();

    void addElement(ImgElement *element);
    void addElements(QList<ImgElement *> elements);
    void addAuxMat(QString name, cv::Mat* mat);
    void deleteElement(QString name);
    void clearElements();

    void setScale(double pixToMM);

    void unmarkAllElements();

    void lock();
    void unlock();

    QStringList getElementNames();
    ImgElement* getElement(QString name);
    QStringList getAuxMatNames();
    cv::Mat* getAuxMat(QString name);

    //ImgProc*getLinkedProcessor(QString ipName);

    //void linkProcessor(QString linkname, ImgProc* processor);

    //tests
    void rainbowFill();
    void setReady(bool ready=true);
    bool isReady();

public slots:
    QString getName();
    QString getDescription();
    void appendHistory(QString entry);
    QList<QString>* getHistory();
    QString getHistoryEntry(int entry);

signals:
    //void contentUpdate(ImgProc* ip);
    void gotReady(ImgProc* ip); //this is also showing that the content was updated

};

class IPRegistry : public QObject
{
    Q_OBJECT
public:
    explicit IPRegistry();

public:
    //all the fancy shit
    void append(ImgProc *proc);
    void append(QString name, ImgProc* p);
    ImgProc *operator[](QString);
    void remove(QString name);
    void remove(ImgProc *p);
    virtual void clear();
    //int size();

    static IPRegistry *getRegistry();
    int size();
    ImgProc* getProcessor(int index);
    ImgProc* getProcessor(QString name);


private:
    //QList<ImgProc*> m_processorList;
    QMap<QString,ImgProc*> m_map;
    //std::list<ImgProc*> m_processorList;
    static IPRegistry *registry;

signals:
    void newElement(ImgProc *proc);
    void elementRemoved(ImgProc *proc, QString name);

};

#endif // IMGPROC_H
