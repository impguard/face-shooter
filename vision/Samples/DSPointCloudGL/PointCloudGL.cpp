#include "DSAPI.h"
#include "DSAPIUtil.h"

#include "DSSampleCommon/Common.h"
#include <cctype>
#include <algorithm>

DSAPI * g_dsapi;
DSThird * g_third;
int g_colorMode, g_geoMode;
bool g_paused, g_showDisplay = true;
float g_pitch, g_yaw;
Timer g_timer;

GLuint g_texture;
uint8_t g_zImageRGB[640 * 480 * 3];
void OnIdle()
{
    if (!g_paused)
    {
        DS_CHECK_ERRORS(g_dsapi->grab());
    }

    if (!g_texture)
    {
        glGenTextures(1, &g_texture);
        glBindTexture(GL_TEXTURE_2D, g_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }

    if (g_colorMode == 0)
    {
        const uint8_t nearColor[] = {255, 0, 0}, farColor[] = {20, 40, 255};
        ConvertDepthToRGBUsingHistogram(g_dsapi->getZImage(), g_dsapi->zWidth(), g_dsapi->zHeight(), nearColor, farColor, g_zImageRGB);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_dsapi->zWidth(), g_dsapi->zHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, g_zImageRGB);
    }
    else if (g_colorMode == 1)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_third->thirdWidth(), g_third->thirdHeight(), 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, g_third->getThirdImage());
    }
    else if (g_colorMode == 2)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_dsapi->lrWidth(), g_dsapi->lrHeight(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, g_dsapi->getLImage());
    }

    g_timer.OnFrame();
    glutPostRedisplay();
}

void PrintControls()
{
    std::cout << "\nProgram controls:" << std::endl;
    std::cout << "  (space) Pause/unpause" << std::endl;
    std::cout << "  (d) Toggle display of overlay" << std::endl;
    std::cout << "  (c) Switch point cloud coloring" << std::endl;
    std::cout << "  (g) Switch point cloud geometry" << std::endl;
    std::cout << "  (o) Reset point cloud orientation to straight-on" << std::endl;
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
        g_showDisplay = !g_showDisplay;
        break;
    case 'c':
        g_colorMode = (g_colorMode + 1) % 3;
        break;
    case 'g':
        g_geoMode = (g_geoMode + 1) % 3;
        break;
    case 'o':
        g_yaw = 0;
        g_pitch = 0;
        break;
    case 'q':
        glutLeaveMainLoop();
        break;
    }
}

int g_mouseX, g_mouseY;
void OnMouse(int button, int state, int x, int y)
{
    g_mouseX = x;
    g_mouseY = y;
}
void OnMotion(int x, int y)
{
    g_yaw += (g_mouseX - x) * 0.3f;
    g_pitch += (y - g_mouseY) * 0.3f;
    g_mouseX = x;
    g_mouseY = y;
}

template <class F>
void DrawPointCloud(const uint16_t * zImage, int width, int height, F emitVertex)
{
    switch (g_geoMode)
    {
    case 0:
        glBegin(GL_POINTS);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if (auto d = *zImage++)
                {
                    emitVertex(x, y, d);
                }
            }
        }
        glEnd();
        break;
    case 1:
        glBegin(GL_QUADS);
        for (int y = 1; y < height; ++y)
        {
            for (int x = 1; x < width; ++x)
            {
                auto d0 = zImage[(y - 1) * width + (x - 1)];
                auto d1 = zImage[(y - 1) * width + (x - 0)];
                auto d2 = zImage[(y - 0) * width + (x - 0)];
                auto d3 = zImage[(y - 0) * width + (x - 1)];
                auto minD = std::min(std::min(d0, d1), std::min(d2, d3));
                auto maxD = std::max(std::max(d0, d1), std::max(d2, d3));
                if (d0 && d1 && d2 && d3 && maxD - minD < 50)
                {
                    emitVertex(x - 1, y - 1, d0);
                    emitVertex(x - 0, y - 1, d1);
                    emitVertex(x - 0, y - 0, d2);
                    emitVertex(x - 1, y - 0, d3);
                }
            }
        }
        glEnd();
        break;
    case 2:
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex3f(-400, -300, +1000);
        glTexCoord2f(1, 0);
        glVertex3f(+400, -300, +1000);
        glTexCoord2f(1, 1);
        glVertex3f(+400, +300, +1000);
        glTexCoord2f(0, 1);
        glVertex3f(-400, +300, +1000);
        glEnd();
        break;
    }
}

