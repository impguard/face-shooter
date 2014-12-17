//
//    Rectification Sample
//
//  Starting from either of its native formats, the Third (rgb color image) can be
//  rectified and rotated to align with the stereo depth data.  This processing can be done
//  on the CPU, or, if only needed for display, can be done by the GPU by adding a small amount
//  of tesselation and adjusting the texture coordinates when rendering the video.
//
//  This can be useful for an AR application that renders objects on top of the video feed.
//  Straight lines in the real world will be straight lines when rendered (no lens distortion).
//  If depth information is used to create and/or place virtual objects, these can then now be
//  more easily lined up with the Third camera's point of view.
//

#include "DSAPI.h"
#include "DSAPIUtil.h"

#include "DSSampleCommon/Common.h"
#include <cassert>
#include <cctype>
#include <functional>

DSAPI * g_dsapi;
DSThird * g_third;
bool g_convertToBGRA = false;    // whether to convert native yuy2 to RGB (false) or BGRA (true);
bool g_rectificationOn = false;  // whether rectification is on or not
bool g_rectificationGPU = false; // will use opengl/gpu to draw rectified image if true, else uses software rectification
bool g_paused, g_showDisplay = true;
int g_thirdWindow;
Timer g_timer;

__declspec(align(16)) uint8_t g_thirdImage[1920 * 1080 * 4];
__declspec(align(16)) uint8_t g_thirdImageRect[1920 * 1080 * 4]; // extra buffer if we do software rectification in this sample
uint32_t g_softwareRectificationTable[1920 * 1080];              // the mapping of pixels from rectified image space to 3rd camera unrectified image

int CreateGLWindow(const char * name, int width, int height, int startX, int startY);
void DrawGLTexture(int windowNum, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels, std::function<void(float, float)> texcoord);

DSCalibIntrinsicsNonRectified g_intrinsicsNonRectThird;
DSCalibIntrinsicsRectified g_intrinsicsRectThird;
double g_extrinsicsRectThirdToNonRectThird[9];

void OnIdle()
{
    if (!g_paused)
    {
        DS_CHECK_ERRORS(g_dsapi->grab());
    }

    ((g_convertToBGRA) ? DSConvertYUY2ToBGRA8 : DSConvertYUY2ToRGB8)((const uint8_t *)g_third->getThirdImage(), g_third->thirdWidth(), g_third->thirdHeight(), g_thirdImage);
    GLenum format = (g_convertToBGRA) ? GL_BGRA_EXT : GL_RGB;

    if (!g_rectificationOn)
    {
        DrawGLTexture(g_thirdWindow, g_third->thirdWidth(), g_third->thirdHeight(), format, GL_UNSIGNED_BYTE, g_thirdImage, glTexCoord2f);
    }
    else
    {
        if (g_rectificationGPU)
        {
            DrawGLTexture(g_thirdWindow, g_third->thirdWidth(), g_third->thirdHeight(), format, GL_UNSIGNED_BYTE, g_thirdImage, [&](float u, float v) // generate texcoords using rectification parameters
                          {
                const float uv[2] = {u * g_intrinsicsRectThird.rw, v * g_intrinsicsRectThird.rh};  // put incoming uv from 0,0 to 1,1 into image range  u * 1920.0f, v * 1080.0f
                float t[2];  // will be coordinates in image space between 0,0 to raw image width,height
                DSTransformFromRectThirdImageToNonRectThirdImage(g_intrinsicsRectThird, g_extrinsicsRectThirdToNonRectThird, g_intrinsicsNonRectThird, uv, t);
                glTexCoord2f(t[0] / g_intrinsicsNonRectThird.w, t[1] / g_intrinsicsNonRectThird.h);    //  put result into 0,0 to 1,1 range  (tu / 1920.0f, tv / 1080.0f)
            });
        }
        else
        {
            static bool tableCreated = false;
            if (!tableCreated)
            {
                tableCreated = true;
                DSRectificationTable(g_intrinsicsNonRectThird, g_extrinsicsRectThirdToNonRectThird, g_intrinsicsRectThird, g_softwareRectificationTable);
            }
            (g_convertToBGRA ? DSRectifyBGRA8ToBGRA8 : DSRectifyRGB8ToRGB8)(g_softwareRectificationTable, g_thirdImage, g_third->thirdWidth(), g_third->thirdWidth(), g_third->thirdHeight(), g_thirdImageRect);
            DrawGLTexture(g_thirdWindow, g_third->thirdWidth(), g_third->thirdHeight(), format, GL_UNSIGNED_BYTE, g_thirdImageRect, glTexCoord2f);
        }
    }
    // Calculate framerate
    g_timer.OnFrame();
}

