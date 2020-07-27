#include "scam.h"


CamSettings::CamSettings()
{

}

CamSettings::~CamSettings()
{

}


SCam::SCam(QString readableName, QString internalName, uint8_t numBufs, QObject *parent) : QObject(parent),
    readableName(readableName),
    internalName(internalName),
    m_numBuf(numBufs),
    m_index(0),
   // prevIndex(0),
    m_status(NOT_AVAILABLE),
    m_worker(Q_NULLPTR),
    m_thread(Q_NULLPTR),
    m_opMutex()
{
    activateWorker();
}

SCam::~SCam()
{
    deactivateWorker();
}


bool SCam::isReady()
{
    return m_status==ACTIVE;
}

QImage *SCam::getNewestImage()
{
    return m_imgProcessors[m_index]->getImage();
}

ImgProc *SCam::getNewestIP()
{
    return m_imgProcessors[m_index];
}

QString SCam::getStatusString()
{
    switch (m_status) {
    case NOT_AVAILABLE: return "Not Available";
    case CLOSED: return "Closed";
    case OPEN: return "Open";
    case ACTIVE: return "Active";
    case CAPTURING: return "Capturing";
    case ERROR: return "Error";
    }
}



void SCam::setState(SCam::SC_STATE newStatus)
{
    if (newStatus!=m_status){
        m_status = newStatus;
        emit stateChanged(this);
    }
}

void SCam::activateWorker()
{
    //qInfo() << "SCam::activateWorker()";
    if (m_thread==Q_NULLPTR) m_thread=new QThread();
    if (m_worker==Q_NULLPTR) m_worker = new SCamWorker(this);

    m_worker->moveToThread(m_thread);
    //qInfo() << "    worker and thread created, now connecting ...";

    //towards worker
    connect(this, SIGNAL(openSig()), m_worker, SLOT(opening()),Qt::QueuedConnection);
    connect(this, SIGNAL(activateSig()), m_worker, SLOT(activating()),Qt::QueuedConnection);
    connect(this, SIGNAL(captureSig()), m_worker, SLOT(capturing()),Qt::QueuedConnection);

    //from worker (signal becomes signal), Queued to cross thread boundaries) SIGNAL forwarding
    connect(m_worker, SIGNAL(newImage()), this, SIGNAL(newImage()),Qt::QueuedConnection );
    connect(m_worker, SIGNAL(captureFailed(QString)), this, SIGNAL(captureFailed(QString)), Qt::QueuedConnection);

    //qInfo() << "    ...start...";
    m_thread->start();
    //qInfo() << "    ...aaand done!";

}

void SCam::deactivateWorker()
{
    //todo: do vice versa
    m_thread->exit();
    m_thread->wait();

    //disconnect all the 5 sigs between worker and object
    disconnect(m_worker, SIGNAL(newImage()), this, SIGNAL(newImage()));
    disconnect(m_worker, SIGNAL(captureFailed(QString)), this, SIGNAL(captureFailed(QString)));

    disconnect(this, SIGNAL(openSig()), m_worker, SLOT(opening()));
    disconnect(this, SIGNAL(activateSig()), m_worker, SLOT(activating()));
    disconnect(this, SIGNAL(captureSig()), m_worker, SLOT(capturing()));


    delete m_thread;
    m_thread=Q_NULLPTR;

    delete m_worker;
    m_worker=Q_NULLPTR;
}

void SCam::open()
{
    emit openSig();
}

void SCam::activate()
{
    emit activateSig();
}

void SCam::capture()
{
    m_opMutex.lock();
    emit captureSig();
}

void SCam::deactivate()
{

}

void SCam::close()
{

}

bool SCam::isActive()
{
    return (m_status == ACTIVE);
}

bool SCam::isCapturing()
{
    return (m_status == CAPTURING);
}


SCamWorker::SCamWorker(SCam *cam):
    m_cam(cam)
{

}

void SCamWorker::opening()
{
    m_cam->open_internal();
}

void SCamWorker::activating()
{
    m_cam->activate_internal();
}

void SCamWorker::capturing()
{
    m_cam->capture_internal();
}

CamFmt::CamFmt(int w, int h, QString name, int bpp):
    width(w),
    height(h),
    bytesPerPixel(bpp),
    name(name),
    internalID(0)
{

}
