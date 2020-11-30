#include "idscam.h"

int IDSCam::sICount=0;

IDSCam::IDSCam(uint8_t numBuf) :
    SCam(QString("ids_")+sICount,QString("ids_")+sICount,numBuf),
    ret(0),
    settings(Q_NULLPTR),
    hCam(0),
    nColorMode(0),
    nBitsPerPixel(0),
    //nSizeX(0),
    //nSizeY(0),
    mCamInfo(),
    mSensorInfo(),
    ppcImgMem(),
    pid(numBuf)
{
    qInfo() <<"IDSCam::IDSCam("<<numBuf<<")";

    int ret;

    ret = is_InitCamera( &hCam, 0 );

    if ( ret==IS_SUCCESS ){
        qInfo() << "    ueye::isInitCamera() returns IS_SUCCESS, handle(hCam)="<<hCam;

        CAMINFO *pInfo = (CAMINFO*) new BYTE[sizeof (UEYE_CAMERA_INFO)];
        ret = is_GetCameraInfo(hCam,pInfo);

        if (ret==IS_SUCCESS){
            QString serNum = pInfo->SerNo;
            qInfo() << "     serNum:"<<serNum<<" selectID"<< pInfo->Select;

            readableName = QString("ids_")+serNum;

        } else {
            qInfo() << "failed to get camera that fits this handle "<<hCam;
            if ( ret==IS_INVALID_CAMERA_HANDLE) qInfo() << "    invalid camera handle";
        }


        is_GetSensorInfo( hCam, &mSensorInfo );
        qInfo() << "    sensor-info";
        qInfo() << "        sensor-id  :"<<mSensorInfo.SensorID;
        qInfo() << "        sensor-name:"<<mSensorInfo.strSensorName;
        qInfo() << "        pixel-size :"<<mSensorInfo.wPixelSize;
        qInfo() << "        r/g/b-gain :"<<mSensorInfo.bRGain<<mSensorInfo.bGGain<<mSensorInfo.bBGain;
        qInfo() << "        master-gain:"<<mSensorInfo.bMasterGain;
        qInfo() << "        color-mode :"<<(int)mSensorInfo.nColorMode;
        qInfo() << "        max w/h    :"<<mSensorInfo.nMaxWidth<<mSensorInfo.nMaxHeight;

        nMaxWidth = mSensorInfo.nMaxWidth;
        nMaxHeight = mSensorInfo.nMaxHeight;




    }

    m_imageSize.setWidth(640);
    m_imageSize.setHeight(480);

    m_opMutex.unlock();
    setState(CLOSED);
}

IDSCam::~IDSCam()
{
    qInfo() << "SLabSIDSCam::~SLabSIDSCam()";

    if ( hCam != 0 ){
        is_StopLiveVideo( hCam, IS_WAIT );
        for (int i=0;i<m_numBuf; i++){
            ret = is_FreeImageMem( hCam, ppcImgMem[i], pid[i] );
            qInfo() << "    freeing buffers["<<i<<"] : "<<((ret==IS_SUCCESS)?"OK":"Failed:");
        }
        is_ExitCamera( hCam );
    }
    qInfo() << "...deleted cam";


    //for (int i=0;i<numBuf; i++) imgs[i]=QImage();
    for (ImgProc *p:m_imgProcessors) delete p;

    m_imgProcessors.clear();

    qInfo() << "...deleted buffers";


}

