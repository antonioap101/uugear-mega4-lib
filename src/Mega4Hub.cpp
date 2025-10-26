#include "Mega4Hub.hpp"
#include <libusb.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <stdexcept>

// Known IDs for VIA Labs VL817 controller used by MEGA4
constexpr uint16_t MEGA4_VENDOR_ID = 0x2109;
constexpr uint16_t MEGA4_PID_USB2 = 0x2817;
constexpr uint16_t MEGA4_PID_USB3 = 0x0817;

// USB hub feature selector for port power
constexpr uint16_t PORT_POWER = 8; // USB_PORT_FEAT_POWER

struct Mega4Hub::Impl
{
    libusb_context* ctx = nullptr;
    std::vector<libusb_device*> mega4Devices;

    Impl()
    {
        if (libusb_init(&ctx) != 0)
        {
            throw std::runtime_error("Failed to initialize libusb");
        }
        // Try to search for MEGA4 devices at initialization
        list();
    }

    ~Impl()
    {
        for (auto* d : mega4Devices) libusb_unref_device(d);
        libusb_exit(ctx);
    }

    std::vector<DeviceInfo> list()
    {
        std::vector<DeviceInfo> foundDevices = {};
        libusb_device** list = nullptr;
        const ssize_t devCount = libusb_get_device_list(ctx, &list);

        if (devCount < 0) return foundDevices;

        for (ssize_t i = 0; i < devCount; ++i)
        {
            libusb_device* dev = list[i];
            libusb_device_descriptor desc{};

            if (libusb_get_device_descriptor(dev, &desc) != 0 ||
                desc.idVendor != MEGA4_VENDOR_ID || desc.idProduct != MEGA4_PID_USB2 && desc.idProduct !=
                MEGA4_PID_USB3)
            {
                continue;
            }

            libusb_ref_device(dev);
            mega4Devices.push_back(dev);

            const uint8_t bus = libusb_get_bus_number(dev);
            uint8_t portPath[8];
            const int pathLen = libusb_get_port_numbers(dev, portPath, sizeof(portPath));

            std::string path = std::to_string(bus);
            for (int j = 0; j < pathLen; ++j)
                path += "-" + std::to_string(portPath[j]);

            DeviceInfo info;
            info.busPortPath = path;
            info.vid = desc.idVendor;
            info.pid = desc.idProduct;
            info.description = "VIA Labs VL817 Hub (" + std::string(desc.idProduct == MEGA4_PID_USB3 ? "USB3" : "USB2")
                + ")";
            foundDevices.push_back(info);
        }
        libusb_free_device_list(list, 1);
        return foundDevices;
    }