void PrintControls()
{
    std::cout << "\nProgram controls:" << std::endl;
    std::cout << "  (space) Pause/unpause" << std::endl;
    std::cout << "  (d) Toggle display of overlay" << std::endl;
    std::cout << "  (b) Switch output format between RGB and BGRA textures" << std::endl;
    std::cout << "  (r) Toggle rectification" << std::endl;
    std::cout << "  (g) Toggle rectification method between CPU and GPU" << std::endl;
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
    case 'b':
        g_convertToBGRA = !g_convertToBGRA;
        std::cout << "Converting YUY2 format to " << (g_convertToBGRA ? "BGRA8" : "RGB8") << std::endl;
        break;
    case 'r':
        g_rectificationOn = !g_rectificationOn;
        std::cout << "Rectification " << (g_rectificationOn ? "ENABLED" : "DISABLED") << std::endl;
        break;
    case 'g':
        g_rectificationGPU = !g_rectificationGPU;
        std::cout << "Rectification method " << (g_rectificationGPU ? "GPU" : "CPU") << std::endl;
        break;
    case 'q':
        glutLeaveMainLoop();
        break;
    }
}

static void OnDisplay() {}

int CreateGLWindow(const char * name, int width, int height, int startX, int startY)
{
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(startX, startY);
    auto windowNum = glutCreateWindow(name);

    glutDisplayFunc(OnDisplay);
    glutKeyboardFunc(OnKeyboard);
    glutIdleFunc(OnIdle);
    return windowNum;
}

void DrawGLTexture(int windowNum, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels, std::function<void(float, float)> texcoord)
{
    glutSetWindow(windowNum);
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    const GLuint texName = 0; // hope 0 is ok
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texName);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // use nearest to see actual uninterpolated image pixels
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, type, pixels);

    int winWidth = glutGet(GLUT_WINDOW_WIDTH), winHeight = glutGet(GLUT_WINDOW_HEIGHT);
    glViewport(0, 0, winWidth, winHeight);
    glClearColor(0.1f, 0.1f, 0.15f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    // float xpad = 0, ypad = 0;
    float ypad = ((float)winHeight / winWidth / ((float)height / width) - 1) / 2.0f;
    float xpad = ((float)winWidth / winHeight / ((float)width / height) - 1) / 2.0f;
    if (ypad < 0) ypad = 0;
    if (xpad < 0) xpad = 0;
    glOrtho(0 - xpad, 1.0 + xpad, 0 - ypad, 1.0 + ypad, .1, 100); //  note: 0.5,0.5 (not 0,0) will be center of viewport
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glColor3f(1.0, 1.0, 0.5);
    glNormal3d(0, 0, 1);

    // render a tesselated quad to allow for low frequency image warping
    const int tesselation = 16;
    float du = 1.0f / (float)tesselation;
    float dv = 1.0f / (float)tesselation;
    for (int i = 0; i < tesselation; i++)
    {
        for (int j = 0; j < tesselation; j++)
        {
            float u = i * du, v = j * dv;
            texcoord(u, 1.0f - v);
            glVertex3f(u, v, -1.0f);
            texcoord(u + du, 1.0f - v);
            glVertex3f(u + du, v, -1.0f);
            texcoord(u + du, 1.0f - (v + dv));
            glVertex3f(u + du, v + dv, -1.0f);
            texcoord(u, 1.0f - (v + dv));
            glVertex3f(u, v + dv, -1.0f);
        }
    }

    glEnd();

    glMatrixMode(GL_PROJECTION); // restore previous matrix stack states
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();

    if (g_showDisplay)
    {
        DrawString(10, 10, "FPS: %0.1f", g_timer.GetFramesPerSecond());
        DrawString(10, 28, "Rectification: %s ('r' enable,'g' method)", g_rectificationOn ? (g_rectificationGPU ? "GPU" : "CPU") : "OFF");
        DrawString(10, 46, "Tex Format: %s ('b' toggles)", g_convertToBGRA ? "BGRA" : "RGB ");
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

    DS_CHECK_ERRORS(g_third->enableThird(true));

    // Switch to a rectified mode, and retrieve calibration intrinsics + extrinsics
    DS_CHECK_ERRORS(g_third->setThirdResolutionMode(true, 1920, 1080, 30, DS_RGB8));
    DS_CHECK_ERRORS(g_third->getCalibIntrinsicsRectThird(g_intrinsicsRectThird));
    DS_CHECK_ERRORS(g_third->getCalibExtrinsicsRectThirdToNonRectThird(g_extrinsicsRectThirdToNonRectThird));

    // Switch to a non-rectified mode, and begin capture
    DS_CHECK_ERRORS(g_third->setThirdResolutionMode(false, 1920, 1080, 30, DS_NATIVE_YUY2)); // w,h 1920,1080 or 640, 480
    DS_CHECK_ERRORS(g_third->getCalibIntrinsicsNonRectThird(g_intrinsicsNonRectThird));
    assert(g_third->getThirdPixelFormat() == DS_NATIVE_YUY2);
    DS_CHECK_ERRORS(g_dsapi->startCapture());

    // Open some GLUT windows to display the incoming images
    glutInit(&argc, argv);

    DSCalibIntrinsicsNonRectified intrinsicsNonRect;
    DS_CHECK_ERRORS(g_third->getCalibIntrinsicsNonRectThird(intrinsicsNonRect));
    std::cout << "\nThird non-rectified intrinsics:" << intrinsicsNonRect << std::endl;

    g_thirdWindow = CreateGLWindow("DS GPU Rectification (OpenGL)", 960, 540, 480, 270);

    // Turn control over to GLUT
    PrintControls();
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutMainLoop();

    std::cout << "Destroying DSAPI instance." << std::endl;
    DSDestroy(g_dsapi);
    return 0;
}
