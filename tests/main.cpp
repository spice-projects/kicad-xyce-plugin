#include <gtest/gtest.h>

int main(int argc, char** argv) {
    // initialize google test
    testing::InitGoogleTest(&argc, argv);
    // run all tests
    return RUN_ALL_TESTS();
}