bool IDSCam::open_internal()
{
    qInfo() << "SLabIDSSCam::openCam(), hCam="<<hCam;

    m_opMutex.lock();

    /*
    ret = is_InitCamera( &hCam, 0 );

    if ( ret==IS_SUCCESS ){
        qInfo() << "    ueye::isInitCamera() returns IS_SUCCESS";
        is_GetSensorInfo( hCam, &mSensorInfo );
        qInfo() << "    sensor-info";
        qInfo() << "        sensor-id  :"<<mSensorInfo.SensorID;
        qInfo() << "        sensor-name:"<<mSensorInfo.strSensorName;
        qInfo() << "        pixel-size :"<<mSensorInfo.wPixelSize;
        qInfo() << "        r/g/b-gain :"<<mSensorInfo.bRGain<<mSensorInfo.bGGain<<mSensorInfo.bBGain;
        qInfo() << "        master-gain:"<<mSensorInfo.bMasterGain;
        qInfo() << "        color-mode :"<<(int)mSensorInfo.nColorMode;
        qInfo() << "        max w/h    :"<<mSensorInfo.nMaxWidth<<mSensorInfo.nMaxHeight;

        nMaxWidth = mSensorInfo.nMaxWidth;
        nMaxHeight = mSensorInfo.nMaxHeight;

        //apply that!

    */
    if (hCam) { //is_InitCamera seems to have worked

        m_imageSize.setWidth(nMaxWidth);
        m_imageSize.setHeight(nMaxHeight);

        //nColorMode = IS_CM_RGBA8_PACKED;
        //nBitsPerPixel=32;

        nColorMode = IS_CM_RGB8_PACKED;
        //nColorMode = IS_CM_RGB8_PLANAR;
        nBitsPerPixel=24;

        ret=is_SetColorMode( hCam, nColorMode );
        qInfo() << "    ueye::is_SetColorMode: "<<((ret==IS_SUCCESS)?"OK":"Failed:")<<ret;

        //API says this is not supported in Linux
        //ret=is_GetColorDepth( hCam, &nBitsPerPixel, &nColorMode );
        //qInfo() << "is_GetColorDepth: "<<((ret==IS_SUCCESS)?"OK":"Failed:")<<ret;
        qInfo() << "        bpp      :"<<nBitsPerPixel;
        qInfo() << "        colorMode:"<<nColorMode;


        //knowning the image formats
        UINT count;
        UINT bytesNeeded = sizeof(IMAGE_FORMAT_LIST);
        ret = is_ImageFormat(hCam, IMGFRMT_CMD_GET_NUM_ENTRIES, &count, sizeof(count));
        qInfo() << "    camera has"<<count<<"supported formats: ";
        bytesNeeded += (count - 1) * sizeof(IMAGE_FORMAT_INFO);
        void* ptr = malloc(bytesNeeded);

        IMAGE_FORMAT_LIST* pformatList = (IMAGE_FORMAT_LIST*) ptr;
        pformatList->nSizeOfListEntry = sizeof(IMAGE_FORMAT_INFO);
        pformatList->nNumListElements = count;
        ret = is_ImageFormat(hCam, IMGFRMT_CMD_GET_LIST, pformatList, bytesNeeded);

        IMAGE_FORMAT_INFO formatInfo;
        for (uint i = 0; i < count; i++){
            formatInfo = pformatList->FormatInfo[i];
            qInfo() << "      "<<i<<":"<<formatInfo.nWidth<<"x"<<formatInfo.nHeight<<":"<<formatInfo.strFormatName;
        }

        //nSizeX = previewSize().width();
        //nSizeY = previewSize().height();

        //int bufferSize = nSizeX*nSizeY*(4);
        //buffer = (char*)malloc(bufferSize);
        //qInfo() << "allocated buffer:"<<bufferSize<<"bytes";

        ret = is_SetDisplayMode( hCam, IS_SET_DM_DIB );
        qInfo() << "    setting up display mode DIB:"<<((ret==IS_SUCCESS)?"OK":"Failed");


        qInfo() << "    let there be image memory allocated"<<m_numBuf<<"times...";
        setResolution(m_imageSize);
        for (int i=0;i<m_numBuf; i++){
            char *ptr; //don't know why, but otherwise pass it directly causes the QVector to loose that shit after this call
            ret = is_AllocImageMem(hCam, m_imageSize.width(), m_imageSize.height(), nBitsPerPixel, &ptr, &(pid[i]) );
            qInfo() << "    buffer["<<i<<"]: "<<((ret==IS_SUCCESS)?"OK":"Failed");
            ppcImgMem.push_back(ptr);
        }

        for (int i=0;i<m_numBuf; i++)
            m_imgProcessors.append( new ImgProc((uchar*)(ppcImgMem[i]),
                                         m_imageSize.width(),
                                         m_imageSize.height(),
                                         QImage::Format_RGB888) );
        qInfo() << "    ...created all QImage objects";

        setState(OPEN);

        qInfo() << "SLabIDSSCam::openCam() ... done!";

        m_opMutex.unlock();
        return true;

    } else {

        qInfo() << "    ueye::isInitCamera() was not successfull";

        m_opMutex.unlock();
        return false;
    }
}

bool IDSCam::activate_internal()
{
    qInfo() << "SLabSIDSCam::activate()...";
    qInfo() << "    cam status ="<<this->getStatusString();

    if (m_status==CLOSED) this->open_internal();

    qInfo() << "    cam status ="<<this->getStatusString();

    if( m_status==OPEN) {

        m_opMutex.lock();

        setState(ACTIVE);

        m_opMutex.unlock();
        return true;
    } else {

        return false;
    }

}


bool IDSCam::deactivate_internal()
{
    if (m_status==ACTIVE){
        setState(OPEN);
        return true;
    }
    return false;
}

bool IDSCam::close_internal()
{
    setState(CLOSED);
    return true;
}


