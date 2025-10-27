#ifndef UUGEAR_MEGA4_LIB_MEGA4TYPES_HPP
#define UUGEAR_MEGA4_LIB_MEGA4TYPES_HPP

#include <string>
#include <cstdint>

namespace UUGear::Mega4
{
    struct DeviceInfo;
    struct PortConnectionInfo;
}

struct UUGear::Mega4::DeviceInfo
{
    std::string busPortPath; ///< Example: "1-1.2"
    uint16_t vid; ///< Vendor ID (0x2109)
    uint16_t pid; ///< Product ID (0x2817 or 0x0817)
    std::string description; ///< Human-readable description
};

struct UUGear::Mega4::PortConnectionInfo
{
    int portNumber; ///< 1–4 for MEGA4
    bool hasDevice; ///< True if a device is connected
    uint16_t vid = 0; ///< Vendor ID (if any)
    uint16_t pid = 0; ///< Product ID (if any)
    std::string manufacturer; ///< Optional string from descriptor
    std::string product; ///< Optional string from descriptor
};

#endif //UUGEAR_MEGA4_LIB_MEGA4TYPES_HPP