void OnDisplay()
{
    if (!g_dsapi || g_dsapi->getFrameNumber() < 1) return;

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    // Set up the viewport
    int winWidth = glutGet(GLUT_WINDOW_WIDTH), winHeight = glutGet(GLUT_WINDOW_HEIGHT);
    glViewport(0, 0, winWidth, winHeight);
    glClearColor(0.1f, 0.1f, 0.15f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up matrices
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    gluPerspective(45, (double)winWidth / winHeight, 100, 4000);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    gluLookAt(0, 0, 0, 0, 0, 1, 0, -1, 0);

    // Orbit around a point 1m away from the camera
    glTranslatef(0, 0, 1000);
    glRotatef(g_pitch, 1, 0, 0);
    glRotatef(g_yaw, 0, 1, 0);
    glTranslatef(0, 0, -1000);

    // Render point cloud
    DSCalibIntrinsicsRectified zIntrin, thirdIntrin;
    double zToThirdTrans[3];
    DS_CHECK_ERRORS(g_dsapi->getCalibIntrinsicsZ(zIntrin));
    DS_CHECK_ERRORS(g_third->getCalibIntrinsicsRectThird(thirdIntrin));
    DS_CHECK_ERRORS(g_third->getCalibExtrinsicsZToRectThird(zToThirdTrans));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    switch (g_colorMode)
    {
    case 0:
    case 2:
        DrawPointCloud(g_dsapi->getZImage(), zIntrin.rw, zIntrin.rh, [&](int x, int y, uint16_t d)
                       {
            float zImage[] = { static_cast<float>(x), static_cast<float>(y), d }, zCamera[3];
            DSTransformFromZImageToZCamera(zIntrin, zImage, zCamera);
            glTexCoord2f((float)x / zIntrin.rw, (float)y / zIntrin.rh);
            glVertex3fv(zCamera);
        });
        break;
    case 1:
        DrawPointCloud(g_dsapi->getZImage(), zIntrin.rw, zIntrin.rh, [&](int x, int y, uint16_t d)
                       {
            float zImage[] = { static_cast<float>(x), static_cast<float>(y), d }, zCamera[3], thirdCamera[3], thirdImage[2];
            DSTransformFromZImageToZCamera(zIntrin, zImage, zCamera);
            DSTransformFromZCameraToRectThirdCamera(zToThirdTrans, zCamera, thirdCamera);
            DSTransformFromThirdCameraToRectThirdImage(thirdIntrin, thirdCamera, thirdImage);
            glTexCoord2f(thirdImage[0] / thirdIntrin.rw, thirdImage[1] / thirdIntrin.rh);
            glVertex3fv(zCamera);
        });
        break;
    }

    // Restore state
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();

    if (g_showDisplay)
    {
        static const char * colorModes[] = {"Depth histogram", "Rectified third image", "Left image"};
        static const char * geoModes[] = {"Points", "Mesh", "Image Only"};
        DrawString(10, 10, "FPS: %0.1f", g_timer.GetFramesPerSecond());
        DrawString(10, 28, "(C)olor mode: %s", colorModes[g_colorMode]);
        DrawString(10, 46, "(G)eometry mode: %s", geoModes[g_geoMode]);
    }

    glutSwapBuffers();
}

int main(int argc, char * argv[])
{
    g_dsapi = DSCreate(DS_DS4_PLATFORM);
    g_third = g_dsapi->accessThird();

    DS_CHECK_ERRORS(g_dsapi->probeConfiguration());

    std::cout << "Firmware version: " << g_dsapi->getFirmwareVersionString() << std::endl;
    std::cout << "Software version: " << g_dsapi->getSoftwareVersionString() << std::endl;

    // Check calibration data
    if (!g_dsapi->isCalibrationValid())
    {
        std::cout << "Calibration data on camera is invalid" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Configure core Z-from-stereo capabilities
    DS_CHECK_ERRORS(g_dsapi->enableZ(true));
    DS_CHECK_ERRORS(g_dsapi->enableLeft(true));
    DS_CHECK_ERRORS(g_dsapi->enableRight(false));
    DS_CHECK_ERRORS(g_dsapi->setLRZResolutionMode(true, 480, 360, 60, DS_LUMINANCE8));
    g_dsapi->enableLRCrop(true);

    // Configure third camera
    if (g_third)
    {
        DS_CHECK_ERRORS(g_third->enableThird(true));
        DS_CHECK_ERRORS(g_third->setThirdResolutionMode(true, 640, 480, 30, DS_BGRA8));
    }

    // Change exposure and gain values
    if (auto hardware = g_dsapi->accessHardware())
    {
        DS_CHECK_ERRORS(hardware->setImagerExposure(16.3f, DS_BOTH_IMAGERS));
        DS_CHECK_ERRORS(hardware->setImagerGain(2.0f, DS_BOTH_IMAGERS));
    }

    // Begin capturing images
    DS_CHECK_ERRORS(g_dsapi->startCapture());

    // Open a GLUT window to display point cloud
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(800, 600);
    glutCreateWindow("DS Point Cloud (OpenGL)");
    glutDisplayFunc(OnDisplay);
    glutKeyboardFunc(OnKeyboard);
    glutMotionFunc(OnMotion);
    glutMouseFunc(OnMouse);
    glutIdleFunc(OnIdle);

    // Turn control over to GLUT
    PrintControls();
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutMainLoop();

    std::cout << "Destroying DSAPI instance." << std::endl;
    DSDestroy(g_dsapi);
    return 0;
}