QList<SCam *> IDSCam::loadCams(int subSysFlag)
{
    QList<SCam *> list;

    qInfo() << "IDSCam::loadCams()";

    int camNum;

    int ret;
    ret = is_GetNumberOfCameras(&camNum);

    if (ret == IS_SUCCESS){
        for (int iCam=0; iCam<camNum; iCam++){
            list.append(new IDSCam(2)); //it is creating its name by its serial number so it can be found by that ("ids_123468"
        }
    }

    return list;
}

void IDSCam::setResolution(QSize &size)
{
    qInfo() << "SLabSIDSCam::setResolution() "<<size.width()<<"x"<<size.height();

    int ret;
    IS_SIZE_2D imageSize;

    imageSize.s32Width = size.width();
    imageSize.s32Height = size.height();


    /* Sensor-scaler sucks, it is not making anything faster
    if (size.width()==nMaxWidth && size.height()==nMaxHeight){
        //no scaling
        ret = is_SetSensorScaler( hCam, IS_DISABLE_SENSOR_SCALER, 1.0);
        if (ret==IS_SUCCESS) {
            qInfo() << "    is_SetSensorScaler(IS_DISABLE_SENSOR_SCALER) returned OK!";
        } else {
            qInfo() << "    is_SetSensorScaler(IS_DISABLE_SENSOR_SCALER) failed with "<<ret;
        }
    } else{
        SENSORSCALERINFO Info;

        ret = is_GetSensorScalerInfo (hCam, &Info, sizeof(Info));

        qInfo() << "    SENSORSCALERINFO:";
        qInfo() << "        minFactor="<<(double)(Info.dblMinFactor);
        qInfo() << "        factIncr= "<<(double)(Info.dblFactorIncrement);


        double scale = (double)nMaxWidth/(double)size.width();

        qInfo() << "    our scale would be "<<scale;

        ret = is_SetSensorScaler( hCam, IS_ENABLE_SENSOR_SCALER, scale);
        if (ret==IS_SUCCESS) {
            qInfo() << "    is_SetSensorScaler(IS_ENABLE_SENSOR_SCALER) returned OK!";
        } else {
            qInfo() << "    is_SetSensorScaler(IS_ENABLE_SENSOR_SCALER) failed with "<<ret;
        }
    }*/

    IMAGE_FORMAT_INFO formatInfo;

    formatInfo.nFormatID=1;

    ret = is_ImageFormat(hCam, IMGFRMT_CMD_SET_FORMAT, &formatInfo.nFormatID, sizeof(formatInfo.nFormatID));

    if (ret==IS_SUCCESS) {
        qInfo() << "    is_ImageFormat(IMGFRMT_CMD_SET_FORMAT, FMT="<<formatInfo.nFormatID<<") returned OK!";
    } else {
        qInfo() << "    is_ImageFormat(IMGFRMT_CMD_SET_FORMAT, FMT="<<formatInfo.nFormatID<<") failed with "<<ret;
    }


    //this just sets the area of interest, just to expand it
    ret = is_AOI( hCam, IS_AOI_IMAGE_SET_SIZE, (void*)&imageSize, sizeof (imageSize) );
    //maybe the given size was readjusted, so we write them back?!?
    size.setWidth(imageSize.s32Width);
    size.setHeight(imageSize.s32Height);

    if (ret==IS_SUCCESS) {
        qInfo() << "is_AOI() returned OK!";
    } else {
        qInfo() << "is_AOI() failed with "<<ret;
    }

}




bool IDSCam::capture_internal()
{
    m_index++;
    if (m_index>=m_numBuf) m_index=0;
    ret = is_SetImageMem( hCam, ppcImgMem[m_index], pid[m_index] );


    if (ret!=IS_SUCCESS){
        qInfo() << "    failed to set memory to m_index="<<m_index;
    }

    //qInfo() << "    trigger now...";
    ret = is_FreezeVideo(hCam, IS_WAIT );

    if (ret==IS_SUCCESS){
        //qInfo() << "    aquire returns IS_SUCCESS";
        //the image is now available inside the buffer or the QImages via getLastImage()

        m_opMutex.unlock();
        return true;

    } else if (ret==IS_TRANSFER_ERROR) {
        qInfo() << "    is_FreezeVideo(hCam, IS_WAIT) failed: transfer error";
        emit captureFailed("IDS: transfer error, may the USB connection has issues.");

    } else {
        qInfo() << "    is_FreezeVideo(hCam, IS_WAIT) failed with"<<ret;
        emit captureFailed(QString("unknown IDS error code:")+ret);
    }

    m_opMutex.unlock();
    return false;
}

bool IDSCam::setBrightness(double b)
{
    Q_UNUSED(b)
    return true;
}

bool IDSCam::setContrast(double c)
{
    Q_UNUSED(c)
    return true;
}

void IDSCam::setFormat(int formatIndex)
{
    Q_UNUSED(formatIndex);
}
