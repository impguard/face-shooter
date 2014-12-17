#pragma once

#include <cstdint> // For standard sized integer types
#include <climits> // For int limits

#ifdef DS_EXPORTS
#define DS_DECL \
    __declspec(dllexport)
#else
#define DS_DECL \
    __declspec(dllimport)
#endif

/// @defgroup Global Types and Constants
/// Defines DSAPI global data types, typedefs, and constants.
/// @{

/// Platform
enum DSPlatform
{
    DS_DS4_PLATFORM = 0,                                       ///< Use DS 4 device over a USB connection
    DS_DS5_PLATFORM = 1,                                       ///< Use DS 5 device over a USB connection
    DS_FILE_PLATFORM = 0x10000,                                ///< Use data previously recorded to disk
    DS_DS4_FILE_PLATFORM = DS_DS4_PLATFORM | DS_FILE_PLATFORM, ///< Use data previously recorded to disk from a DS4 device
    DS_DS5_FILE_PLATFORM = DS_DS5_PLATFORM | DS_FILE_PLATFORM  ///< Use data previously recorded to disk from a DS5 device
};

/// Logging level in order of increasing detail of logging.
enum DSLoggingLevel
{
    DS_LOG_OFF = 0,
    DS_LOG_FATAL,
    DS_LOG_ERROR,
    DS_LOG_WARN,
    DS_LOG_INFO,
    DS_LOG_DEBUG,
    DS_LOG_TRACE
};

/// Z units
enum DSZUnits
{
    DS_MILLIMETERS = 1000,  ///< mm
    DS_CENTIMETERS = 10000, ///< cm
    DS_METERS = 1000000,    ///< m
    DS_INCHES = 25400,      ///< in
    DS_FEET = 304800        ///< ft
};

/// DSPixelFormat specifies one of the legal pixel formats.
enum DSPixelFormat
{
    DS_LUMINANCE8,            ///< Luminance 8 bits
    DS_LUMINANCE16,           ///< Luminance 16 bits
    DS_RGB8,                  ///< Color 8 bits color in order red, green, blue
    DS_BGRA8,                 ///< Color 8 bits color in order blue, green, red, alpha (Equivalent to D3DCOLOR, GL_BGRA)
    DS_NATIVE_L_LUMINANCE8,   ///< Native luminance format, equivalent to DS_LUMINANCE8, for left image only (right image must be disabled)
    DS_NATIVE_L_LUMINANCE16,  ///< Native luminance format, equivalent to DS_LUMINANCE16, for left image only (right image must be disabled)
    DS_NATIVE_RL_LUMINANCE8,  ///< Native pixel-interleaved stereo luminance format, 8 bit pixel from right image, then 8 bit pixel from left image (all 8 bits used)
    DS_NATIVE_RL_LUMINANCE12, ///< Native pixel-interleaved stereo luminance format, 12 bit pixel from right image, then 12 bit pixel from left image (call maxLRBits() to determine how many are used)
    DS_NATIVE_RL_LUMINANCE16, ///< Native pixel-interleaved stereo luminance format, 16 bit pixel from right image, then 16 bit pixel from left image (call maxLRBits() to determine how many are used)
    DS_NATIVE_YUY2,           ///< Native color format, 32 bits per 2x1 pixel patch, with separate 8-bit luminance values (Y), and shared 8-bit chrominance values (U,V) in Y0 U Y1 V order.
    DS_NATIVE_RAW10           ///< Native color format, 40 bits per 4x1 pixel patch, in an 8:8:8:8:2:2:2:2 pattern (10 bits per pixel, high bits first). Bayer patterned: Even rows represent R G R G, odd rows represent G B G B.
};
DS_DECL const char * DSPixelFormatString(DSPixelFormat pixelFormat);

/// Format for image files
enum DSImageFileFormat
{
    DS_PNG = 0, ///< png format
    DS_PNM,     ///< pgm, ppm and psm formats
};
DS_DECL const char * DSImageFileFormatString(DSImageFileFormat imageFileFormat);

/// Status information returned by many DSAPI low level functions.
enum DSStatus
{
    DS_NO_ERROR = 0,        ///< Call was successful.
    DS_FAILURE,             ///< Call failed.
    DS_INVALID_CALL,        ///< Can't call this function in the current "state" (before/after startCapture(), etc)
    DS_INVALID_ARG,         ///< The user passed an argument that is invalid for this function
    DS_INVALID_CALIBRATION, ///< Device has not been calibrated or has invalid calibration data
    DS_HARDWARE_MISSING,    ///< The device is not plugged in / not enumerating
    DS_HARDWARE_ERROR,      ///< Something bad happened on the hardware
    DS_HARDWARE_IN_USE,     ///< Hardware is being used by another application.
    DS_FILE_ERROR,          ///< Failed to find, read from, or write to file
    DS_MEMORY_ERROR,        ///< Ran out of memory
    DS_UNSUPPORTED_CONFIG,  ///< Current configuration setting is unsupported, change your configuration
    DS_FIRMWARE_OUT_OF_DATE ///< Device needs a firmware update before it can be used with this version of DSAPI
};
DS_DECL const char * DSStatusString(DSStatus status);

