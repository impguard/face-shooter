#pragma once

#include "DSAPI/DSAPITypes.h"

/// @class DSHardware
/// Defines methods specific to a hardware implementation.
class DS_DECL DSHardware
{
public:
    /// Returns true if the hardware is active/ready.
    virtual bool isHardwareActive() = 0;

    /// Returns the version of DS ASIC used to compute data.
    virtual DSChipType getChipVersion() = 0;

    /// Turn autoexposure on or off. Must be called after the camera has been initialized and capture has begun.
    virtual bool setAutoExposure(DSWhichImager whichImager, bool state) = 0;
    /// Returns true if autoexposure is enabled.
    virtual bool getAutoExposure(DSWhichImager whichImager, bool & state) = 0;

    /// Sets imager gain factor. For camera without internal auto exposure control.
    /// @param gain the gain factor
    /// @param which for which imager (or both) do we set the gain
    /// @return true for success, false on error.
    virtual bool setImagerGain(float gain, DSWhichImager whichImager) = 0;
    /// Gets imager gain factor. For camera without internal auto exposure control.
    /// @param[out] gain the gain factor.
    /// @param which for which imager do we get the gain.
    /// @return true for success, false on error.
    virtual bool getImagerGain(float & gain, DSWhichImager whichImager) = 0;
    /// Gets imager minimum gain factor. For camera without internal auto exposure control.
    /// @param[out] minGain the minimum gain factor.
    /// @param which for which imager do we get the minimum gain factor.
    /// @return true for success, false on error.
    virtual bool getImagerMinMaxGain(float & minGain, float & maxGain, DSWhichImager whichImager) = 0;

    /// Sets imager exposure time (in ms). For camera without internal auto exposure control.
    /// @param exposureTime the exposure time.
    /// @param which for which imager (or both) do we set the exposure time.
    /// @return true for success, false on error.
    virtual bool setImagerExposure(float exposure, DSWhichImager whichImager) = 0;
    /// Gets imager exposure time (in ms). For camera without internal auto exposure control.
    /// @param[out] exposure the exposure time.
    /// @param which for which imager do we get the exposure time.
    /// @return true for success, false on error.
    virtual bool getImagerExposure(float & exposure, DSWhichImager whichImager) = 0;
    /// Gets imager minimum exposure time (in ms). For camera without internal auto exposure control.
    /// @param[out] minExposure the minimum exposure time.
    /// @param which for which imager do we get the minimum exposure time.
    /// @return true for success, false on error.
    virtual bool getImagerMinMaxExposure(float & minExposure, float & maxExposure, DSWhichImager whichImager) = 0;

    /// Not validated.
    /// Sets Brightness
    virtual bool setBrightness(int val, DSWhichImager whichImager) = 0;
    virtual bool getBrightness(int & val, DSWhichImager whichImager) = 0;
    virtual bool getMinMaxBrightness(int & min, int & max, DSWhichImager whichImager) = 0;

    /// Not validated.
    /// Sets contrast, which is a value expressed as a gain factor multiplied by 100. Windows range is 0 - 10000, default is 100 (x1)
    virtual bool setContrast(int val, DSWhichImager whichImager) = 0;
    virtual bool getContrast(int & val, DSWhichImager whichImager) = 0;
    virtual bool getMinMaxContrast(int & min, int & max, DSWhichImager whichImager) = 0;

    /// Not validated.
    /// Sets saturation, which is a value expressed as a gain factor multiplied by 100. Windows range is 0 - 10000, default is 100 (x1)
    virtual bool setSaturation(int val, DSWhichImager whichImager) = 0;
    virtual bool getSaturation(int & val, DSWhichImager whichImager) = 0;
    virtual bool getMinMaxSaturation(int & min, int & max, DSWhichImager whichImager) = 0;

    /// Not validated.
    /// Sets hue, which is a value expressed as degrees multiplied by 100. Windows & UVC range is -18000 to 18000 (-180 to +180 degrees), default is 0
    virtual bool setHue(int val, DSWhichImager whichImager) = 0;
    virtual bool getHue(int & val, DSWhichImager whichImager) = 0;

    /// Not validated.
    /// Sets gamma, this is expressed as gamma multiplied by 100. Windows and UVC range is 1 to 500.
    virtual bool setGamma(int val, DSWhichImager whichImager) = 0;
    virtual bool getGamma(int & val, DSWhichImager whichImager) = 0;

