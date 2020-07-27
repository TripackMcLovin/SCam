#include "basler.h"

long Basler::s_instanceCount=1000;

Basler::Basler(Pylon::CDeviceInfo &deviceInfo) :
    SCam(QString::fromUtf8(deviceInfo.GetFriendlyName().c_str()),
         QString::fromUtf8(deviceInfo.GetSerialNumber().c_str()), 0),
    m_bufferContext(s_instanceCount),
    m_deviceInfo(deviceInfo),
    m_CInstantCamera(Q_NULLPTR)
{
    //we must check the camera properties (resolution and color) before we allocate our imgProcs
    //this->open_internal();
    //this->close_internal();
    s_instanceCount+=1000;
}

Basler::~Basler()
{
    this->close_internal();
}

bool Basler::open_internal()
{
    //    if (m_status != CLOSED)
    //        return false;
    m_CInstantCamera = new Pylon::CInstantCamera(Pylon::CTlFactory::GetInstance().CreateDevice(m_deviceInfo));
    m_CInstantCamera->MaxNumBuffer = m_numBuf;

    m_CInstantCamera->SetBufferFactory(this,Pylon::Cleanup_None);

    qInfo() << "SLabSBasler::open() on " << readableName;

    // Open the camera for accessing the parameters.
    m_CInstantCamera->Open();
    setState(OPEN);

    // Get and Set Parameters
    GenApi::INodeMap& nodemap = m_CInstantCamera->GetNodeMap();

    //    GenApi::NodeList_t nodes;
    //    nodemap.GetNodes(nodes);
    //    for (auto &n : nodes){
    //        qInfo() << n->GetName() << " " << n->GetDisplayName() << " " << n->GetDescription();
    //    }

    // Temperatur
    double temp;
    std::string tempState;
    if ( getDeviceTemperature(temp, tempState) ){
        qInfo() << "Temperature " << temp << " C (" << tempState.c_str() << ")";
    }

    // Set Pixel format (assuming "Mono8" always available)
    if (!setPixelFormat("RGB8"))
        setPixelFormat("Mono8");

    // Get maximum format
    // SensorWidth, SensorHeight: Actual Size of the sensor in pixels
    int64_t maxWidth = Pylon::CIntegerParameter(nodemap, "Width").GetMax(); // after binning
    int64_t maxHeight = Pylon::CIntegerParameter(nodemap, "Height").GetMax();
    formats.push_back(new CamFmt(int(maxWidth), int(maxHeight), "FullSensor"));

    // Set Format (extract it via Pixelformat from pylon, only 1 format is valid)
    this->setFormat(0);

    CamFmt *f=formats[0];
    int cvfmt = CV_8UC3;
    if (f->bytesPerPixel==1) cvfmt=CV_8U;

    //now lets create buffers
    for (int i=0; i<m_numBuf; i++){
        m_imgProcessors.append(new ImgProc(f->width,f->height,cvfmt,"Basler_buf"+QString::number(s_instanceCount*1000+i)));
    }


    // Set exposure to auto
    // ..

    // Set gain to auto
    // ..

    return true;
}

bool Basler::activate_internal()
{
    if (m_status == CLOSED){
        open_internal();
    }

    if (m_status == OPEN){
        qInfo() << "SLabSBasler::activate()";

        m_CInstantCamera->StartGrabbing();

        if (m_CInstantCamera->IsGrabbing()){
            setState(ACTIVE);

            qInfo() << "    ...done!";
            return true;
        }
    }
    qInfo() << "    Error: Camera not ACTIVE!";
    return false;
}

bool Basler::isReady()
{
    return (m_status == ACTIVE);
}

bool Basler::deactivate_internal()
{
    if (m_status == ACTIVE){
        m_CInstantCamera->StopGrabbing();
        setState(OPEN);
        return true;
    }
    return false;
}

bool Basler::close_internal()
{
    if (m_status == ACTIVE) {
        deactivate_internal();
    }

    if (m_status == OPEN) {
        qInfo() << "SLabSBasler::close() on " << readableName;

        m_CInstantCamera->Close();

        //m_status = OPEN;
        setState(CLOSED);

        return true;

    } else {
        return false;
    }
}

void Basler::AllocateBuffer(size_t bufferSize, void **pCreatedBuffer, intptr_t &bufferContext)
{
    //this should point to our current memory,
    //since we call "capture" or request the image filling after
    //we update m_index before, this should work as expected
    *pCreatedBuffer = m_imgProcessors[m_index]->data;
    m_bufferContext = ++bufferContext;


    //maybe the memory is not in a continous chung, anyway
    //some simple checks
    cv::Mat *mat=m_imgProcessors[m_index];
    size_t sizeInBytes = mat->step[0] * mat->rows;
    assert(bufferSize<=sizeInBytes);


}

void Basler::FreeBuffer(void *pCreatedBuffer, intptr_t bufferContext)
{
    //the buffer is just returned, our imgProc is still there
}

void Basler::DestroyBufferFactory()
{
    //nothing to do here for us, the buffers belong already to us
}


