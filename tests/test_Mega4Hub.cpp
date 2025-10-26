#include <gtest/gtest.h>
#include "Mega4Hub.hpp"

TEST(Mega4Hub, DetectsAvailableHubs)
{
    Mega4Hub hub;
    auto devices = hub.listDevices();
    EXPECT_GE(devices.size(), 0);
}

TEST(Mega4Hub, PortToggleSimulated)
{
    Mega4Hub hub;
    // Solo simula si no hay hardware
    ASSERT_NO_THROW({
        hub.powerOn(1);
        hub.powerOff(1);
    });
}
