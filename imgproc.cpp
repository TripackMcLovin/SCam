#include "imgproc.h"

uint32_t ImgProc::instanceCounter=0;
QMap<int,int> ImgProc::formatMap={{CV_8UC3,CV_8UC3}};


ImgProc::ImgProc(QImage &img, bool deepCopy, QString name) :
    //QImage(img),
    cv::Mat(img.height(),img.width(),CV_8UC3,img.data_ptr()),
    m_name(name),
    m_description(""),
    m_history(),
    m_scalePixToMM(0.0),
    m_isPersistent(false),
    m_isHidden(false),
    m_imgElements(),
    m_auxMats(),
    matToDraw(""),
    m_thisAsQImg(Q_NULLPTR)
{

    Q_UNUSED(deepCopy)
    if (m_name.isEmpty()) m_name.sprintf("ip_%03d",instanceCounter);

    instanceCounter++;
    IPRegistry::getRegistry()->append(this);
}

ImgProc::ImgProc(cv::Mat &mat, bool deepCopy, QString name) :
    //QImage(mat.data,mat.cols,mat.rows,QImage::Format_ARGB32),
    cv::Mat(mat),
    m_name(name),
    m_description(""),
    m_history(),
    m_scalePixToMM(0.0),
    m_isPersistent(false),
    m_isHidden(false),
    m_imgElements(),
    m_auxMats(),
    matToDraw(""),
    m_thisAsQImg(Q_NULLPTR)
{

    Q_UNUSED(deepCopy)
    if (m_name.isEmpty()) m_name.sprintf("ip_%03d",instanceCounter);

    instanceCounter++;
    IPRegistry::getRegistry()->append(this);
}

ImgProc::ImgProc(uchar *dataPtr, int width, int height, int cvfmt, QString name):
    //QImage(dataPtr, width, height, formatMap.value(cvfmt)),
    cv::Mat(height, width, cvfmt, dataPtr),
    m_name(name),
    m_description(""),
    m_history(),
    m_scalePixToMM(0.0),
    m_isPersistent(false),
    m_isHidden(false),
    m_imgElements(),
    m_auxMats(),
    matToDraw(""),
    m_thisAsQImg(Q_NULLPTR)
{

    assert(formatMap.contains(cvfmt));
    instanceCounter++;
    IPRegistry::getRegistry()->append(this);
}

ImgProc::ImgProc(int width, int height, int cvfmt, QString name):
    //QImage(static_cast<uchar*>(Q_NULLPTR),width,height,fmt),
    cv::Mat(height,width,cvfmt),
    m_name(name),
    m_description(""),
    m_history(),
    m_scalePixToMM(0.0),
    m_isPersistent(false),
    m_isHidden(false),
    m_imgElements(),
    m_auxMats(),
    matToDraw(""),
    m_thisAsQImg(Q_NULLPTR)
{
   //since we first instanciate us as a QImage, we can only later set the pointer to the data from the internal cv::Mat to that
    //((QImage*)this)->data_ptr()=(uchar*)(this->data);

    if (m_name.isEmpty()) m_name.sprintf("ip_%03d",instanceCounter);

    instanceCounter++;
    IPRegistry::getRegistry()->append(this);
}

ImgProc::ImgProc(ImgProc *that, QString name):
    cv::Mat(that->height(),that->width(),that->type()),
    m_name(name),
    m_description(""),
    m_history(),
    m_scalePixToMM(0.0),
    m_isPersistent(false),
    m_isHidden(false),
    m_imgElements(),
    m_auxMats(),
    matToDraw(""),
    m_thisAsQImg(Q_NULLPTR)
{
    Q_UNUSED(that)
    //TODO: how to handle the deepcopy in cv
    that->copyTo(*this);
    instanceCounter++;
    IPRegistry::getRegistry()->append(this);
    m_description.sprintf("copy_of_%s_%d03",that->m_description.toLatin1().data(),instanceCounter);
}

