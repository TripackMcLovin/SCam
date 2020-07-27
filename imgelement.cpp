#include "imgelement.h"

ImgElement::ImgElement(QImage *parentImage, QString name): QObject(),
    m_parentImage(parentImage),
    m_parentMat(Q_NULLPTR),
    m_name(name),
    m_closed(false),
    m_lineColor(QColor(0xFF,0x00,0x50)),
    m_fillColor(Qt::red),
    m_lineWidth(1.0),
    m_marked(false),
    m_equidistant(true),
    m_regular(false)
{
    m_lineColor.setAlpha(64);
    m_fillColor.setAlpha(64);
}

ImgElement::ImgElement(cv::Mat *parentMat, QString name): QObject (),
    m_parentImage(Q_NULLPTR),
    m_parentMat(parentMat),
    m_name(name),
    m_closed(false),
    m_lineColor(QColor(0xFF,0x00,0x50)),
    m_fillColor(Qt::red),
    m_lineWidth(1.0),
    m_marked(false),
    m_equidistant(true),
    m_regular(false)
{
    m_lineColor.setAlpha(64);
    m_fillColor.setAlpha(64);
}

ImgElement::~ImgElement()
{
    m_points.clear();
}

ImgElement *ImgElement::factoryRect(QImage *parentImage, QString name, QPointF start, QSizeF range )
{
    ImgElement *ret=new ImgElement(parentImage, name);

    ret->appendPoint(start);
    ret->appendPoint(QPointF(start.x()+range.width(),start.y()));
    ret->appendPoint(QPointF(start.x()+range.width(),start.y()+range.height()));
    ret->appendPoint(QPointF(start.x(), start.y()+range.height()));
    ret->closePolygon();

    //maybe some specialties like rotation?!?

    return ret;
}

ImgElement *ImgElement::factoryCircle(QImage *parentImage, QString name, QPointF center, double radius, int supportPoints, double rotationOffset)
{
    ImgElement *ret=new ImgElement(parentImage, name);

    //OK, this is the easy stuff
    double alpha;
    for (int i=0; i<supportPoints; i++){
        alpha = (static_cast<double>(i)/supportPoints)*2*M_PI+rotationOffset;
        ret->appendPoint(QPointF(center.x()+radius*cos(alpha),center.y()+radius*sin(alpha)));

    }
    ret->closePolygon();

    return ret;
}

ImgElement *ImgElement::factoryEllipse(QImage *parentImage, QString name, QPointF center, QRectF supportRect, double rotation, int supportPoints)
{
    ImgElement *ret=new ImgElement(parentImage, name);

    assert(false);//don't use this method until implementation!!!

    return ret;
}

QString ImgElement::getName()
{
    return m_name;
}

void ImgElement::setMarked(bool marked)
{
    m_marked=marked;
}

void ImgElement::clear()
{
    m_points.clear();
    m_scanPoints.clear();
    m_scanValues.clear();
}

void ImgElement::setStyle(QColor lineCol, QColor fillCol, int width)
{
    m_lineColor=lineCol;
    m_fillColor=fillCol;
    m_lineWidth=width;
}

void ImgElement::updateParentImage(QImage *parentImage)
{
    if (m_parentImage!=Q_NULLPTR){

        assert(parentImage->width()==m_parentImage->width() &&
               parentImage->height()==m_parentImage->height() &&
               parentImage->format()==m_parentImage->format() );

        m_parentImage=parentImage;
    }
}


