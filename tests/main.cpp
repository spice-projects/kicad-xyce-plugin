#include <gtest/gtest.h>
#include <wx/init.h>

int main(int argc, char** argv) {
    // initialize wxWidgets
    wxInitializer initializer(argc, argv);
    // initialize google test
    testing::InitGoogleTest(&argc, argv);
    // run all tests
    return RUN_ALL_TESTS();
}