bool Basler::capture_internal()
{
    assert(isReady());

    //m_CInstantCamera->ExecuteSoftwareTrigger();??????????
    m_index++;

    // only trigger -> start slot with remainig function?

    // Grab
    m_CInstantCamera->StartGrabbing(1);

    // This smart pointer will receive the grab result data.
    Pylon::CGrabResultPtr ptrGrabResult;
    if (!m_CInstantCamera->RetrieveResult(5000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException))
        qInfo() << "Error: Not able to retrieve Result.";
    //idling here

    if (!ptrGrabResult.IsValid()) {
        return false;
    }

    // Check size of result
    if (ptrGrabResult->GetHeight() != imgCV->rows ||
            ptrGrabResult->GetWidth() != imgCV->cols)
        qInfo() << "Error: Wrong image size.";

    // Convert image
    //the result should be already in our ImgProc's buffer, so nothing to do here
    /*
    Pylon::CImageFormatConverter formatConverter;
    if (imgCV->type() == CV_8UC3)
        formatConverter.OutputPixelFormat = Pylon::PixelType_RGB8packed;
    else
        formatConverter.OutputPixelFormat = Pylon::PixelType_Mono8;

    Pylon::CPylonImage pylonImage;
    formatConverter.Convert(imgCV->data, ptrGrabResult->GetBufferSize(), ptrGrabResult);
    */

    // Signal new Image
    emit newImage();

    return false;
}

QList<SCam *> Basler::loadCams()
{
    qInfo() << "SLabSBasler::loadCams()";

    QList<SCam*> cameraList;

    // Before using any pylon methods, the pylon runtime must be initialized.
    Pylon::PylonInitialize();

    // Get the transport layer factory.
    Pylon::CTlFactory& tlFactory = Pylon::CTlFactory::GetInstance();

    // Get all attached devices
    Pylon::DeviceInfoList_t devices;
    tlFactory.EnumerateDevices(devices);

    // Create all Pylon Devices.
    for (auto &d : devices){
        cameraList.push_back(new Basler( d ));
    }
    qInfo() << " ...  " << cameraList.size() << " Cams loaded.";

    // Return camera list
    return cameraList;
}

bool Basler::getDeviceTemperature(double &temp, std::string &tempState)
{
    try
    {
        // Get node list
        GenApi::INodeMap& nodemap = m_CInstantCamera->GetNodeMap();
        tempState = Pylon::CEnumParameter(nodemap, "TemperatureState").GetValue().c_str();
        temp = Pylon::CFloatParameter(nodemap, "DeviceTemperature").GetValue();
        return true;
    }
    catch (const Pylon::GenericException &e)
    {
        // Error handling.
//        qInfo() << "An exception occurred (Basler::getDeviceTemperature).\n " << e.GetDescription() << endl;
        return false;
    }
}

bool Basler::setPixelFormat(Pylon::String_t pixelFormat)
{
    try
    {
        // Get node list
        GenApi::INodeMap& nodemap = m_CInstantCamera->GetNodeMap();

        // Access the PixelFormat
        Pylon::CEnumParameter pF = Pylon::CEnumParameter(nodemap, "PixelFormat");

        // Set the pixel format to RGB if available.
        if (pF.CanSetValue(pixelFormat))
        {
            pF.SetValue(pixelFormat);
        }  else {
            qInfo() << "Unable to set pixel format to" << pixelFormat << "for" << readableName;

            // Get posible pixel formats
            Pylon::StringList_t settableValues;
            pF.GetSettableValues(settableValues);
            qInfo() << "  Available pixel formats:";
            for (auto &sv : settableValues)
                qInfo() << "    " << sv;
        }
        qInfo() << "PixelFormat set to: " << pF.GetValue();
    }
    catch (const Pylon::GenericException &e)
    {
        // Error handling.
        qInfo() << "An exception occurred (Basler::setPixelFormat()).\n " << e.GetDescription() << endl;
        return false;
    }
    return true;
}

bool Basler::setBrightness(double b)
{
    return false;
}

bool Basler::setContrast(double c)
{

    return false;
}

void Basler::setFormat(int formatIndex)
{
    assert(formatIndex<formats.size());

    CamFmt *f=Q_NULLPTR;

    // Get node list
    GenApi::INodeMap& nodemap = m_CInstantCamera->GetNodeMap();

    // Access the PixelFormat (assuming to be always available)
    Pylon::CEnumParameter pF = Pylon::CEnumParameter(nodemap, "PixelFormat");

    // Set up image storage
    if (pF.GetValue() == "RGB8") {
        f = new CamFmt(f->width, f->height, pF.GetValue().c_str(), 3);
    } else if (pF.GetValue() == "Mono8") {
        f = new CamFmt(f->width, f->height, pF.GetValue().c_str(), 1);
    } else {
        qInfo() << "Error: Unknown OpenCV - Type for PixelFormat (Basler::setFormat())";
    }

    assert(f!=Q_NULLPTR);

    qInfo() << "Format set to" << f->name << f->width << f->height;
    //store it
    formats[formatIndex]=f;
}
