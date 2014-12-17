#include "DSAPI.h"
#include "DSAPIUtil.h"
#include "DSSampleCommon/Common.h"
#include <cctype>
#include <algorithm>
#include <sstream>
#include <string>

DSAPI * g_dsapi;
DSFile * g_file;
DSThird * g_third;
DSHardware * g_hardware;
bool g_paused, g_showOverlay = true, g_showImages = true, g_stopped = true;
GlutWindow g_depthWindow, g_leftWindow, g_rightWindow, g_thirdWindow; // Windows where we will display depth, left, right, third images
Timer g_timer, g_thirdTimer;
bool g_useAutoExposure;
float g_exposure, g_gain;
int g_lastThirdFrame;

uint8_t g_zImageRGB[640 * 480 * 3];
uint8_t g_leftImage[640 * 480], g_rightImage[640 * 480];
__declspec(align(16)) uint8_t g_thirdImage[1920 * 1080 * 4];

void DrawGLImage(GlutWindow & window, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels, GLfloat multiplier);

void OnIdle()
{
    if (g_stopped)
        return;

    if (!g_paused)
    {
        DS_CHECK_ERRORS(g_dsapi->grab());
    }

    if (g_showImages)
    {
        // Display Z image, if it is enabled
        if (g_dsapi->isZEnabled())
        {
            const uint8_t nearColor[] = {255, 0, 0}, farColor[] = {20, 40, 255};
            ConvertDepthToRGBUsingHistogram(g_dsapi->getZImage(), g_dsapi->zWidth(), g_dsapi->zHeight(), nearColor, farColor, g_zImageRGB);
            DrawGLImage(g_depthWindow, g_dsapi->zWidth(), g_dsapi->zHeight(), GL_RGB, GL_UNSIGNED_BYTE, g_zImageRGB, 1);
        }

        // Display left/right images, if they are enabled
        if (g_dsapi->isLeftEnabled() || g_dsapi->isRightEnabled())
        {
            float multiplier = static_cast<float>(1 << (16 - g_dsapi->maxLRBits()));
            switch (g_dsapi->getLRPixelFormat())
            {
            case DS_LUMINANCE8:
                if (g_dsapi->isLeftEnabled()) DrawGLImage(g_leftWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, g_dsapi->getLImage(), 1);
                if (g_dsapi->isRightEnabled()) DrawGLImage(g_rightWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, g_dsapi->getRImage(), 1);
                break;
            case DS_LUMINANCE16:
                if (g_dsapi->isLeftEnabled()) DrawGLImage(g_leftWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_SHORT, g_dsapi->getLImage(), multiplier);
                if (g_dsapi->isRightEnabled()) DrawGLImage(g_rightWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_SHORT, g_dsapi->getRImage(), multiplier);
                break;
            case DS_NATIVE_L_LUMINANCE8:
                if (g_dsapi->isLeftEnabled()) DrawGLImage(g_leftWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, g_dsapi->getLImage(), 1);
                break;
            case DS_NATIVE_L_LUMINANCE16:
                if (g_dsapi->isLeftEnabled()) DrawGLImage(g_leftWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_SHORT, g_dsapi->getLImage(), multiplier);
                break;
            case DS_NATIVE_RL_LUMINANCE8:
                DSConvertRLLuminance8ToLuminance8(g_dsapi->getLImage(), g_dsapi->lrWidth(), g_dsapi->lrHeight(), g_leftImage, g_rightImage);
                if (g_dsapi->isLeftEnabled()) DrawGLImage(g_leftWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, g_leftImage, 1);
                if (g_dsapi->isRightEnabled()) DrawGLImage(g_rightWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, g_rightImage, 1);
                break;
            case DS_NATIVE_RL_LUMINANCE12:
                DSConvertRLLuminance12ToLuminance8(g_dsapi->getLImage(), g_dsapi->lrWidth(), g_dsapi->lrHeight(), g_dsapi->maxLRBits() - 8, g_leftImage, g_rightImage);
                if (g_dsapi->isLeftEnabled()) DrawGLImage(g_leftWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, g_leftImage, 1);
                if (g_dsapi->isRightEnabled()) DrawGLImage(g_rightWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, g_rightImage, 1);
                break;
            case DS_NATIVE_RL_LUMINANCE16:
                DSConvertRLLuminance16ToLuminance8(g_dsapi->getLImage(), g_dsapi->lrWidth(), g_dsapi->lrHeight(), g_dsapi->maxLRBits() - 8, g_leftImage, g_rightImage);
                if (g_dsapi->isLeftEnabled()) DrawGLImage(g_leftWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, g_leftImage, 1);
                if (g_dsapi->isRightEnabled()) DrawGLImage(g_rightWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, g_rightImage, 1);
                break;
            }
        }

        // Display third image, if it is enabled
        if (g_third && g_third->isThirdEnabled() && g_third->getThirdFrameNumber() != g_lastThirdFrame)
        {
            switch (g_third->getThirdPixelFormat())
            {
            case DS_RGB8:
                DrawGLImage(g_thirdWindow, g_third->thirdWidth(), g_third->thirdHeight(), GL_RGB, GL_UNSIGNED_BYTE, g_third->getThirdImage(), 1);
                break;
            case DS_BGRA8:
                DrawGLImage(g_thirdWindow, g_third->thirdWidth(), g_third->thirdHeight(), GL_BGRA_EXT, GL_UNSIGNED_BYTE, g_third->getThirdImage(), 1);
                break;
            case DS_NATIVE_YUY2:
                DSConvertYUY2ToBGRA8((const uint8_t *)g_third->getThirdImage(), g_third->thirdWidth(), g_third->thirdHeight(), g_thirdImage);
                DrawGLImage(g_thirdWindow, g_third->thirdWidth(), g_third->thirdHeight(), GL_BGRA_EXT, GL_UNSIGNED_BYTE, g_thirdImage, 1);
                break;
            case DS_NATIVE_RAW10:
                DSConvertRaw10ToBGRA8((uint8_t *)g_third->getThirdImage(), g_third->thirdWidth(), g_third->thirdHeight(), g_thirdImage);
                DrawGLImage(g_thirdWindow, g_third->thirdWidth(), g_third->thirdHeight(), GL_BGRA_EXT, GL_UNSIGNED_BYTE, g_thirdImage, 1);
                break;
            }

            g_lastThirdFrame = g_third->getThirdFrameNumber();
            g_thirdTimer.OnFrame();
        }
    }
    else
    {
        if (g_dsapi->isZEnabled()) DrawGLImage(g_depthWindow, 0, 0, 0, 0, nullptr, 1);
        if (g_dsapi->isLeftEnabled()) DrawGLImage(g_leftWindow, 0, 0, 0, 0, nullptr, 1);
        if (g_dsapi->isRightEnabled()) DrawGLImage(g_rightWindow, 0, 0, 0, 0, nullptr, 1);
        if (g_third && g_third->isThirdEnabled() && g_third->getThirdFrameNumber() != g_lastThirdFrame)
        {
            DrawGLImage(g_thirdWindow, 0, 0, 0, 0, nullptr, 1);
            g_lastThirdFrame = g_third->getThirdFrameNumber();
            g_thirdTimer.OnFrame();
        }
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
        if (!g_useAutoExposure)
        {
            g_exposure = std::min(std::max(g_exposure + (key == 'E' ? 0.1f : -0.1f), 0.0f), 16.3f);
            std::cout << "Setting exposure to " << std::fixed << std::setprecision(1) << g_exposure << std::endl;
            DS_CHECK_ERRORS(g_hardware->setImagerExposure(g_exposure, DS_BOTH_IMAGERS));
        }
        break;
    case 'g':
        if (!g_useAutoExposure)
        {
            g_gain = std::min(std::max(g_gain + (key == 'G' ? 0.1f : -0.1f), 1.0f), 64.0f);
            std::cout << "Setting gain to " << std::fixed << std::setprecision(1) << g_gain << std::endl;
            DS_CHECK_ERRORS(g_hardware->setImagerGain(g_gain, DS_BOTH_IMAGERS));
        }
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

bool PromptYesNo(const char * prompt)
{
    while (true)
    {
        std::cout << prompt << " (y/n): ";
        std::string token;
        std::cin >> token;
        if (token == "y" || token == "Y" || token == "yes") return true;
        if (token == "n" || token == "N" || token == "no") return false;
    }
}

int PromptChoice(const char * prompt, int min, int max)
{
    while (true)
    {
        std::cout << prompt << " (" << min << "-" << max << "): ";
        int choice;
        if (std::cin >> choice && choice >= min && choice <= max) return choice;
    }
}

int main(int argc, char * argv[])
{
    std::string directory;
    std::cout << "Enter directory to read from (or leave blank for live device): ";
    std::getline(std::cin, directory);

    if (directory.empty())
    {
        g_dsapi = DSCreate(DS_DS4_PLATFORM);
        DS_CHECK_ERRORS(g_dsapi->probeConfiguration());
    }
    else
    {
        g_dsapi = DSCreate(DS_FILE_PLATFORM);
        g_dsapi->accessFile()->setReadFileModeParams(directory.c_str());
        DS_CHECK_ERRORS(g_dsapi->probeConfiguration());
    }
    std::cout << "Firmware version: " << g_dsapi->getFirmwareVersionString() << std::endl;
    std::cout << "Software version: " << g_dsapi->getSoftwareVersionString() << std::endl;

    g_dsapi->setLoggingLevelAndFile(DS_LOG_TRACE, "DSInteractiveCaptureGL.log");
    g_dsapi->logCustomMessage(DS_LOG_INFO, "DSInteractiveCaptureGL started");

    g_file = g_dsapi->accessFile();
    g_third = g_dsapi->accessThird();
    g_hardware = g_dsapi->accessHardware();

    // Configure file mode
    if (g_file)
    {
        std::cout << "File has recorded images for:";
        if (g_file->isDepthRecorded()) std::cout << " Z";
        if (g_file->isLeftRecorded()) std::cout << " left";
        if (g_file->isRightRecorded()) std::cout << " right";
        if (g_file->isThirdRecorded()) std::cout << " third";
        std::cout << std::endl;

        g_file->enableReadFileModeLoop(PromptYesNo("Loop through file?"));
    }

    if (!g_file || !PromptYesNo("Use default configuration from file?"))
    {
        // Configure DSAPI interface
        if (g_dsapi)
        {
            DS_CHECK_ERRORS(g_dsapi->enableZ(false));
            const int nRectLRZ = g_dsapi->getLRZNumberOfResolutionModes(true), nNonRectLRZ = g_dsapi->getLRZNumberOfResolutionModes(false);
            if (nRectLRZ + nNonRectLRZ > 0)
            {
                std::cout << "Left/right/Z resolution modes: " << nRectLRZ << " rectified, " << nNonRectLRZ << " non-rectified available" << std::endl;
                bool enableLeft = PromptYesNo("Enable left images?");
                bool enableRight = PromptYesNo("Enable right images?");
                bool enableZ = 0, rectifyLRZ = 0, cropLR = 0;
                if (nRectLRZ > 0)
                {
                    enableZ = PromptYesNo("Enable Z images?");
                    if (enableZ)
                    {
                        std::cout << "Rectifying left/right images, because Z was requested." << std::endl;
                        rectifyLRZ = true;
                    }
                    else if (enableLeft || enableRight)
                    {
                        rectifyLRZ = PromptYesNo("Rectify left/right images?");
                    }
                }
                else if (enableLeft || enableRight)
                {
                    std::cout << "Not rectifying left/right images, since no rectified resolutions are available." << std::endl;
                }

                if (enableLeft || enableRight || enableZ)
                {
                    std::cout << "Available left/right/Z resolution modes:" << std::endl;
                    for (int i = 0; i < g_dsapi->getLRZNumberOfResolutionModes(rectifyLRZ); i++)
                    {
                        int zWidth, zHeight, lrzFps;
                        DSPixelFormat lrFormat;
                        DS_CHECK_ERRORS(g_dsapi->getLRZResolutionMode(rectifyLRZ, i, zWidth, zHeight, lrzFps, lrFormat));
                        std::cout << "  (" << i << ") " << zWidth << " x " << zHeight << " Z, " << DSPixelFormatString(lrFormat) << " left/right at " << lrzFps << " FPS" << std::endl;
                    }
                    int mode = PromptChoice("Choose resolution mode.", 0, g_dsapi->getLRZNumberOfResolutionModes(rectifyLRZ) - 1);

                    if ((enableLeft || enableRight) && enableZ)
                    {
                        cropLR = PromptYesNo("Crop left/right images to Z image?");
                    }

                    int zWidth, zHeight, lrzFps;
                    DSPixelFormat lrFormat;
                    DS_CHECK_ERRORS(g_dsapi->getLRZResolutionMode(rectifyLRZ, mode, zWidth, zHeight, lrzFps, lrFormat));
                    DS_CHECK_ERRORS(g_dsapi->setLRZResolutionMode(rectifyLRZ, zWidth, zHeight, lrzFps, lrFormat));
                    DS_CHECK_ERRORS(g_dsapi->enableLeft(enableLeft));
                    DS_CHECK_ERRORS(g_dsapi->enableRight(enableRight));
                    DS_CHECK_ERRORS(g_dsapi->enableZ(enableZ));
                    g_dsapi->enableLRCrop(cropLR);
                }
                std::cout << "Left/right/Z images fully configured.\n" << std::endl;
            }
            else
            {
                std::cout << "Left/right/Z images will be disabled, since no left/right/Z resolution modes are available!\n" << std::endl;
            }
        }

        // Configure DSThird interface
        if (g_third)
        {
            const int nRectThird = g_third->getThirdNumberOfResolutionModes(true), nNonRectThird = g_third->getThirdNumberOfResolutionModes(false);
            if (nRectThird + nNonRectThird > 0)
            {
                std::cout << "Third resolution modes: " << nRectThird << " rectified, " << nNonRectThird << " non-rectified available" << std::endl;
                bool enableThird = PromptYesNo("Enable third images?");
                if (enableThird)
                {
                    bool rectifyThird = 0;
                    if (nRectThird > 0)
                    {
                        rectifyThird = PromptYesNo("Rectify third images?");
                    }
                    else
                    {
                        std::cout << "Not third images, since no rectified resolutions are available." << std::endl;
                    }

                    std::cout << "Available third resolution modes:" << std::endl;
                    for (int i = 0; i < g_third->getThirdNumberOfResolutionModes(rectifyThird); i++)
                    {
                        int thirdWidth, thirdHeight, thirdFps;
                        DSPixelFormat thirdFormat;
                        DS_CHECK_ERRORS(g_third->getThirdResolutionMode(rectifyThird, i, thirdWidth, thirdHeight, thirdFps, thirdFormat));
                        std::cout << "  (" << i << ") " << thirdWidth << " x " << thirdHeight << " " << DSPixelFormatString(thirdFormat) << " at " << thirdFps << " FPS" << std::endl;
                    }
                    int mode = PromptChoice("Choose resolution mode.", 0, g_third->getThirdNumberOfResolutionModes(rectifyThird) - 1);

                    int thirdWidth, thirdHeight, thirdFps;
                    DSPixelFormat thirdFormat;
                    DS_CHECK_ERRORS(g_third->getThirdResolutionMode(rectifyThird, mode, thirdWidth, thirdHeight, thirdFps, thirdFormat));
                    DS_CHECK_ERRORS(g_third->setThirdResolutionMode(rectifyThird, thirdWidth, thirdHeight, thirdFps, thirdFormat));
                    DS_CHECK_ERRORS(g_third->enableThird(enableThird));
                }
                std::cout << "Third images fully configured.\n" << std::endl;
            }
            else
            {
                std::cout << "Third images will be disabled, since no third resolution modes are available!\n" << std::endl;
            }
        }
    }

    g_useAutoExposure = PromptYesNo("Turn on auto exposure?");

    std::getline(std::cin, directory); // Probably a newline on stdin from the last prompt. Kill it.
    std::cout << "\nEnter directory to record to (or leave blank for no recording): ";
    std::getline(std::cin, directory);
    if (!directory.empty())
    {
        int everyNthImage = PromptChoice("Record one out of every how many frames?", 1, 1000);
        g_dsapi->setRecordingFilePath(directory.c_str());
        g_dsapi->startRecordingToFile(true, everyNthImage);
    }

    // Change exposure and gain values
    if (g_hardware)
    {
        if (g_useAutoExposure)
        {
            g_hardware->setAutoExposure(DS_BOTH_IMAGERS, true);
        }
        else
        {
            g_exposure = 16.3f;
            g_gain = 2.0f;
            DS_CHECK_ERRORS(g_hardware->setImagerExposure(g_exposure, DS_BOTH_IMAGERS));
            DS_CHECK_ERRORS(g_hardware->setImagerGain(g_gain, DS_BOTH_IMAGERS));
        }
    }

    // Begin capturing images
    std::cout << "Starting capture." << std::endl;
    DS_CHECK_ERRORS(g_dsapi->startCapture());
    g_stopped = false;

    // Open some GLUT windows to display the incoming images
    glutInit(&argc, argv);
    glutIdleFunc(OnIdle);
    DSCalibIntrinsicsRectified intrinsicsRect;
    DSCalibIntrinsicsNonRectified intrinsicsNonRect;

    if (g_dsapi->isZEnabled())
    {
        if (g_dsapi->isCalibrationValid())
        {
            DS_CHECK_ERRORS(g_dsapi->getCalibIntrinsicsZ(intrinsicsRect));
            std::cout << "\nZ intrinsics:" << intrinsicsRect << std::endl;
        }

        std::ostringstream title;
        title << "Z: " << g_dsapi->zWidth() << "x" << g_dsapi->zHeight() << "@" << g_dsapi->getLRZFramerate() << " FPS";
        g_depthWindow.Open(title.str(), 300 * g_dsapi->zWidth() / g_dsapi->zHeight(), 300, 60, 60, OnKeyboard);
    }

    if (g_dsapi->isLeftEnabled())
    {
        if (g_dsapi->isCalibrationValid())
        {
            if (g_dsapi->isRectificationEnabled())
            {
                DS_CHECK_ERRORS(g_dsapi->getCalibIntrinsicsRectLeftRight(intrinsicsRect));
                std::cout << "\nLeft rectified intrinsics:" << intrinsicsRect << std::endl;
            }
            else
            {
                DS_CHECK_ERRORS(g_dsapi->getCalibIntrinsicsNonRectLeft(intrinsicsNonRect));
                std::cout << "\nLeft non-rectified intrinsics:" << intrinsicsNonRect << std::endl;
            }
        }

        std::ostringstream title;
        title << "Left (" << DSPixelFormatString(g_dsapi->getLRPixelFormat()) << "): " << g_dsapi->lrWidth() << "x" << g_dsapi->lrHeight() << "@" << g_dsapi->getLRZFramerate() << " FPS";
        g_leftWindow.Open(title.str(), 300 * g_dsapi->lrWidth() / g_dsapi->lrHeight(), 300, 60, 420, OnKeyboard);
    }

    if (g_dsapi->isRightEnabled())
    {
        if (g_dsapi->isCalibrationValid())
        {
            if (g_dsapi->isRectificationEnabled())
            {
                DS_CHECK_ERRORS(g_dsapi->getCalibIntrinsicsRectLeftRight(intrinsicsRect));
                std::cout << "\nRight rectified intrinsics:" << intrinsicsRect << std::endl;
            }
            else
            {
                DS_CHECK_ERRORS(g_dsapi->getCalibIntrinsicsNonRectRight(intrinsicsNonRect));
                std::cout << "\nRight non-rectified intrinsics:" << intrinsicsNonRect << std::endl;
            }
        }

        std::ostringstream title;
        title << "Right (" << DSPixelFormatString(g_dsapi->getLRPixelFormat()) << "): " << g_dsapi->lrWidth() << "x" << g_dsapi->lrHeight() << "@" << g_dsapi->getLRZFramerate() << " FPS";
        g_rightWindow.Open(title.str(), 300 * g_dsapi->lrWidth() / g_dsapi->lrHeight(), 300, 520, 420, OnKeyboard);
    }

    if (g_third->isThirdEnabled())
    {
        if (g_dsapi->isCalibrationValid())
        {
            if (g_third->isThirdRectificationEnabled())
            {
                DS_CHECK_ERRORS(g_third->getCalibIntrinsicsRectThird(intrinsicsRect));
                std::cout << "\nThird rectified intrinsics:" << intrinsicsRect << std::endl;
            }
            else
            {
                DS_CHECK_ERRORS(g_third->getCalibIntrinsicsNonRectThird(intrinsicsNonRect));
                std::cout << "\nThird non-rectified intrinsics:" << intrinsicsNonRect << std::endl;
            }
        }

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

void DrawGLImage(GlutWindow & window, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels, GLfloat multiplier)
{
    window.ClearScreen(0.1f, 0.1f, 0.15f);

    if (g_showImages)
    {
        window.DrawImage(width, height, format, type, pixels, multiplier);
    }

    if (g_showOverlay)
    {
        DrawString(10, 10, "FPS: %0.1f", (&window == &g_thirdWindow ? g_thirdTimer : g_timer).GetFramesPerSecond());
        DrawString(10, 28, "Frame #: %d", &window == &g_thirdWindow ? g_third->getThirdFrameNumber() : g_dsapi->getFrameNumber());
        DrawString(10, 46, "Frame time: %s", GetHumanTime(&window == &g_thirdWindow ? g_third->getThirdFrameTime() : g_dsapi->getFrameTime()).c_str());
    }

    glutSwapBuffers();
}
