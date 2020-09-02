#ifndef SCAMMAN_H
#define SCAMMAN_H

#include <QObject>
#include "scam.h"
#include "v4l2cam.h"
#include "basler.h"
#include "dummycam.h"
#include "ffmpeg.h"

enum SubSys{
    SCM_NONE=0,
    SCM_DUMMY=1,
    SCM_IDS=1<<1,
    SCM_BASLER=1<<2,
    SCM_V4L2=1<<3,
    SCM_FFMPEG=1<<4 //should not be regarded as long as a video location is given
};


class LIB_EXPORT SCamMan : public QObject
{
    Q_OBJECT

public: //object is only available through the static getter
    explicit SCamMan(int subSys=SCM_IDS|SCM_BASLER|SCM_V4L2,
                     QStringList videoLocations={},
                     QObject *parent = nullptr);
    static SCamMan* getCamMan();

private:
    static QMap<QString, SCam*> named_cams;
    static SCamMan* theMan;

public:
    void load(int subSys, QStringList videoLocations={}); // ToDo: remove static
    SCam* getCam(QString name);
    QStringList getCamNames();
    QList<SCam*> getCams();

};

#endif // SCAMMAN_H
