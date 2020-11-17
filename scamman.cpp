#include "scamman.h"

QMap<QString, SCam*> SCamMan::named_cams;
SCamMan* SCamMan::theMan = Q_NULLPTR;


SCamMan::SCamMan(int subSys, QStringList videoLocations, QObject *parent) : QObject(parent)
{
    //this->load(subSys, videoLocations);
    theMan=this;
}

SCamMan *SCamMan::getCamMan()
{
    //at least one camMan instance needs to be loade before getCamMan should be called
    //this way, one can give parameters like video location and so on
    assert(theMan!=Q_NULLPTR);
    //theMan = new SCamMan();

    return theMan;
}

/**
 * @brief SCamMan::load all cams specified, acessible by their readable name in the named_cams map through
 *                  getCam(id)
 * @param subSys is a bitfilter for the cam-subsystems SCM_V4L2, SCM_IDS, SCM_BASLER, SCM_FFMPEG, SCM_DUMMY
 * @param videoLocations a list of locations to scan for FFMPEG compatible video files, optional
 */
void SCamMan::load(int subSys, QStringList videoLocations)
{

    if ( subSys & SCM_V4L2){
        QList<SCam*> cams;
        cams.append( V4L2Cam::loadCams() );
        for(auto c:cams){
            named_cams.insert(QString("v4l2_").append(c->readableName),c);
        }
        qInfo() <<"V4L2Cams loaded, now we have"<<cams.count()<<"cams in the list";
    }

    if (subSys & SCM_IDS) {
        QList<SCam*> cams;
        //cams.append( IDS::loadCams(IDS::USB|IDS::ETH) ); //TODO: implement me
        //qInfo() <<"IDS-Cams loaded, now we have"<<cams.count()<<"cams in the list";
        for(auto c:cams){
            named_cams.insert(QString("ids_").append(c->readableName),c);
        }
    }

    if (subSys & SCM_BASLER) {
        QList<SCam*> cams;
        cams.append( Basler::loadCams() );
        qInfo() <<"Basler Cams loaded, now we have"<<cams.count()<<"cams in the list";
        for(auto c:cams){
            named_cams.insert(QString("basler_").append(c->readableName),c);
        }
    }

    //videofiles in specific location, readable name is the filename
    if (subSys & SCM_FFMPEG && !videoLocations.isEmpty() ){
        QList<SCam*> cams;
        cams.append( SCamVideo::loadCams(videoLocations) );
        qInfo() <<"ffpeg file-cams loaded";
        for(auto c:cams){
            named_cams.insert(QString("ffmpeg_").append(c->readableName),c);
        }
    }

    //test
    if (subSys & SCM_DUMMY ){
        QList<SCam*> cams;
        cams.append(new DummyCam());
        cams.append(new DummyCam());
        qInfo() <<"loaded dummy cams";
        for(auto c:cams){
            named_cams.insert(QString("dummy_").append(c->readableName),c);
        }
    }

    qInfo() << "Available cams in this manager:";
    for (QString key:named_cams.keys()){
            qInfo() << "    "<<key<<":"<<named_cams[key]->internalName;
    }


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