ImgProc::ImgProc(QString filepath, bool rgb, bool deepCopy, QString name):
    cv::Mat(),
    m_name(name),
    m_description(""),
    m_history(),
    m_scalePixToMM(0.0),
    m_isPersistent(false),
    m_isHidden(false),
    m_imgElements(),
    m_auxMats(),
    matToDraw(""),
    m_thisAsQImg(Q_NULLPTR)
{
    cv::Mat mat = cv::imread(filepath.toStdString());

    mat.copyTo(*this);

    //qInfo() << this.type();
    //if(this->type() == 16) cv::cvtColor(*this, *this, cv::COLOR_BGR2RGB);

    if(rgb) cv::cvtColor(*this, *this, cv::COLOR_BGR2RGB);
    else cv::cvtColor(*this, *this, cv::COLOR_BGR2GRAY);

    Q_UNUSED(deepCopy)
    if (m_name.isEmpty()) m_name.sprintf("ip_%03d",instanceCounter);

    instanceCounter++;
    IPRegistry::getRegistry()->append(this);

    m_isReady=false;
}

ImgProc::~ImgProc()
{
    instanceCounter--;
    IPRegistry::getRegistry()->remove(this);
}

QString ImgProc::getName()
{
    return m_name;
}

QString ImgProc::getDescription()
{
    if (m_description.isEmpty()) return QString("No description available");
    return m_description;
}

void ImgProc::appendHistory(QString entry)
{
    m_history.append(entry);
    this->m_description.append(entry);
    this->m_description.append("\n\r");
}

QList<QString> *ImgProc::getHistory()
{
    return &m_history;
}

QString ImgProc::getHistoryEntry(int entry)
{
    return m_history.at(entry);
}

void ImgProc::setPersistent()
{
    m_isPersistent=true;
}

bool ImgProc::isPersistent()
{
    return m_isPersistent;
}

void ImgProc::setDescription(QString descr)
{
    m_description=descr;
}

void ImgProc::setImage(QString path)
{
    this->setReady(false);
    cv::Mat mat = cv::imread(path.toStdString());
    mat.copyTo(*this);
    //emit contentUpdate(this);
    setReady();
}

QImage *ImgProc::getImage()
{
    if (m_thisAsQImg==Q_NULLPTR){
        int bpl=static_cast<int>(this->step);
        qInfo() << "bpl="<<bpl;

        if(this->type() == CV_8UC1){
            m_thisAsQImg = new QImage(data,width(),height(),bpl,QImage::Format_Grayscale8);
        } else{
            m_thisAsQImg = new QImage(data,width(),height(),bpl,QImage::Format_RGB888);
        }
    }

    return m_thisAsQImg;
}

void ImgProc::copyImageFrom(ImgProc *that)
{
    //ImgProc is still a cv::Mat
    //in case of unfitting formats!! (maybe it is included in copyTo() ?????
    /*
    this->deallocate();
    this->create(that->rows,
                 that->cols,
                 that->type());
    */

    if (this->type()==that->type()){

        if (this->size == that->size){
            m_isReady=false;
            that->copyTo(*this);
        } else {
            //scale?!?
            //for now we assume same ratio
            double r=static_cast<double>(this->width())/that->width();

#ifndef CV_INTER_NN
#define CV_INTER_NN cv::INTER_NEAREST
#endif
            cv::resize(*that, *this, cv::Size(),r,r,CV_INTER_NN);


        }
    } else {
        qDebug() << "missing implementation for ImgProc::copyImageFrom() with different formats!";
    }
}

