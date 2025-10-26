#ifndef UUGEAR_MEGA4HUB_HPP
#define UUGEAR_MEGA4HUB_HPP

#include <vector>
#include <string>
#include <cstdint>

/**
 * @brief C++ interface for controlling the UUGear MEGA4 USB hub.
 *        It uses libusb to perform standard USB Hub Class requests.
 */
class Mega4Hub
{
public:
    struct DeviceInfo
    {
        std::string busPortPath; ///< Example: "1-1.2"
        uint16_t vid; ///< Vendor ID (0x2109)
        uint16_t pid; ///< Product ID (0x2817 or 0x0817)
        std::string description; ///< Human-readable description
    };

    /**
     * @brief Initializes libusb. Does not open any device yet.
     * @throws std::runtime_error if libusb initialization fails.
     */
    Mega4Hub();

    /**
     * @brief Frees all resources used by libusb.
     */
    ~Mega4Hub();

    /**
     * @brief Scans the USB bus for VIA Labs VL817 hubs (used in MEGA4).
     * @return A list of detected hubs.
     */
    std::vector<DeviceInfo> listDevices() const;

    /**
     * @brief Turns a port ON.
     * @param port Port number (1–4).
     * @param deviceIndex Index of the detected hub (default = 0).
     */
    void powerOn(int port, int deviceIndex = 0) const;

    /**
     * @brief Turns a port OFF.
     * @param port Port number (1–4).
     * @param deviceIndex Index of the detected hub (default = 0).
     */
    void powerOff(int port, int deviceIndex = 0) const;

    /**
     * @brief Enables or disables simulation mode (no hardware access).
     *        Useful for unit tests.
     */
    void setSimulationMode(bool enabled) const;

private:
    struct Impl;
    Impl* pImpl; ///< PIMPL pattern to hide implementation details.
};

#endif // UUGEAR_MEGA4HUB_HPP
