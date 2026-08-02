#include <gtest/gtest.h>

#include "simulation_parameters/simulation_config.h"

TEST(SimulationConfigOutputPathChecks, raw_output_file_path_returns_nullopt_for_non_raw_format) {
    // arrange
    const auto config = SimulationConfig::from_xyce_directives({".OP", ".PRINT DC V(1)"});
    // act
    auto raw_path = config.raw_output_file_path("/tmp", "/tmp/test.cir");
    // assert
    ASSERT_FALSE(raw_path.has_value());
}

TEST(SimulationConfigOutputPathChecks, raw_output_file_path_uses_print_file) {
    // arrange
    const auto config = SimulationConfig::from_xyce_directives({".TRAN 1u 1m", ".PRINT TRAN FORMAT=RAW FILE=output.raw V(1)"});
    // act
    auto raw_path = config.raw_output_file_path("/tmp/work", "/tmp/work/test.cir");
    // assert
    ASSERT_TRUE(raw_path.has_value());
    ASSERT_EQ(raw_path->string(), "/tmp/work/output.raw");
}

TEST(SimulationConfigOutputPathChecks, raw_output_file_path_appends_raw_suffix) {
    // arrange
    const auto config = SimulationConfig::from_xyce_directives({".TRAN 1u 1m", ".PRINT TRAN FORMAT=RAW V(1)"});
    // act
    auto raw_path = config.raw_output_file_path("/tmp/work", "/tmp/work/test.cir");
    // assert
    ASSERT_TRUE(raw_path.has_value());
    ASSERT_EQ(raw_path->string(), "/tmp/work/test.cir.raw");
}

TEST(SimulationConfigOutputPathChecks, fft_output_file_path_returns_nullopt_for_op_analysis) {
    // arrange
    const auto config = SimulationConfig::from_xyce_directives({".OP"});
    // act
    auto fft_path = config.fft_output_file_path_pattern("/tmp/test.cir");
    // assert
    ASSERT_FALSE(fft_path.has_value());
}

TEST(SimulationConfigOutputPathChecks, fft_output_file_path_returns_pattern_for_tran_with_fft) {
    // arrange
    const auto config = SimulationConfig::from_xyce_directives({".TRAN 1u 1m", ".FFT V(1)"});
    // act
    auto fft_path = config.fft_output_file_path_pattern("/tmp/test.cir");
    // assert
    ASSERT_TRUE(fft_path.has_value());
    ASSERT_EQ(fft_path->string(), "/tmp/test.cir.fft*");
}

TEST(SimulationConfigOutputPathChecks, fft_output_file_path_returns_nullopt_for_tran_without_fft) {
    // arrange
    const auto config = SimulationConfig::from_xyce_directives({".TRAN 1u 1m"});
    // act
    auto fft_path = config.fft_output_file_path_pattern("/tmp/test.cir");
    // assert
    ASSERT_FALSE(fft_path.has_value());
}

TEST(SimulationConfigTopologyChecks, expands_wildcards_in_analysis_print) {
    // arrange
    const auto [netlist, topology] = parse_netlist("Title\nR1 1 0 100\nR2 2 0 200\n.END\n");
    const auto config = SimulationConfig::from_xyce_directives({".OP", ".PRINT DC V(*)"});
    // act
    const auto directives = config.to_xyce_directives(&topology);
    // assert
    bool found_print = false;
    for (const auto& d : directives) {
        if (d.find(".PRINT DC") == 0) {
            found_print = true;
            // V(*) expanded to V(0), V(1), V(2)
            ASSERT_NE(d.find("V(0)"), std::string::npos);
            ASSERT_NE(d.find("V(1)"), std::string::npos);
            ASSERT_NE(d.find("V(2)"), std::string::npos);
            // V(*) should not appear verbatim
            ASSERT_EQ(d.find("V(*)"), std::string::npos);
        }
    }
    ASSERT_TRUE(found_print);
}

TEST(SimulationConfigTopologyChecks, expands_wildcards_in_unassociated_prints) {
    // arrange
    const auto [netlist, topology] = parse_netlist("Title\nR1 1 0 100\n.END\n");
    const auto config = SimulationConfig::from_xyce_directives({".OP", ".PRINT DC V(*)"});
    // act
    const auto directives = config.to_xyce_directives(&topology);
    // assert
    bool found_print = false;
    for (const auto& d : directives) {
        if (d.find(".PRINT DC") == 0) {
            found_print = true;
            // V(*) expanded to V(0), V(1)
            ASSERT_NE(d.find("V(0)"), std::string::npos);
            ASSERT_NE(d.find("V(1)"), std::string::npos);
        }
    }
    ASSERT_TRUE(found_print);
}

TEST(SimulationConfigTopologyChecks, passes_through_wildcards_without_topology) {
    // arrange
    const auto config = SimulationConfig::from_xyce_directives({".OP", ".PRINT DC V(*)"});
    // act
    const auto directives = config.to_xyce_directives(nullptr);
    // assert
    bool found_print = false;
    for (const auto& d : directives) {
        if (d.find(".PRINT DC") == 0) {
            found_print = true;
            // V(*) passes through verbatim without topology
            ASSERT_NE(d.find("V(*)"), std::string::npos);
        }
    }
    ASSERT_TRUE(found_print);
}

TEST(SimulationConfigTopologyChecks, round_trips_with_build_final_netlist) {
    // arrange
    const auto [sanitized, topology] = parse_netlist("Title\nR1 1 0 100\n.OP\n.PRINT DC V(*)\n.END\n");
    const auto config = SimulationConfig::from_xyce_directives(topology.m_directives);
    // act
    const auto directives = config.to_xyce_directives(&topology);
    const auto final_netlist = build_final_netlist(sanitized, directives, topology.m_passthrough_directives);
    // assert
    // V(*) should be expanded in the final netlist
    ASSERT_NE(final_netlist.find("V(0)"), std::string::npos);
    ASSERT_NE(final_netlist.find("V(1)"), std::string::npos);
    // V(*) should not appear verbatim in final netlist
    ASSERT_EQ(final_netlist.find("V(*)"), std::string::npos);
    // .END should still be present
    ASSERT_NE(final_netlist.find(".END"), std::string::npos);
}
