#ifndef SCAMMAN_H
#define SCAMMAN_H

#include <QObject>
#include "scam.h"
#include "v4l2cam.h"
#include "basler.h"
#include "dummycam.h"

class LIB_EXPORT SCamMan : public QObject
{
    Q_OBJECT
public:
    explicit SCamMan(QObject *parent = nullptr);

private:
    static QMap<QString, SCam*> named_cams;

public:
    static void load(); // ToDo: remove static
    SCam* getCam(QString name);
    QStringList getCamNames();
    QList<SCam*> getCams();

};

#endif // SCAMMAN_H
