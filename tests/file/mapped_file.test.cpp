#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "file/mapped_file.h"

TEST(MappedFileChecks, default_constructed_is_invalid) {
    // arrange / act
    MappedFile mapped;
    // assert
    EXPECT_FALSE(mapped.is_valid());
    EXPECT_EQ(mapped.size(), 0);
    EXPECT_EQ(mapped.data(), nullptr);
}

TEST(MappedFileChecks, maps_existing_file_content) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "mapped_file_content.bin";
    const std::string content = "hello mapped world";
    {
        std::ofstream out(path, std::ios::binary);
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
    }
    // scope the mapping so the file handle is released before cleanup
    {
        // act
        MappedFile mapped(path);
        // assert
        ASSERT_TRUE(mapped.is_valid());
        ASSERT_EQ(mapped.size(), content.size());
        EXPECT_EQ(std::string(mapped.data(), mapped.size()), content);
    }
    // cleanup
    std::filesystem::remove(path);
}

TEST(MappedFileChecks, missing_file_is_invalid) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "mapped_file_missing.bin";
    std::filesystem::remove(path);
    // act
    MappedFile mapped(path);
    // assert
    EXPECT_FALSE(mapped.is_valid());
    EXPECT_EQ(mapped.size(), 0);
    EXPECT_EQ(mapped.data(), nullptr);
}

TEST(MappedFileChecks, empty_file_is_invalid) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "mapped_file_empty.bin";
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out.close();
    }
    // act
    MappedFile mapped(path);
    // assert
    EXPECT_FALSE(mapped.is_valid());
    // cleanup
    std::filesystem::remove(path);
}

TEST(MappedFileChecks, move_constructor_transfers_mapping) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "mapped_file_move.bin";
    const std::string content = "move me";
    {
        std::ofstream out(path, std::ios::binary);
        out << content;
    }
    MappedFile original(path);
    ASSERT_TRUE(original.is_valid());
    // scope the mapping so the file handle is released before cleanup
    {
        // act
        MappedFile moved(std::move(original));
        // assert
        ASSERT_TRUE(moved.is_valid());
        EXPECT_EQ(std::string(moved.data(), moved.size()), content);
        EXPECT_FALSE(original.is_valid());
        EXPECT_EQ(original.data(), nullptr);
    }
    // cleanup
    std::filesystem::remove(path);
}

TEST(MappedFileChecks, move_assignment_transfers_mapping) {
    // arrange
    const auto path_a = std::filesystem::temp_directory_path() / "mapped_file_assign_a.bin";
    const auto path_b = std::filesystem::temp_directory_path() / "mapped_file_assign_b.bin";
    const std::string content_a = "aaaa";
    const std::string content_b = "bbbb";
    {
        std::ofstream out(path_a, std::ios::binary);
        out << content_a;
    }
    {
        std::ofstream out(path_b, std::ios::binary);
        out << content_b;
    }
    MappedFile a(path_a);
    MappedFile b(path_b);
    ASSERT_TRUE(a.is_valid());
    ASSERT_TRUE(b.is_valid());
    // scope the mapping so the file handle is released before cleanup
    {
        // act
        a = std::move(b);
        // assert
        ASSERT_TRUE(a.is_valid());
        EXPECT_EQ(std::string(a.data(), a.size()), content_b);
        EXPECT_FALSE(b.is_valid());
    }
    // cleanup
    std::filesystem::remove(path_a);
    std::filesystem::remove(path_b);
}

TEST(MappedFileChecks, prefetch_keeps_mapping_valid) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "mapped_file_prefetch.bin";
    const std::string content = "prefetch me";
    {
        std::ofstream out(path, std::ios::binary);
        out << content;
    }
    MappedFile mapped(path);
    ASSERT_TRUE(mapped.is_valid());
    // scope the mapping so the file handle is released before cleanup
    {
        // act
        mapped.prefetch();
        // assert
        EXPECT_TRUE(mapped.is_valid());
        EXPECT_EQ(std::string(mapped.data(), mapped.size()), content);
    }
    // cleanup
    std::filesystem::remove(path);
}

TEST(MappedFileChecks, prefetch_is_safe_on_invalid_mapping) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "mapped_file_prefetch_invalid.bin";
    std::filesystem::remove(path);
    MappedFile mapped(path);
    ASSERT_FALSE(mapped.is_valid());
    // act / assert (must not crash)
    mapped.prefetch();
    EXPECT_FALSE(mapped.is_valid());
}