//ToDo: IE should not know anything about UI, also IP
//create interfaces to extract their information and do it in UI!!!
void ImgElement::drawToScene(QGraphicsScene *scene)
{
    if(m_points.count()>0){
        //qInfo() << "dawing ImgElement"<<m_name<<" to scene with "<<m_points.size()<<"points";

        //better: use for "marked" a wider back-pen (shaddow)
        m_lineColor.setAlpha(m_marked?200:64);
        QPen pen(m_lineColor);
        pen.setWidth(m_marked?3*m_lineWidth:m_lineWidth);

        QPen bigPen(m_lineColor);
        bigPen.setWidth(m_lineWidth*5);
        //mark the start-point TODO: replace with actual Dot or better:square
        QPointF start(m_points.at(0));
        QLineF line(start,start);
        scene->addLine(line,bigPen);

        //n-1 elements (in between points)
        for(int i=1;i<m_points.size(); i++){
            scene->addLine(QLineF(m_points[i-1],m_points[i]),pen);
        }

        //and if closed the n'th line
        if (m_closed){
            QLineF line(m_points.last(),m_points.at(0));
            scene->addLine(QLineF(m_points.last(),m_points.at(0)),pen);
            //filling? -> drawing it as a polygon
        }

        QPen crosspen(m_lineColor);
        pen.setWidth(m_lineWidth);

        if (!m_scanPoints.isEmpty()){
            for (QPointF p: m_scanPoints){
                scene->addLine(QLineF(p.x()-2,p.y(),p.x()+2,p.y()),crosspen);
                scene->addLine(QLineF(p.x(),p.y()-2,p.x(),p.y()+2),crosspen);
            }
        }
    }

}

void ImgElement::appendPoint(QPointF point)
{
    m_points.append(point);
}

void ImgElement::appendPoints(QVector<QPointF> points)
{
    for(int i=0; i<points.size(); i++){
        m_points.append(points.at(i));
    }
}

void ImgElement::setLineColor(QColor lineColor, int alpha)
{
    m_lineColor=lineColor;
    m_lineColor.setAlpha(alpha);
}

void ImgElement::setFillColor(QColor fillColor, int alpha)
{
    m_fillColor=fillColor;
    m_fillColor.setAlpha(alpha);
}

void ImgElement::closePolygon()
{
    m_closed=true;
}

/**
 * @brief ImgElement::polyLength
 * @return the length of the complete Polygon, including the last in case of a closed one
 */
double ImgElement::polyLength()
{
    double l=0;
    QPointF a=m_points.at(0);
    QPointF b;
    QPointF d;
    for(int i=1; i<m_points.size(); i++){
        b=a;
        a=m_points.at(i);
        d=b-a;
        l+= std::sqrt(d.x()*d.x() + d.y()*d.y());
    }

    //a last segment if this is closed polygon
    if (m_closed) {
        b=a;
        a=m_points.at(0);
        d=b-a;
        l+= std::sqrt(d.x()*d.x() + d.y()*d.y());
    }

    return l;
}

/**
 * @brief ImgElement::getPoint delives a specific point along the polygon-lines
 *          !should not be used for a equidistant series, use getPoints() instead to avoid squared complexity!
 * @param curveParam the leading value evaluating curve/polygon
 *          should be between 0. and polyLength() or 0. and 1. if normalize
 * @param normalize switch between real distance curve parameter (default, false)
 *          and normalized curve (true), the latter one helps on iterating through
 *          the curve
 * @return a point in the curve according to the parameter
 */
QPointF ImgElement::getPoint(double curveParam, bool normalized)
{
    if (m_points.size()>1){


        double pl=polyLength();
        if (normalized) curveParam*=pl;

        //clamp the parameter
        if (curveParam>pl)curveParam=pl;
        else if (curveParam<0.0) curveParam=0.0;


        //intermediate append front to the back for easier calculations
        if (m_closed) m_points.append(m_points.front());

        double remaining=curveParam;
        QPointF s;
        double sLen;
        QPointF ret;
        int i;

        for(i=1; i<m_points.size(); i++){
            s = m_points.at(i)-m_points.at(i-1);
            sLen = sqrt( s.x()*s.x() + s.y()*s.y() );
            if (sLen >= remaining){//we found the segment containing that point
                ret = m_points.at(i-1)+s*(remaining/sLen); //startpoint of segment plus the partial vector
                break;
            } else { //it is not in this segment, go on finding it
                remaining-=sLen;
            }
        }

        //and remove that intermediate element if it was added
        //if(m_closed) m_points.removeLast();
        if(m_points.front()==m_points.back()) m_points.removeLast();


        return ret;

    } else if (m_points.size()>0) {
        return m_points.at(0);
    }

    return QPointF(0,0);
}

