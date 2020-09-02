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


void SCamMan::load(int subSys, QStringList videoLocations)
{
    QList<SCam*> cams;
    if ( subSys & SCM_V4L2){
        cams.append( V4L2Cam::loadCams() );
        qInfo() <<"V4L2Cams loaded, now we have"<<cams.count()<<"cams in the list";
    }

    if (subSys & SCM_IDS) {
        //cams.append( IDS::loadCams(IDS::USB|IDS::ETH) ); //TODO: implement me
        //qInfo() <<"IDS-Cams loaded, now we have"<<cams.count()<<"cams in the list";
    }

    if (subSys & SCM_BASLER) {
        cams.append( Basler::loadCams() );
        qInfo() <<"Basler Cams loaded, now we have"<<cams.count()<<"cams in the list";
    }

    //videofiles in specific location
    if( !videoLocations.isEmpty() ){
        cams.append( SCamVideo::loadCams(videoLocations) );
        qInfo() <<"ffpeg file-cams loaded";
    }

    //test
    if (subSys & SCM_DUMMY ){
        cams.append(new DummyCam());
        cams.append(new DummyCam());
        qInfo() <<"loaded 2 dummy cams";
    }

    //put them all in our map
    for (auto s:cams){
        named_cams.insert(s->readableName,s);
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
