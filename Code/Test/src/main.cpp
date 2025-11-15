#include "gmock/gmock.h"
#include "gtest/gtest.h"

int main(int argc, char** argv)
{
    // Initialize Google Test/Mock ONLY
    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}