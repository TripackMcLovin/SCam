# SCam
An easy generic image capture library for industrial and consumer cameras with an OpenCV and Qt compatible output format

Organized as a qmake project for QTCreator, intended to be used as a depending project.

Supported Cams and CamLibrary:

-V4L2 directly with linux ioctrl
-IDS via µeye library
-Basler via pylon library
-ffmpeg video files for simulation situations where no cam is at hand

dependencies:
qt (>=4.3)
µeye lib of IDS Imaging,
pylon (>=6.0) by Basler,
ffmpeg
opencv (>=4.2)
