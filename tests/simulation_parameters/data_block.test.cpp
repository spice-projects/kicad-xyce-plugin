#include <gtest/gtest.h>

#include "simulation_parameters/data_block.h"

// ========================================================================================
// from_xyce_directives
// ========================================================================================

TEST(DataBlockChecks, parse_single_block_flattened) {
    // arrange
    const std::vector<std::string> directives = {
        ".DATA myTable r1 r2 1.0 2.0 3.0 4.0",
        ".ENDDATA",
    };
    // act
    const auto blocks = DataBlock::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(blocks.size(), 1);
    ASSERT_EQ(blocks[0].name, "myTable");
    ASSERT_EQ(blocks[0].parameters.size(), 2);
    ASSERT_EQ(blocks[0].parameters[0], "r1");
    ASSERT_EQ(blocks[0].parameters[1], "r2");
    ASSERT_EQ(blocks[0].records.size(), 2);
    ASSERT_EQ(blocks[0].records[0].size(), 2);
    ASSERT_EQ(blocks[0].records[0][0], "1.0");
    ASSERT_EQ(blocks[0].records[0][1], "2.0");
    ASSERT_EQ(blocks[0].records[1].size(), 2);
    ASSERT_EQ(blocks[0].records[1][0], "3.0");
    ASSERT_EQ(blocks[0].records[1][1], "4.0");
}

TEST(DataBlockChecks, parse_single_column_table) {
    // arrange
    const std::vector<std::string> directives = {
        ".DATA temperatures TEMP 25.0 50.0 75.0",
        ".ENDDATA",
    };
    // act
    const auto blocks = DataBlock::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(blocks.size(), 1);
    ASSERT_EQ(blocks[0].parameters.size(), 1);
    ASSERT_EQ(blocks[0].parameters[0], "TEMP");
    ASSERT_EQ(blocks[0].records.size(), 3);
    ASSERT_EQ(blocks[0].records[0][0], "25.0");
    ASSERT_EQ(blocks[0].records[1][0], "50.0");
    ASSERT_EQ(blocks[0].records[2][0], "75.0");
}

TEST(DataBlockChecks, parse_scientific_notation_values) {
    // arrange
    const std::vector<std::string> directives = {
        ".DATA myTable r1 c1 8.0000e+00 4.0000e-09 9.0000e+00 4.0000e-09",
        ".ENDDATA",
    };
    // act
    const auto blocks = DataBlock::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(blocks.size(), 1);
    ASSERT_EQ(blocks[0].parameters.size(), 2);
    ASSERT_EQ(blocks[0].parameters[0], "r1");
    ASSERT_EQ(blocks[0].parameters[1], "c1");
    ASSERT_EQ(blocks[0].records.size(), 2);
    ASSERT_EQ(blocks[0].records[0][0], "8.0000e+00");
    ASSERT_EQ(blocks[0].records[0][1], "4.0000e-09");
    ASSERT_EQ(blocks[0].records[1][0], "9.0000e+00");
    ASSERT_EQ(blocks[0].records[1][1], "4.0000e-09");
}

TEST(DataBlockChecks, parse_multiple_blocks) {
    // arrange
    const std::vector<std::string> directives = {
        ".DATA tableA r1 1.0 2.0",
        ".ENDDATA",
        ".DATA tableB c1 1e-9 2e-9",
        ".ENDDATA",
    };
    // act
    const auto blocks = DataBlock::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(blocks.size(), 2);
    ASSERT_EQ(blocks[0].name, "tableA");
    ASSERT_EQ(blocks[1].name, "tableB");
}

TEST(DataBlockChecks, no_data_directives_returns_empty) {
    // arrange
    const std::vector<std::string> directives = {
        ".TRAN 1u 1m",
        ".STEP R1 1k 10k 1k",
    };
    // act
    const auto blocks = DataBlock::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(blocks.size(), 0);
}

TEST(DataBlockChecks, spice_suffix_values_recognized) {
    // arrange
    const std::vector<std::string> directives = {
        ".DATA myTable r1 1k 2k 5k",
        ".ENDDATA",
    };
    // act
    const auto blocks = DataBlock::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(blocks.size(), 1);
    ASSERT_EQ(blocks[0].parameters.size(), 1);
    ASSERT_EQ(blocks[0].parameters[0], "r1");
    ASSERT_EQ(blocks[0].records.size(), 3);
    ASSERT_EQ(blocks[0].records[0][0], "1k");
    ASSERT_EQ(blocks[0].records[1][0], "2k");
    ASSERT_EQ(blocks[0].records[2][0], "5k");
}

