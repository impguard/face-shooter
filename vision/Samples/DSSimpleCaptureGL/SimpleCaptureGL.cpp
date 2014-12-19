#include "DSAPI.h"
#include "DSAPIUtil.h"

#include "DSSampleCommon/Common.h"
#include <cassert>
#include <cctype>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <sstream>
#include <cstdlib>

#include "opencv2\core\core.hpp"
#include "opencv2\highgui\highgui.hpp"
#include "opencv2\opencv.hpp"

using namespace std;
// currently has a memory leak - never releases the storage or the cascade

CvHaarClassifierCascade *cascade;
CvMemStorage *storage;

void detectFaces(IplImage *newframe);

int key;
int iImg;
int N_image = 20;

//using namespace cv;

DSAPI * g_dsapi;
DSThird * g_third;
DSHardware * g_hardware;
bool g_paused, g_showOverlay = true, g_showImages = true, g_stopped = true;
GlutWindow g_depthWindow, g_leftWindow, g_rightWindow, g_thirdWindow; // Windows where we will display depth, left, right, third images
Timer g_timer, g_thirdTimer;
int g_lastThirdFrame;
float g_exposure, g_gain;

uint8_t g_zImageRGB[640 * 480 * 3];

void DrawGLImage(GlutWindow & window, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels);
double count = 0;

void postData(int x, int y, int z){
	stringstream ss;
	ss << "start /min C:\\python27\\python post.py " << x << " " << y << " " << z;
	system(ss.str().c_str());
}

void detectFaces(IplImage *newframe, uint16_t *Zimg, IplImage *zImgPic, double *trans) {
	CvSeq *faces = cvHaarDetectObjects(newframe, cascade, storage,
		1.15, 2,
		0, //CV_HAAR_DO_CANNY_PRUNING 
		cvSize(50, 50));

	if (faces->total > 0){
		int maxSiz = 0;
		int maxInd = 0;
		for (int i = 0; i < (faces ? faces->total : 0); i++)
		{
			CvRect *r = (CvRect *)cvGetSeqElem(faces, i);
			if (r->height * r->width > maxSiz){
				maxSiz = r->height * r->width;
				maxInd = i;
			}
		}
		CvRect *r = (CvRect *)cvGetSeqElem(faces, maxInd);
		cvRectangle(newframe,
			cvPoint(r->x, r->y),
			cvPoint(r->x + r->width, r->y + r->height),
			CV_RGB(0, 255, 0), 2, 8, 0);
			

		// Want to extract the mean distance of non-zero elements within some window around center
		int elemCount = 0;
		double vCount = 0;
		double widthPxZ = ((r->x + (r->width / 2)-trans[0]) * 628.0) / 640;
		double heighPxZ = ((r->y + (r->height / 2)) * 468.0) / 480;
		int window = 10;
		for (int i = widthPxZ - window; i < widthPxZ + window; i++){
			if ((i >= 0) && (i < 628)){
				for (int j = heighPxZ - window; j < heighPxZ + window; j++){
					//cout << "loop\n";
					if ((j >= 0) && (j < 468)){
						if (Zimg[j * 628 + i] != 0){
							vCount += Zimg[j * 628 + i];
							elemCount++;
						}
					}
				}
			}
		}
		cvRectangle(zImgPic,
			cvPoint(widthPxZ-10, heighPxZ-10),
			cvPoint(widthPxZ+10, heighPxZ+10),
			CV_RGB(0, 255, 0), 2, 8, 0);
		if (elemCount > ((window * window * 4) * 0.7)){
			postData(widthPxZ, heighPxZ, (int) vCount/elemCount);
		}
		else {
			postData(widthPxZ, heighPxZ, 0);
		}
		
	}
	else {
		postData(-1, -1, -1);
	}
}