void ImgElement::calcScanPoints(double stepsize, int steps)
{
    m_scanPoints.clear();

    if (stepsize<=0.0){ //meaning just use the support points as scan points

        for (QPointF p:m_points){
            m_scanPoints.append(p);
        }

        if (m_closed) m_scanPoints.append(m_points.front());

    } else {

        double sumlen=0.0;
        double total=polyLength();

        if (steps<=0) steps=total/stepsize;

        if(m_closed) m_points.append(m_points.front());

        //qInfo() << "polyLength" <<polyLength()<<"stepLength"<<stepLength;

        //the current segment between the n'th-1 and n'th element of the polygon
        int a=1;
        QPointF seg = m_points.at(a)-m_points.at(a-1);

        //the length of the current segment is what remains on current segment
        double segLen = sqrt(seg.x()*seg.x() + seg.y()*seg.y());
        double segRemain = segLen;


        //the current point we are at each iteration
        QPointF curPos = m_points.at(0);

        //the actual collecting iteration
        //todo: replace the condition by a fix number to prevent issues with rounded addition

        //for (int i=0; sumlen<total; i++){
        for (int i=0; i<steps; i++){
            m_scanPoints.append(curPos);
            sumlen+=stepsize;
            //so now move forward along the curve-parameter
            segRemain-=stepsize; //which is basically reducing what was left

            //qInfo() << "appending "<<curPos<< " segLen"<< segLen<<" segRemain"<<segRemain;
            //going forward on that segment
            curPos+=seg*(stepsize/segLen);

            //as long as the remaining lenght on that segment hasn't become negative,
            //we go forward in the polygon list
            //otherwise start from the next point
            while(segRemain<0){
                //step into next segment
                //!Warning: rounding error can cause a jump over the last point throwing a out of range!!!
                if (a+1>=m_points.size()) break;

                a++;
                seg = m_points.at(a)-m_points.at(a-1);
                //the new segments length
                segLen = sqrt(seg.x()*seg.x() + seg.y()*seg.y());

                //the new position starts at the startpoint of the segment
                //added with the remaining length(!its negative)
                curPos=m_points.at(a-1) + seg*(-segRemain/segLen);

                //what is left on the new segment is the already negative segRemain plus the new segments length
                segRemain += segLen;
                //qInfo() << "on a new segment with "<<segLen<<" and remaining "<<segRemain;
            }
        }

        if(m_closed) m_points.removeLast();
    }

}

/**
 * @brief ImgElement::calcScanPoints
 *          determine the geometric positions along the polygon
 * @param pointNumber
 *          if less than one, the scanPoints are basically just the support points,
 *          which might be better for
 */
void ImgElement::calcScanPoints(int pointNumber)
{
    if (pointNumber>1){
        double stepLength;
        if (m_closed){
            stepLength = polyLength()/(pointNumber);
        } else {
            stepLength = polyLength()/(pointNumber-1);
        }
        calcScanPoints(stepLength);

    } else {
        //if no pointNumber is given, we just copy the support points to scan
        m_scanPoints.clear();
        for (QPointF p:m_points){
            m_scanPoints.append(p);
        }
        if(m_closed){

        }

    }

}

QVector<QPointF> &ImgElement::getScanPoints()
{
    return m_scanPoints;
}

QVector<QPointF> &ImgElement::getPoints()
{
    return m_points;
}