// ========================================================================================
// to_xyce_directives
// ========================================================================================

TEST(DataBlockChecks, generates_data_block_with_header_and_rows) {
    // arrange
    const DataBlock block("myTable", {"r1", "r2"}, {{"1.0", "2.0"}, {"3.0", "4.0"}});
    // act
    const auto directives = block.to_xyce_directives();
    // assert
    ASSERT_EQ(directives.size(), 5);
    ASSERT_EQ(directives[0], ".DATA myTable");
    ASSERT_EQ(directives[1], "+ r1 r2");
    ASSERT_EQ(directives[2], "+ 1.0 2.0");
    ASSERT_EQ(directives[3], "+ 3.0 4.0");
    ASSERT_EQ(directives[4], ".ENDDATA");
}

TEST(DataBlockChecks, generates_data_block_with_single_column) {
    // arrange
    const DataBlock block("temperatures", {"TEMP"}, {{"25.0"}, {"50.0"}, {"75.0"}});
    // act
    const auto directives = block.to_xyce_directives();
    // assert
    ASSERT_EQ(directives.size(), 6);
    ASSERT_EQ(directives[0], ".DATA temperatures");
    ASSERT_EQ(directives[1], "+ TEMP");
    ASSERT_EQ(directives[2], "+ 25.0");
    ASSERT_EQ(directives[3], "+ 50.0");
    ASSERT_EQ(directives[4], "+ 75.0");
    ASSERT_EQ(directives[5], ".ENDDATA");
}

// ========================================================================================
// equality operator
// ========================================================================================

TEST(DataBlockChecks, equality_operator_equal_blocks) {
    // arrange
    const DataBlock block1("myTable", {"r1", "r2"}, {{"1.0", "2.0"}});
    const DataBlock block2("myTable", {"r1", "r2"}, {{"1.0", "2.0"}});
    // act
    const bool result = block1 == block2;
    // assert
    ASSERT_TRUE(result);
}

TEST(DataBlockChecks, equality_operator_different_name) {
    // arrange
    const DataBlock block1("myTable", {"r1"}, {{"1.0"}});
    const DataBlock block2("otherTable", {"r1"}, {{"1.0"}});
    // act
    const bool result = block1 == block2;
    // assert
    ASSERT_FALSE(result);
}

TEST(DataBlockChecks, equality_operator_different_parameters) {
    // arrange
    const DataBlock block1("myTable", {"r1"}, {{"1.0"}});
    const DataBlock block2("myTable", {"r2"}, {{"1.0"}});
    // act
    const bool result = block1 == block2;
    // assert
    ASSERT_FALSE(result);
}

TEST(DataBlockChecks, equality_operator_different_records) {
    // arrange
    const DataBlock block1("myTable", {"r1"}, {{"1.0"}});
    const DataBlock block2("myTable", {"r1"}, {{"2.0"}});
    // act
    const bool result = block1 == block2;
    // assert
    ASSERT_FALSE(result);
}

// ========================================================================================
// Additional tests for round-trip
// ========================================================================================

TEST(DataBlockChecks, round_trip_preserves_structure) {
    // arrange
    const DataBlock original("sweep", {"r1", "c1"}, {{"1k", "10n"}, {"2k", "20n"}});
    // act - generate directives then parse them back
    const auto directives = original.to_xyce_directives();
    const auto parsed = DataBlock::from_xyce_directives(directives);
    // assert
    ASSERT_EQ(parsed.size(), 1);
    ASSERT_EQ(parsed[0].name, original.name);
    ASSERT_EQ(parsed[0].parameters.size(), original.parameters.size());
    ASSERT_EQ(parsed[0].parameters[0], original.parameters[0]);
    ASSERT_EQ(parsed[0].parameters[1], original.parameters[1]);
    ASSERT_EQ(parsed[0].records.size(), original.records.size());
    ASSERT_EQ(parsed[0].records[0].size(), original.records[0].size());
    ASSERT_EQ(parsed[0].records[0][0], original.records[0][0]);
    ASSERT_EQ(parsed[0].records[0][1], original.records[0][1]);
    ASSERT_EQ(parsed[0].records[1].size(), original.records[1].size());
    ASSERT_EQ(parsed[0].records[1][0], original.records[1][0]);
    ASSERT_EQ(parsed[0].records[1][1], original.records[1][1]);
}