void OnIdle()
{
    if (g_stopped)
        return;

    if (!g_paused)
    {
        DS_CHECK_ERRORS(g_dsapi->grab());
    }

    // Display Z image, if it is enabled
    if (g_dsapi->isZEnabled())
    {
        const uint8_t nearColor[] = {255, 0, 0}, farColor[] = {20, 40, 255};
        if (g_showImages) ConvertDepthToRGBUsingHistogram(g_dsapi->getZImage(), g_dsapi->zWidth(), g_dsapi->zHeight(), nearColor, farColor, g_zImageRGB);
        DrawGLImage(g_depthWindow, g_dsapi->zWidth(), g_dsapi->zHeight(), GL_RGB, GL_UNSIGNED_BYTE, g_zImageRGB);
    }

    // Display left/right images, if they are enabled
    if (g_dsapi->isLeftEnabled() || g_dsapi->isRightEnabled())
    {
        assert(g_dsapi->getLRPixelFormat() == DS_LUMINANCE8);
		if (g_dsapi->isLeftEnabled()){
			DrawGLImage(g_leftWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, g_showImages ? g_dsapi->getLImage() : nullptr);
			//saveImage((uint8_t *)g_dsapi->getLImage(), g_dsapi->lrWidth(), g_dsapi->lrHeight());

		}
		if (g_dsapi->isRightEnabled()){
			DrawGLImage(g_rightWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, g_showImages ? g_dsapi->getRImage() : nullptr);
			//saveImage((uint8_t *)g_dsapi->getRImage(), g_dsapi->lrWidth(), g_dsapi->lrHeight());

		}
    }

    // Display third image, if it is enabled
    if (g_third && g_third->isThirdEnabled() && g_third->getThirdFrameNumber() != g_lastThirdFrame)
    {
        assert(g_third->getThirdPixelFormat() == DS_RGB8);
        DrawGLImage(g_thirdWindow, g_third->thirdWidth(), g_third->thirdHeight(), GL_RGB, GL_UNSIGNED_BYTE, g_showImages ? g_third->getThirdImage() : nullptr);
        g_lastThirdFrame = g_third->getThirdFrameNumber();
		double translation[3];
		bool x = g_third->getCalibExtrinsicsZToRectThird(translation);

		g_dsapi->takeSnapshot();
		//saveImage((uint8_t *)g_third->getThirdImage(), g_third->thirdWidth(), g_third->thirdHeight());
        // load the image
		char filenameThird[100];
		char filenameZ[100];
		sprintf(filenameThird, "recording/Third/%06d.png", g_lastThirdFrame);
		sprintf(filenameZ, "recording/Z/%06d.png", g_lastThirdFrame);

		IplImage* inImage = cvLoadImage(filenameThird, 1);
		IplImage *zImage = cvLoadImage(filenameZ, 1);
		// do the face detection -- should rename to something else
		detectFaces(inImage, g_dsapi->getZImage(), zImage, translation);
		//cvShowImage("Face Detection", inImage);
		cvShowImage("Face Detection", zImage);
		cvShowImage("Face Detection2", inImage);


		cvReleaseImage(&inImage);
		// wait awhile 
		//cvWaitKey(0);
		remove(filenameThird);
		remove(filenameZ);

		g_thirdTimer.OnFrame();
    }

    g_timer.OnFrame();
}

void PrintControls()
{
    std::cout << "\nProgram controls:" << std::endl;
    std::cout << "  (space) Pause/unpause" << std::endl;
    std::cout << "  (d) Toggle display of overlay" << std::endl;
    std::cout << "  (i) Toggle display of images" << std::endl;
    std::cout << "  (s) Take snapshot" << std::endl;
    std::cout << "  (e/E) Modify exposure" << std::endl;
    std::cout << "  (g/G) Modify gain" << std::endl;
    std::cout << "  (l) Toggle emitter" << std::endl;
    std::cout << "  (q) Quit application" << std::endl;
}