void ImgProc::rainbowFill()
{
    m_isReady=false;

    uint8_t r;
    uint8_t g;
    uint8_t b;

    double cf=(10.+(std::rand()%300))/this->cols;
    double cb=200./this->rows;

    cv::Vec3b pixel;

    int ra=std::rand()%255;

    for (int y=0;y<this->rows; y++){
        for (int x=0; x<this->cols; x++){
            uint8_t s = ( ( ((x>>7)&0x01) + ((y>>7)&0x01) )&0x01)?180:255;
            ColorSpaces::hsv2rgb((static_cast<u_int8_t>(x*cf)+ra)&0xFF,s,255-static_cast<uint8_t>(y*cb),&r,&g,&b);
            pixel[0]=r;
            pixel[1]=g;
            pixel[2]=b;
            this->at<cv::Vec3b>(cv::Point(x,y)) = pixel;
        }
    }

    this->appendHistory("rainbow filled");

    ImgElement *elem = new ImgElement(this->getImage(),"testLine");
    elem->appendPoint(QPoint(10,10));
    elem->appendPoint(QPoint(80,10));
    elem->appendPoint(QPoint(150,300));
    elem->appendPoint(QPoint(180,410));
    elem->appendPoint(QPoint(230,330));
    elem->appendPoint(QPoint(300,130));
    //forcing the readout along that line
    elem->readScanValues(50);

    m_imgElements.insert(elem->getName(),elem);


    elem = new ImgElement(this->getImage(),"straight");
    elem->appendPoint(QPoint(100,400));
    elem->appendPoint(QPoint(400,410));
    elem->readScanValues(200);

    m_imgElements.insert(elem->getName(),elem);

    this->appendHistory("sample elements included and scanned");

    //emit contentUpdate(this);
    setReady();

}

void ImgProc::setReady(bool ready)
{
    if (ready){
        if (!m_isReady){
            m_isReady=true;
            //emit contentUpdate(this);
            emit gotReady(this);
        }
    } else {
        m_isReady=false;
    }
}

bool ImgProc::isReady()
{
    return m_isReady;
}


QStringList ImgProc::getElementNames()
{
    return m_imgElements.keys();
}

ImgElement *ImgProc::getElement(QString name)
{
    if(m_imgElements.contains(name)) return m_imgElements.value(name);
    else return Q_NULLPTR;
}

QStringList ImgProc::getAuxMatNames()
{
    return m_auxMats.keys();
}

/*
ImgProc *ImgProc::getLinkedProcessor(QString ipName)
{
    return IPRegistry::getRegistry()->getProcessor(m_linkedIPs[ipName]);
}

void ImgProc::linkProcessor(QString linkname, ImgProc *processor)
{
    m_linkedIPs[linkname]=processor->getName();
}
*/

void ImgProc::drawToScene(QGraphicsScene *scene, bool drawElems)
{
    int w = static_cast<int>(scene->sceneRect().width());
    int h = static_cast<int>(scene->sceneRect().height());

    double rScene=static_cast<double>(w)/h;
    double rImg=static_cast<double>(width())/height();

    //decide on scene ratio the minor fit (either black strips on top/bottom or l/r)

    QImage imgSelect;
    if (matToDraw.isEmpty()){
        imgSelect = *getImage();
    } else {
        cv::Mat *m=m_auxMats[matToDraw];
        int bpl=static_cast<int>(m->step);
        //qInfo() << "bpl="<<bpl;

        if(m->type() == CV_8UC1){
            imgSelect = QImage(m->data,m->cols,m->rows,bpl,QImage::Format_Grayscale8);
        } else{
            imgSelect = QImage(m->data,m->cols,m->rows,bpl,QImage::Format_RGB888);
        }

    }

    if(w!=this->width()){
        QImage scaled;
        if (rScene>rImg){
            scaled=imgSelect.scaledToHeight(h);
        } else {
            scaled=imgSelect.scaledToWidth(w);
        }
        QPixmap pm=QPixmap::fromImage(scaled);
        scene->addPixmap(pm);
    } else {
        scene->addPixmap(QPixmap::fromImage(imgSelect));
    }



    if (drawElems){
        for(ImgElement *elem: m_imgElements){
            elem->drawToScene(scene);
        }
    }
}

