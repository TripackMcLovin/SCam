#ifndef IMGELEMENT_H
#define IMGELEMENT_H

#include <QObject>
#include <QGraphicsScene>
#include <QDebug>
#include <cmath>
#include <assert.h>
//#include "imgprocessor.h"
#include "opencv2/core/core.hpp"

//forward declaration
class ImgProcessor;

/**
 * @brief The ImgElement class is a describing element for readouts and results of
 *              image analysis tasks. It is intended to be a superclass for everything
 *              we are talking in an image like points, lines, polygones and other
 *              geometric elements.
 *          The elements coordinate are always in image coordinates, therefore they are
 *              connected to a specific imageProcessor (ip)
 *
 *          It is intended that this base-class offers most of the capabilities that are
 *              needed by any subclass. Therefore its main member is a list of 2d-points
 *              that is representing a Point-Series connected trough lines,
 *              either open (series) or closed (polygon)
 *
 */
class ImgElement : QObject
{
    Q_OBJECT
public:
    //[[deprecated("use the cv::Mat constructor")]]
    ImgElement(QImage *parentImage, QString name);
    //better: since we try to put elements as part of ImageProcessor,
    //and IP is a opencv mat, we give the original mat instead of the qimgae
    ImgElement(cv::Mat *parentMat, QString name);

    ~ImgElement();

    enum ChannelIndex{
        CI_DEFAULT =0, //will give you either RGB (for 24bit image) or Val (for 8 bit image)
        CI_RED      =1,
        CI_GREEN    =1<<1,
        CI_BLUE     =1<<2,
        CI_HUE      =1<<3,
        CI_SAT      =1<<4,
        CI_VAL      =1<<5, //also used for 8-bit gray values
        CI_RGB      =CI_RED|CI_GREEN|CI_BLUE,
        CI_HSV      =CI_HUE|CI_SAT|CI_VAL,
        CI_ALL      =CI_RED|CI_GREEN|CI_BLUE|CI_HUE|CI_SAT|CI_VAL
    };


    //factories
    /*
        intended to be used instead of creating the geometric base by user every time
        all polygons run clockwise
    */
    static ImgElement *factoryRect(QImage *parentImage, QString name, QPointF start, QSizeF range);
    static ImgElement *factoryCircle(QImage *parentImage, QString name, QPointF center, double radius, int supportPoints, double rotationOffset=0.0);
    static ImgElement *factoryEllipse(QImage *parentImage, QString name, QPointF center, QRectF supportRect, double rotation, int supportPoints);

    //factory for regular polygons? or basic constructors?!?
protected:
    //ImgProcessor* m_parentImgProcessor;
    QImage* m_parentImage;
    cv::Mat* m_parentMat;
    QString m_name;

    QVector<QPointF> m_points;
    QVector<double> m_pntWeights;
    bool m_closed;

    //for the drawing (we should try to minimize information about how we are drawn - better use a drawing class for us)
    QColor m_lineColor;
    QColor m_fillColor;
    int m_lineWidth;

    bool m_marked;

    //for status/performance issues
    bool m_equidistant;//for performance in regular polygons to avoid overly complex point-calculations
    bool m_regular;

    QVector<QPointF> m_scanPoints; //calculated with calcScanPoints(), equidistant points along the curve, not the supporting points
    QMap<QString,QVector<int> > m_scanValues; //value readout along the scanPoints

public:

    QString getName();

    void setMarked(bool marked);
    void clear();
    void setStyle(QColor lineCol, QColor fillCol, int width);

    void updateParentImage(QImage *parentImage);

    void drawToScene(QGraphicsScene *scene);

    //void getGrayValue()
    void appendPoint(QPointF point);
    void appendPoints(QVector<QPointF> points);

    void setLineColor(QColor lineColor, int alpha);
    void setFillColor(QColor fillColor, int alpha);
    void closePolygon();

    //metrics
    double polyLength();
    QPointF getPoint(double curveParam, bool normalized=false);
    void calcScanPoints(double stepsize, int steps=0);
    void calcScanPoints(int pointNumber=0);

    QVector<QPointF> &getScanPoints();
    QVector<QPointF> &getPoints();
    //either make a readout at scan points or
    void readScanValues(int channelIndex=CI_DEFAULT);
    //at the supporting points
    void readPointValues(int channelIndex=CI_DEFAULT);

    QStringList getAvailableValues(); //to find out which arguments can be used at getScanValues
    QVector<int> &getScanValues(QString name);


};
#endif // IMGELEMENT_H