    void togglePower(const int mega4DeviceIdx, const int mega4PortNumber, const bool on) const
    {
        std::cout << "Port " << mega4PortNumber << (on ? " ON" : " OFF") << " (hub " << mega4DeviceIdx << ")\n";

        if (mega4DeviceIdx >= static_cast<int>(mega4Devices.size()))
        {
            throw std::out_of_range("Invalid hub index. Tried to access hub " + std::to_string(mega4DeviceIdx) +
                " but only " + std::to_string(mega4Devices.size()) + " hubs are available.");
        }

        if (mega4PortNumber < 1 || mega4PortNumber > 4)
        {
            throw std::out_of_range("Invalid port number. MEGA4 has ports 1 to 4.");
        }


        libusb_device* dev = mega4Devices[mega4DeviceIdx];
        libusb_device_handle* handle = nullptr;

        if (libusb_open(dev, &handle) != 0)
        {
            throw std::runtime_error("Failed to open USB device");
        }

        constexpr uint16_t feature = PORT_POWER;
        constexpr uint8_t bmRequestType = LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_OTHER;
        const uint8_t request = on ? LIBUSB_REQUEST_SET_FEATURE : LIBUSB_REQUEST_CLEAR_FEATURE;

        const int ret = libusb_control_transfer(handle,
                                                bmRequestType,
                                                request,
                                                feature,
                                                mega4PortNumber,
                                                nullptr,
                                                0,
                                                1000);
        libusb_close(handle);

        if (ret < 0)
        {
            throw std::runtime_error("Failed to send control transfer to MEGA4");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    [[nodiscard]] std::array<bool, 4> getPortStates(const int deviceIndex) const
    {
        std::array<bool, 4> states{false, false, false, false};

        if (deviceIndex >= static_cast<int>(mega4Devices.size()))
            throw std::out_of_range("Invalid hub index");

        libusb_device* dev = mega4Devices[deviceIndex];
        libusb_device_handle* handle = nullptr;
        if (libusb_open(dev, &handle) != 0)
            throw std::runtime_error("Failed to open USB device");

        for (int port = 1; port <= 4; ++port)
        {
            uint8_t status[4] = {0};
            const int ret = libusb_control_transfer(
                handle,
                LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_OTHER,
                LIBUSB_REQUEST_GET_STATUS,
                0,
                port,
                status,
                sizeof(status),
                1000
            );

            if (ret == 4)
            {
                const auto wPortStatus = static_cast<uint16_t>(status[0] | (status[1] << 8));
                states[port - 1] = (wPortStatus & 0x0100) != 0; // Bit 8 = PORT_POWER
            }
            else
            {
                std::cerr << "Warning: failed to get port " << port << " status (ret=" << ret << ")\n";
            }
        }

        libusb_close(handle);
        return states;
    }

    [[nodiscard]] std::vector<PortConnectionInfo> getPortConnections(const int deviceIndex) const
    {
        std::vector<PortConnectionInfo> ports(4);
        for (int i = 0; i < 4; ++i)
            ports[i].portNumber = i + 1;

        if (deviceIndex >= static_cast<int>(mega4Devices.size()))
            throw std::out_of_range("Invalid hub index. Tried to access hub " + std::to_string(deviceIndex) +
                " but only " + std::to_string(mega4Devices.size()) + " hubs are available.");

        const libusb_device* hubDev = mega4Devices[deviceIndex];

        libusb_device** list = nullptr;
        const ssize_t cnt = libusb_get_device_list(ctx, &list);
        if (cnt < 0)
            return ports;

        for (ssize_t i = 0; i < cnt; ++i)
        {
            libusb_device* dev = list[i];
            const libusb_device* parent = libusb_get_parent(dev);
            if (parent != hubDev) continue;

            libusb_device_descriptor desc{};
            if (libusb_get_device_descriptor(dev, &desc) != 0)
                continue;

            const uint8_t port = libusb_get_port_number(dev);
            if (port < 1 || port > 4)
                continue;

            PortConnectionInfo& info = ports[port - 1];
            info.hasDevice = true;
            info.vid = desc.idVendor;
            info.pid = desc.idProduct;

            libusb_device_handle* handle = nullptr;

            if (libusb_open(dev, &handle) != 0) continue;

            unsigned char buf[256];
            if (desc.iManufacturer &&
                libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, buf, sizeof(buf)) > 0)
                info.manufacturer = reinterpret_cast<char*>(buf);

            if (desc.iProduct &&
                libusb_get_string_descriptor_ascii(handle, desc.iProduct, buf, sizeof(buf)) > 0)
                info.product = reinterpret_cast<char*>(buf);

            libusb_close(handle);
        }

        libusb_free_device_list(list, 1);
        return ports;
    }
};

Mega4Hub::Mega4Hub() : pImpl(new Impl)
{
}

Mega4Hub::~Mega4Hub() { delete pImpl; }

std::vector<Mega4Hub::DeviceInfo> Mega4Hub::listDevices() const { return pImpl->list(); }
std::array<bool, 4> Mega4Hub::getPortStates(const int deviceIndex) const { return pImpl->getPortStates(deviceIndex); }

std::vector<Mega4Hub::PortConnectionInfo>
Mega4Hub::getPortConnections(const int deviceIndex) const { return pImpl->getPortConnections(deviceIndex); }

void Mega4Hub::powerOn(const int port, const int deviceIndex) const { pImpl->togglePower(deviceIndex, port, true); }

void Mega4Hub::powerOff(const int port, const int deviceIndex) const { pImpl->togglePower(deviceIndex, port, false); }

bool Mega4Hub::isPortOn(const int port, const int deviceIndex) const
{
    const auto states = pImpl->getPortStates(deviceIndex);
    if (port < 1 || port > 4)
        throw std::out_of_range("Invalid port number. MEGA4 has ports 1 to 4.");
    return states[port - 1];
}