void ImgProc::setMatToDraw(QString matName)
{
    if(m_auxMats.contains(matName)) matToDraw = matName;
    else matToDraw="";
}

int ImgProc::width()
{
    return cols;
}

int ImgProc::height()
{
    return rows;
}

ImgProc::IP_format ImgProc::getFormat()
{
    return m_ipFormat;
}

void ImgProc::addElement(ImgElement *element)
{
    m_isReady=false;
    //two options: either delete the existing one
    QString name=element->getName();

    if (m_imgElements.contains(name)){
        ImgElement *e=m_imgElements.take(name);
        delete e;
    }
    //or just do stuff with the elements name
    //if (m_imgElements.contains(element->getName())) delete m_imgElements.value(element->getName());


    m_imgElements.insert(element->getName(),element);

}

void ImgProc::addElements(QList<ImgElement *> elements)
{
    m_isReady=false;
    for(ImgElement* e: elements){
        if (m_imgElements.contains(e->getName())) delete m_imgElements.value(e->getName());
        m_imgElements.insert(e->getName(),e);
    }

}

void ImgProc::addAuxMat(QString name, cv::Mat *mat)
{
    m_auxMats.insert(name, mat);
}

void ImgProc::deleteElement(QString name)
{
    if(m_imgElements.contains(name)){
        delete (m_imgElements.take(name));
    }
    //what about signals etc?
}

void ImgProc::clearElements()
{
    m_isReady=false;

    for(ImgElement *elem : (m_imgElements.values())){
        delete elem;
    }
    m_imgElements.clear();

}

void ImgProc::setScale(double pixToMM)
{
    if (pixToMM>0.0) m_scalePixToMM=pixToMM;
}

void ImgProc::unmarkAllElements()
{
    for(ImgElement *e: m_imgElements){
        e->setMarked(false);
    }
}

IPRegistry* IPRegistry::registry = Q_NULLPTR;

IPRegistry *IPRegistry::getRegistry()
{
    if (registry==Q_NULLPTR) registry=new IPRegistry();
    return registry;
}

int IPRegistry::size()
{
    return m_map.size();
}

ImgProc *IPRegistry::getProcessor(int index)
{
    return m_map[m_map.keys().at(index)];
}

ImgProc *IPRegistry::getProcessor(QString name)
{
    if(!m_map.contains(name)) qInfo() << "IPRegistry::getProcessor(): found no ImgProc with name" << name;
    return m_map[name];
}

IPRegistry::IPRegistry() :
    QObject(Q_NULLPTR),
    m_map()
{
}


void IPRegistry::append(ImgProc *proc)
{
    //m_processorList.append(proc);
    qInfo() << "IPRegistry::append("<<proc->getName()<<")";
    m_map.insert(proc->getName(), proc);
    //m_processorList.insert(proc->getName(), proc);
    emit newElement(proc);
}

void IPRegistry::remove(QString name)
{
    if(m_map.contains(name)){
        ImgProc* p = m_map[name];
        remove(p);
    }
}

void IPRegistry::remove(ImgProc* p)
{

    QList<QString> aliases = m_map.keys(p);

    for(QString str : aliases){

        if(p->isPersistent()){

            if(str != p->getName()){

                this->remove(str);
                emit elementRemoved(p, str);
            }

        } else{

            this->remove(str);
            emit elementRemoved(p, str);
        }


    }

    if(!p->isPersistent()) delete p;
}



void IPRegistry::clear()
{
   QList<QString> names = m_map.keys();

   for(QString str : names){
       remove(str);
   }
}

ImgProc *IPRegistry::operator[](QString name)
{
    if(m_map.contains(name)){
        return m_map[name];
    } else {
        return Q_NULLPTR;
    }
}

void IPRegistry::append(QString name, ImgProc *p)
{
    m_map[name]=p;
    emit newElement(p);
}