void OnKeyboard(unsigned char key, int x, int y)
{
    switch (std::tolower(key))
    {
    case ' ':
        g_paused = !g_paused;
        break;
    case 'd':
        g_showOverlay = !g_showOverlay;
        break;
    case 'i':
        g_showImages = !g_showImages;
        break;
    case 's':
        g_dsapi->takeSnapshot();
        break;
    case 'e':
        g_exposure = std::min(std::max(g_exposure + (key == 'E' ? 0.1f : -0.1f), 0.0f), 16.3f);
        std::cout << "Setting exposure to " << std::fixed << std::setprecision(1) << g_exposure << std::endl;
        DS_CHECK_ERRORS(g_hardware->setImagerExposure(g_exposure, DS_BOTH_IMAGERS));
        break;
    case 'g':
        g_gain = std::min(std::max(g_gain + (key == 'G' ? 0.1f : -0.1f), 1.0f), 64.0f);
        std::cout << "Setting gain to " << std::fixed << std::setprecision(1) << g_gain << std::endl;
        DS_CHECK_ERRORS(g_hardware->setImagerGain(g_gain, DS_BOTH_IMAGERS));
        break;
    case 'l':
        if (auto emitter = g_dsapi->accessEmitter())
        {
            DS_CHECK_ERRORS(emitter->enableEmitter(!emitter->isEmitterEnabled()));
        }
        break;
    case 'q':
        glutLeaveMainLoop();
        break;
    case 'r':
        if (g_dsapi->startCapture())
            g_stopped = false;
        break;
    case 't':
        if (g_dsapi->stopCapture())
            g_stopped = true;
        break;
    }
}

