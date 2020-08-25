#include "scamman.h"

QMap<QString, SCam*> SCamMan::named_cams;
SCamMan* SCamMan::theMan = Q_NULLPTR;


SCamMan::SCamMan(QObject *parent) : QObject(parent)
{
    this->load();
}

SCamMan *SCamMan::getCamMan()
{
    if (theMan==Q_NULLPTR)
        theMan = new SCamMan();

    return theMan;
}


void SCamMan::load()
{
    QList<SCam*> cams;
//    cams.append( V4L2Cam::loadCams() );

    qInfo() <<"V4L2Cams loaded, now we have"<<cams.count()<<"cams in the list";
    //ids

    //basler
    cams.append( Basler::loadCams() );


    //videofiles in specific location


    for (auto s:cams){
        named_cams.insert(s->readableName,s);
    }

    //test
    SCam *nc=new DummyCam();
    SCam *nc2=new DummyCam();
    named_cams.insert(nc->readableName,nc);
    named_cams.insert(nc2->readableName,nc2);
}

SCam *SCamMan::getCam(QString name)
{
    return named_cams[name];
}

QStringList SCamMan::getCamNames()
{
    return named_cams.keys();
}

QList<SCam *> SCamMan::getCams()
{
    return named_cams.values();
}
