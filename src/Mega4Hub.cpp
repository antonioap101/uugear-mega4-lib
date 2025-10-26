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
    std::vector<libusb_device*> devices;
    bool simulation = false;

    Impl()
    {
        if (libusb_init(&ctx) != 0)
        {
            throw std::runtime_error("Failed to initialize libusb");
        }
    }

    ~Impl()
    {
        for (auto* d : devices) libusb_unref_device(d);
        libusb_exit(ctx);
    }

    std::vector<DeviceInfo> list()
    {
        std::vector<DeviceInfo> found;
        libusb_device** list = nullptr;
        ssize_t cnt = libusb_get_device_list(ctx, &list);
        if (cnt < 0) return found;

        for (ssize_t i = 0; i < cnt; ++i)
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
            devices.push_back(dev);

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
            info.description = "VIA Labs VL817 Hub (" +
                std::string(desc.idProduct == MEGA4_PID_USB3 ? "USB3" : "USB2") + ")";
            found.push_back(info);
        }
        libusb_free_device_list(list, 1);
        return found;
    }

    void toggle(const int port, const bool on, const int index) const
    {
        if (simulation)
        {
            std::cout << "[SIMULATION] Port " << port
                << (on ? " ON" : " OFF") << " (hub " << index << ")\n";
            return;
        }

        if (index >= static_cast<int>(devices.size()))
        {
            throw std::out_of_range("Invalid hub index");
        }

        libusb_device* dev = devices[index];
        libusb_device_handle* handle = nullptr;
        if (libusb_open(dev, &handle) != 0)
        {
            throw std::runtime_error("Failed to open USB device");
        }

        uint16_t feature = PORT_POWER;
        uint8_t bmRequestType = LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_OTHER;
        uint8_t request = on ? LIBUSB_REQUEST_SET_FEATURE : LIBUSB_REQUEST_CLEAR_FEATURE;

        int ret = libusb_control_transfer(handle, bmRequestType, request,
                                          feature, port, nullptr, 0, 1000);
        libusb_close(handle);

        if (ret < 0)
        {
            throw std::runtime_error("Failed to send control transfer to MEGA4");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
};

Mega4Hub::Mega4Hub() : pImpl(new Impl)
{
}

Mega4Hub::~Mega4Hub() { delete pImpl; }

std::vector<Mega4Hub::DeviceInfo> Mega4Hub::listDevices() const { return pImpl->list(); }
void Mega4Hub::powerOn(const int port, const int index) const { pImpl->toggle(port, true, index); }
void Mega4Hub::powerOff(const int port, const int index) const { pImpl->toggle(port, false, index); }
void Mega4Hub::setSimulationMode(const bool e) const { pImpl->simulation = e; }