    /// Not validated.
    /// Sets white balance. Color temperature, in degrees Kelvin. Windows has no defined range.  UVC range is 2800 (incandescent) to 6500 (daylight) but still needs to provide range
    virtual bool setWhiteBalance(int val, DSWhichImager whichImager) = 0;
    virtual bool getWhiteBalance(int & val, DSWhichImager whichImager) = 0;
    virtual bool getMinMaxWhiteBalance(int & min, int & max, DSWhichImager whichImager) = 0;

    /// Not validated.
    /// Sets sharpness. Arbitrary units. UVC has no specified range (min sharpness means no sharpening), Windows required range must be 0 through 100. The default value must be 50.
    virtual bool setSharpness(int val, DSWhichImager whichImager) = 0;
    virtual bool getSharpness(int & val, DSWhichImager whichImager) = 0;
    virtual bool getMinMaxSharpness(int & min, int & max, DSWhichImager whichImager) = 0;

    /// Not validated.
    /// Sets back light compensation. A value of false indicates that the back-light compensation is disabled. The default value of true indicates that the back-light compensation is enabled.
    virtual bool setBacklightCompensation(bool val, DSWhichImager whichImager) = 0;
    virtual bool getBacklightCompensation(bool & val, DSWhichImager whichImager) = 0;

    /// Not validated.
    /// Sends an i2c command to the imager(s) designated by DSWhichImagers. Register regAddress is given value regValue.
    /// @param DSWhichImagers one of DS_LEFT_IMAGER, DS_RIGHT_IMAGER DS_BOTH_IMAGERS
    /// @param regAddress  the i2c register address
    /// @param regValue  the value to set register regAddress to. This value could be 2 bytes or 4 bytes for the third image and the overloaded versions are for this.
    /// @param noCheck if true, do not check whether the write occurred correctly.
    /// @return false on fail else true.
    virtual bool setImagerRegister(DSWhichImager whichImager, uint16_t regAddress, uint8_t regValue, bool noCheck = false) = 0;
    virtual bool setImagerRegister(DSWhichImager whichImager, uint16_t regAddress, uint16_t regValue, bool noCheck = false) = 0;
    virtual bool setImagerRegister(DSWhichImager whichImager, uint16_t regAddress, uint32_t regValue, bool noCheck = false) = 0;
    /// Sends an i2c command to the imager designated by whichImagers. regValue is set to the contents of Register regAddress.
    /// @param DSWhichImagers one of DS_LEFT_IMAGER, DS_RIGHT_IMAGER DS_BOTH_IMAGERS
    /// @param regAddress  the i2c register address
    /// @param[out] regValue  where to put the value of register regAddress. This value could be 2 bytes or 4 bytes for the third image and the overloaded versions are for this.
    /// @return false on fail else true.
    virtual bool getImagerRegister(DSWhichImager whichImager, uint16_t regAddress, uint8_t & regValue) = 0;
    virtual bool getImagerRegister(DSWhichImager whichImager, uint16_t regAddress, uint16_t & regValue) = 0;
    virtual bool getImagerRegister(DSWhichImager whichImager, uint16_t regAddress, uint32_t & regValue) = 0;

    /// Not validated.
    /// Sets third power line frequency.
    virtual bool setThirdPowerLineFrequency(DSPowerLineFreqOption plf) = 0;
    virtual bool getThirdPowerLineFrequency(DSPowerLineFreqOption & plf) = 0;

    /// Not validated.
    /// Reads temperature to describe the current temperture of the camera head.
    /// @param curTemperature current temperture in Celsius.
    /// @param minTemperature minimal temperature in Celsius since last reset.
    /// @param maxTemperature maximal temperature in Celsius since last reset.
    /// @param minFaultThreshold mimimal temperature allowed in Celsius for the module to operate.
    /// @return true for success, false on error.
    virtual bool getTemperature(int8_t & curTemperature, int8_t & minTemperature, int8_t & maxTemperature, int8_t & minFaultThreshold) = 0;
    virtual bool resetMinMaxRecordedTemperture() = 0;

    /// Set/get parameters that affect the stereo algorithm. See DSAPITypes.h for documentation.
    virtual bool setDepthControlParameters(const DSDepthControlParameters & parameters) = 0;
    virtual bool getDepthControlParameters(DSDepthControlParameters & parameters) = 0;

protected:
    // Creation (and deletion) of an object of this
    // type is supported through the DSFactory functions.
    DSHardware(){};
    DSHardware(const DSHardware & other) DS_DELETED_FUNCTION;
    DSHardware(DSHardware && other) DS_DELETED_FUNCTION;
    DSHardware & operator=(const DSHardware & other) DS_DELETED_FUNCTION;
    DSHardware & operator=(DSHardware && other) DS_DELETED_FUNCTION;
    virtual ~DSHardware(){};
};
