#ifndef UUGEAR_MEGA4HUB_HPP
#define UUGEAR_MEGA4HUB_HPP

#include <vector>
#include <array>

namespace UUGear::Mega4
{
    class Mega4Hub;
    struct DeviceInfo;
    struct PortConnectionInfo;
}

/**
 * @brief C++ interface for controlling the UUGear MEGA4 USB hub.
 *        It uses libusb to perform standard USB Hub Class requests.
 */
class UUGear::Mega4::Mega4Hub
{
public:
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
    [[nodiscard]] virtual std::vector<DeviceInfo> listDevices() const;

    /**
     * @brief Turns a port ON.
     * @param port Port number (1–4).
     * @param deviceIndex Index of the detected hub (default = 0).
     */
    virtual void powerOn(int port, int deviceIndex = 0) const;

    /**
     * @brief Turns a port OFF.
     * @param port Port number (1–4).
     * @param deviceIndex Index of the detected hub (default = 0).
     */
    virtual void powerOff(int port, int deviceIndex = 0) const;

    /**
     * @brief Queries the actual ON/OFF power state for all four ports
     *        of a specific MEGA4 hub by reading from hardware.
     * @param deviceIndex Index of the detected hub (default = 0).
     * @return Array<bool,4> with true = ON, false = OFF.
     * @throws std::out_of_range or std::runtime_error on failure.
     */
    [[nodiscard]] virtual std::array<bool, 4> getPortStates(int deviceIndex = 0) const;

    [[nodiscard]] virtual bool isPortOn(int port, int deviceIndex = 0) const;

    /**
 * @brief Lists all devices connected to each of the 4 downstream ports
 *        of a MEGA4 hub.
 * @param deviceIndex Index of the detected hub (default = 0)
 * @return Vector of 4 entries, one per port.
 */
    [[nodiscard]] virtual std::vector<PortConnectionInfo> getPortConnections(int deviceIndex = 0) const;

private:
    struct Impl;
    Impl* pImpl; ///< PIMPL pattern to hide implementation details.
};

#endif // UUGEAR_MEGA4HUB_HPP