/// DSWhichImager specifies which imager (or both) is (are) being addressed
enum DSWhichImager
{
    DS_LEFT_IMAGER = 0x1,       ///< Address only the left imager
    DS_RIGHT_IMAGER = 0x2,      ///< Address only the right imager
    DS_BOTH_IMAGERS = 0x3,      ///< Address both imagers
    DS_THIRD_IMAGER = 0x10,     ///< Address Third Imager
    DS_ALL_THREE_IMAGERS = 0x13 ///< Address Left, Right and Third imager
};

/// DSWhichSensor specifies which sensor is being addressed
enum DSWhichSensor
{
    DS_TEMPERATURE_SENSOR_1 = 0x1000 ///< Address individual temperature sensors, if any are present
};

/// DSAPI chip types
enum DSChipType
{
    DS3_CHIP = 30,
    DS4_CHIP = 40,
    DS5_CHIP = 50
};

/// Power line frequence options
enum DSPowerLineFreqOption
{
    DS_POWER_LINE_FREQ_50 = 50,
    DS_POWER_LINE_FREQ_60 = 60
};

/// Stereo depth computation control parameters, can be set and get via DSHardware interface methods.
/// robinsMunroeMinus(Plus)Increment:
///   Sets the value to subtract(add) when estimating the median of the correlation surface.
///   RobbinsMunroe is an incremental algorithm for approximating the median of set of numbers.  It works by maintaining a running
///   estimate of the median.  If an incoming value is smaller than the estimate, the estimate is reduced by a certain amount, the
///   minusIncrement. The estimated median is used to evaluate the distinctiveness of the winning disparity.
/// medianThreshold:
///   Sets a threshold by how much the winning score must beat the median.
///   RobbinsMunroe is an incremental algorithm for approximating the median of set of numbers.  It works by maintaining a running
///   estimate of the median.  After the whole disparity search has been performed for a pixel, the result is evaluated based upon
///   the distinctiveness of its correlation score. The score must be a threshold less than the median.
/// scoreMinimum(Maximum)Threshold:
///   Sets the minimum and maximum correlation score that is considered acceptable.
/// textureCountThreshold:
///   Set parameter for determining whether the texture in the region is sufficient to justify a depth result.
///   Some minimum level of texture is required in the region of a pixel to determine stereo correspondence.
///   The textureCountThresh specifies how many neighboring pixels in a 7x7 area surrounding the current pixel
///   must meet the threshold on their difference in intensity from the current pixel.
/// textureDifferenceThreshold:
///   Set parameter for determining whether the texture in the region is sufficient to justify a depth result.
///   Some minimum level of texture is required in the region of a pixel to determine stereo correspondence.
///   The textureDifferenceThresh specifies how much a neighboring pixel must differ from its neighbors to be deemed countable texture.
/// secondPeakThreshold:
///   Sets the threshold on how much the minimum correlation score must differ from the next best score.
/// neighborThreshold:
///   Sets the threshold on how much at least one adjacent disparity score must differ from the minimum score.
/// lrThreshold:
///   Determines the current threshold for determining whether the left-right match agrees with the right-left match.
///   A traditional means of determining whether the disparity estimate for a pixel is correct is the so-called
///   left-right check. The left-right disparity is computed with 5 bits of sub-pixel information.  The right-left match is
///   computed to the nearest integer disparity. Clearly, in general, we cannot expect better than one half pixel difference
///   between the two. The units of this value are 32nds of a pixel.  Thus a value of 16 would indicate a half-pixel threshold.
struct DSDepthControlParameters
{
    uint32_t robinsMunroeMinusIncrement;
    uint32_t robinsMunroePlusIncrement;
    uint32_t medianThreshold;
    uint32_t scoreMinimumThreshold;
    uint32_t scoreMaximumThreshold;
    uint32_t textureCountThreshold;
    uint32_t textureDifferenceThreshold;
    uint32_t secondPeakThreshold;
    uint32_t neighborThreshold;
    uint32_t lrThreshold;
};

#if _MSC_VER < 1800 // Visual Studio versions prior to 2013 do not allow functions to be declared deleted
#define DS_DELETED_FUNCTION
#else
#define DS_DELETED_FUNCTION = delete
#endif

// Some functions are not yet implemented. They should not be called in this release.
#ifdef _MSC_VER
#define DS_NOT_IMPLEMENTED __declspec(deprecated("Function not yet implemented."))
#else
#define DS_NOT_IMPLEMENTED
#endif

/// @}
