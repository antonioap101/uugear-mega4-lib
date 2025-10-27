#include <thread>
#include <gtest/gtest.h>
#include "UUGear/Mega4/Mega4Hub.hpp"
#include "UUGear/Mega4/Mega4Types.hpp"

TEST(Mega4Hub, DetectsAvailableHubs)
{
    UUGear::Mega4::Mega4Hub hub;
    const auto devices = hub.listDevices();
    EXPECT_GE(devices.size(), 0);
}

TEST(Mega4Hub, PortToggleSimulated)
{
    UUGear::Mega4::Mega4Hub hub;
    // Solo simula si no hay hardware
    ASSERT_NO_THROW({
        hub.powerOn(1);
        std::this_thread::sleep_for(std::chrono::seconds(1)); // small delay to simulate time for power on
        hub.powerOff(1);
        });
}

TEST(Mega4Hub, InvalidPortHandling)
{
    UUGear::Mega4::Mega4Hub hub;
    // Probar con un puerto inválido
    EXPECT_THROW(hub.powerOn(99), std::out_of_range);
    EXPECT_THROW(hub.powerOff(99), std::out_of_range);
}

TEST(Mega4Hub, PowerOnAssert)
{
    UUGear::Mega4::Mega4Hub hub;
    // Turns on a port and verifies no exception is thrown
    ASSERT_NO_THROW({
        hub.powerOff(2);
        std::this_thread::sleep_for(std::chrono::seconds(1)); // small delay to simulate time for power on
        hub.powerOn(2);
        });
    // Check that the port is on
    EXPECT_TRUE(hub.isPortOn(2));
}

TEST(Mega4Hub, PowerOffAssert)
{
    UUGear::Mega4::Mega4Hub hub;

    // Turns off a port and verifies no exception is thrown
    ASSERT_NO_THROW({
        hub.powerOn(2);
        std::this_thread::sleep_for(std::chrono::seconds(1)); // small delay to simulate time for power on
        hub.powerOff(2);
        });
    // Check that the port is off
    EXPECT_FALSE(hub.isPortOn(2));
}


TEST(Mega4Hub, MultiplePortControlSimulated)
{
    UUGear::Mega4::Mega4Hub hub;
    // Simula el encendido y apagado de múltiples puertos
    ASSERT_NO_THROW({
        for (int port = 1; port <= 4; ++port)
        {
        hub.powerOn(port);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)); // small delay to simulate time for power on
        for (int port = 1; port <= 4; ++port)
        {
        hub.powerOff(port);
        }
        });
}


TEST(Mega4Hub, PortConnectionEnumeration)
{
    UUGear::Mega4::Mega4Hub hub;

    ASSERT_NO_THROW({
        const auto devices = hub.listDevices();

        // Solo se ejecuta si hay al menos un hub detectado
        if (devices.empty())
        {
        std::cout << "No MEGA4 hubs detected — skipping connection test.\n";
        return;
        }

        const auto connections = hub.getPortConnections(0);
        EXPECT_EQ(connections.size(), 4);

        for (const auto& port : connections)
        {
            EXPECT_GE(port.portNumber, 1);
            EXPECT_LE(port.portNumber, 4);

            // Si hay un dispositivo, debe tener un VID/PID válidos
            if (port.hasDevice)
            {
                EXPECT_GT(port.vid, 0);
                EXPECT_GT(port.pid, 0);
                std::cout << "[Port " << port.portNumber << "] "
                << "VID:PID = " << std::hex << port.vid << ":" << port.pid << std::dec
                << " (" << port.manufacturer << " " << port.product << ")\n";
            }
            else
            {
                std::cout << "[Port " << port.portNumber << "] empty\n";
            }
        }


        });
}

TEST(Mega4Hub, PortConnectionAndPowerStateConsistency)
{
    UUGear::Mega4::Mega4Hub hub;

    ASSERT_NO_THROW({
        const auto devices = hub.listDevices();
        if (devices.empty())
        {
            std::cout << "No MEGA4 hubs detected — skipping power consistency test.\n";
            return;
        }

        const auto states = hub.getPortStates(0);
        const auto connections = hub.getPortConnections(0);

        EXPECT_EQ(states.size(), 4);
        EXPECT_EQ(connections.size(), 4);

        // Si un puerto tiene un dispositivo, normalmente debería estar encendido
        for (size_t i = 0; i < states.size(); ++i)
        {
            if (connections[i].hasDevice) {
                EXPECT_TRUE(states[i]) << "Port " << (i + 1) << " reports a device but power state is OFF.";
            }
            std::cout << "[Port " << (i + 1) << "] "
                      << "Power State: " << (states[i] ? "ON" : "OFF") << ", "
                      << "Has Device: " << (connections[i].hasDevice ? "YES" : "NO") << "\n";
        }
        });
}