void ImgElement::readScanValues(int channelIndex)
{
    //qInfo() << "ImgElement::getValuesAlong("<<steps<<") on image"<<m_parentImage->size()<<" with depth"<<m_parentImage->depth();
    for(QString sn:m_scanValues.keys()){
        m_scanValues[sn].clear();
    }


    if (m_parentImage->depth()==24){
        if (channelIndex==CI_DEFAULT) channelIndex=CI_RGB;
        //qInfo() << "creating 3 scan value vectors...";
        if(channelIndex&CI_RED  ) m_scanValues.insert("red",QVector<int>());
        if(channelIndex&CI_GREEN) m_scanValues.insert("green",QVector<int>());
        if(channelIndex&CI_BLUE ) m_scanValues.insert("blue",QVector<int>());
        if(channelIndex&CI_HUE  ) m_scanValues.insert("hue",QVector<int>());
        if(channelIndex&CI_SAT  ) m_scanValues.insert("sat",QVector<int>());
        if(channelIndex&CI_VAL  ) m_scanValues.insert("val",QVector<int>());

    } else if (m_parentImage->depth()==8){
        channelIndex=CI_VAL;
        m_scanValues.insert("val",QVector<int>());
    }

    for(QPointF p:m_scanPoints){

        QPoint pr=p.toPoint();

        //now go to the image and
        if (m_parentImage->depth()==24){
            QColor c = m_parentImage->pixel(pr);
            if(channelIndex&CI_RED  ) m_scanValues["red"].append( c.red());
            if(channelIndex&CI_GREEN) m_scanValues["green"].append( c.green());
            if(channelIndex&CI_BLUE ) m_scanValues["blue"].append( c.blue());
            if(channelIndex&CI_HUE  ) m_scanValues["hue"].append( c.hue());
            if(channelIndex&CI_SAT  ) m_scanValues["sat"].append( c.saturation());
            if(channelIndex&CI_VAL  ) m_scanValues["val"].append( c.lightness());
        } else if (m_parentImage->depth()==8) {
            QColor c = m_parentImage->pixel(pr);
            m_scanValues["val"].append( c.value() );

        }
    }


}


void ImgElement::readPointValues(int channelIndex)
{
    for(QString sn:m_scanValues.keys()){
        m_scanValues[sn].clear();
    }


    if (m_parentImage->depth()==24){
        if (channelIndex==CI_DEFAULT) channelIndex=CI_RGB;
        //qInfo() << "creating 3 scan value vectors...";
        if(channelIndex&CI_RED  ) m_scanValues.insert("red",QVector<int>());
        if(channelIndex&CI_GREEN) m_scanValues.insert("green",QVector<int>());
        if(channelIndex&CI_BLUE ) m_scanValues.insert("blue",QVector<int>());
        if(channelIndex&CI_HUE  ) m_scanValues.insert("hue",QVector<int>());
        if(channelIndex&CI_SAT  ) m_scanValues.insert("sat",QVector<int>());
        if(channelIndex&CI_VAL  ) m_scanValues.insert("val",QVector<int>());


    } else if (m_parentImage->depth()==8){
        channelIndex=CI_VAL;
        m_scanValues.insert("val",QVector<int>());
    }

    if (m_points.size()>1){

        int i=0;
        //testing:
        //qInfo() << steps << "points along this element of length"<<polyLength();
        for(QPointF p:m_points){
            i++;
            QPoint pr=p.toPoint();

            //now go to the image and
            if (m_parentImage->depth()==24){
                QColor c = m_parentImage->pixel(pr);
                if(channelIndex&CI_RED  ) m_scanValues["red"].append( c.red());
                if(channelIndex&CI_GREEN) m_scanValues["green"].append( c.green());
                if(channelIndex&CI_BLUE ) m_scanValues["blue"].append( c.blue());
                if(channelIndex&CI_HUE  ) m_scanValues["hue"].append( c.hue());
                if(channelIndex&CI_SAT  ) m_scanValues["sat"].append( c.saturation());
                if(channelIndex&CI_VAL  ) m_scanValues["val"].append( c.value());

            } else if (m_parentImage->depth()==8) {
                QColor c = m_parentImage->pixel(pr);
                m_scanValues["val"].append( c.value() );

            }
        }


    } else {
        //only a dot, nothing to get here
        //assert(false);
    }



}

QStringList ImgElement::getAvailableValues()
{
    return m_scanValues.keys();
}

QVector<int> &ImgElement::getScanValues(QString name)
{
    //since we must have a reference the caller has to make sure not to ask for something
    //that the ie don't have
    assert(m_scanValues.contains(name));
    return m_scanValues[name];
}