int main(int argc, char * argv[])
{
	// load the classifier
	char *CascadeFile = "haarcascade_frontalface_default.xml";
	cascade = (CvHaarClassifierCascade*)cvLoad(CascadeFile);
	// create a buffer
	storage = cvCreateMemStorage(0);
	if (!cascade || !storage) {
		printf("Initialization failed\n");
		return 0;
	}

    g_dsapi = DSCreate(DS_DS4_PLATFORM);
    g_third = g_dsapi->accessThird();
    g_hardware = g_dsapi->accessHardware();

    DS_CHECK_ERRORS(g_dsapi->probeConfiguration());

    std::cout << "Firmware version: " << g_dsapi->getFirmwareVersionString() << std::endl;
    std::cout << "Software version: " << g_dsapi->getSoftwareVersionString() << std::endl;
    uint32_t serialNo;
    DS_CHECK_ERRORS(g_dsapi->getCameraSerialNumber(serialNo));
    std::cout << "Camera serial no: " << serialNo << std::endl;

    // Check calibration data
    if (!g_dsapi->isCalibrationValid())
    {
        std::cout << "Calibration data on camera is invalid" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Configure core Z-from-stereo capabilities
    DS_CHECK_ERRORS(g_dsapi->enableZ(true));
    DS_CHECK_ERRORS(g_dsapi->enableLeft(false));
    DS_CHECK_ERRORS(g_dsapi->enableRight(false));
    DS_CHECK_ERRORS(g_dsapi->setLRZResolutionMode(true, 628, 468, 60, DS_LUMINANCE8)); // Valid resolutions: 628x468, 480x360
    g_dsapi->enableLRCrop(true);

    // Configure third camera
    if (g_third)
    {
        DS_CHECK_ERRORS(g_third->enableThird(true));
        DS_CHECK_ERRORS(g_third->setThirdResolutionMode(true, 640, 480, 30, DS_RGB8)); // Valid resolutions: 1920x1080, 640x480
    }

    // Change exposure and gain values
    g_exposure = 16.3f;
    g_gain = 2.0f;
    DS_CHECK_ERRORS(g_hardware->setImagerExposure(g_exposure, DS_BOTH_IMAGERS));
    DS_CHECK_ERRORS(g_hardware->setImagerGain(g_gain, DS_BOTH_IMAGERS));

    //float exposure, gain;
    //DS_CHECK_ERRORS(g_hardware->getImagerExposure(exposure, DS_BOTH_IMAGERS));
    //DS_CHECK_ERRORS(g_hardware->getImagerGain(gain, DS_BOTH_IMAGERS));
    //std::cout << "(Before startCapture()) Initial exposure: " << exposure << ", initial gain: " << gain << std::endl;

    // Begin capturing images
    DS_CHECK_ERRORS(g_dsapi->startCapture());
    g_stopped = false;

    //DS_CHECK_ERRORS(g_hardware->getImagerExposure(exposure, DS_BOTH_IMAGERS));
    //DS_CHECK_ERRORS(g_hardware->getImagerGain(gain, DS_BOTH_IMAGERS));
    //std::cout << "(After startCapture()) Initial exposure: " << exposure << ", initial gain: " << gain << std::endl;

    // Open some GLUT windows to display the incoming images
    glutInit(&argc, argv);
    glutIdleFunc(OnIdle);

    if (g_dsapi->isZEnabled())
    {
        std::ostringstream title;
        title << "Z: " << g_dsapi->zWidth() << "x" << g_dsapi->zHeight() << "@" << g_dsapi->getLRZFramerate() << " FPS";
        g_depthWindow.Open(title.str(), 300 * g_dsapi->zWidth() / g_dsapi->zHeight(), 300, 60, 60, OnKeyboard);
    }

    if (g_dsapi->isLeftEnabled())
    {
        std::ostringstream title;
        title << "Left (" << DSPixelFormatString(g_dsapi->getLRPixelFormat()) << "): " << g_dsapi->lrWidth() << "x" << g_dsapi->lrHeight() << "@" << g_dsapi->getLRZFramerate() << " FPS";
        g_leftWindow.Open(title.str(), 300 * g_dsapi->lrWidth() / g_dsapi->lrHeight(), 300, 60, 420, OnKeyboard);
    }

    if (g_dsapi->isRightEnabled())
    {
        std::ostringstream title;
        title << "Right (" << DSPixelFormatString(g_dsapi->getLRPixelFormat()) << "): " << g_dsapi->lrWidth() << "x" << g_dsapi->lrHeight() << "@" << g_dsapi->getLRZFramerate() << " FPS";
        g_rightWindow.Open(title.str(), 300 * g_dsapi->lrWidth() / g_dsapi->lrHeight(), 300, 520, 420, OnKeyboard);
    }

    if (g_third->isThirdEnabled())
    {
        std::ostringstream title;
        title << "Third (" << DSPixelFormatString(g_third->getThirdPixelFormat()) << "): " << g_third->thirdWidth() << "x" << g_third->thirdHeight() << "@" << g_third->getThirdFramerate() << " FPS";
        g_thirdWindow.Open(title.str(), 300 * g_third->thirdWidth() / g_third->thirdHeight(), 300, 520, 60, OnKeyboard);
    }

    // Turn control over to GLUT
    PrintControls();
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutMainLoop();

    std::cout << "Destroying DSAPI instance." << std::endl;
    DSDestroy(g_dsapi);
    return 0;
}

static void OnDisplay() {}

int CreateGLWindow(std::string name, int width, int height, int startX, int startY)
{
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(startX, startY);
    auto windowNum = glutCreateWindow(name.c_str());

    glutDisplayFunc(OnDisplay);
    glutKeyboardFunc(OnKeyboard);
    glutIdleFunc(OnIdle);
    return windowNum;
}

void DrawGLImage(GlutWindow & window, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels)
{
    window.ClearScreen(0.1f, 0.1f, 0.15f);

    if (g_showImages)
    {
        window.DrawImage(width, height, format, type, pixels, 1);
    }

    if (g_showOverlay)
    {
        DrawString(10, 10, "FPS: %0.1f", (&window == &g_thirdWindow ? g_thirdTimer : g_timer).GetFramesPerSecond());
        DrawString(10, 28, "Frame #: %d", &window == &g_thirdWindow ? g_third->getThirdFrameNumber() : g_dsapi->getFrameNumber());
        DrawString(10, 46, "Frame time: %s", GetHumanTime(&window == &g_thirdWindow ? g_third->getThirdFrameTime() : g_dsapi->getFrameTime()).c_str());
    }

    glutSwapBuffers();
}
