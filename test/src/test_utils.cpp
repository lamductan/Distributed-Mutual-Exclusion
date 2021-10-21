#include <iostream>
#include <string>
#include <gtest/gtest.h>

#include "distributed-me/utils.h"

TEST(TestUtils, TestIntToChar) {
    int x = 123456789;
    char correct_char[4] = {21, -51, 91, 7};

    char t[4];
    intToChar(x, t);
    EXPECT_TRUE(memcmp(t, correct_char, 4) == 0);
}

TEST(TestUtils, TestIntToChar1) {
    int x = 2;
    char correct_char[4] = {2, 0, 0, 0};

    char t[4];
    intToChar(x, t);
    EXPECT_TRUE(memcmp(t, correct_char, 4) == 0);
}

TEST(TestUtils, TestIntToChar2) {
    int x = 256;
    char correct_char[4] = {0, 1, 0, 0};

    char t[4];
    intToChar(x, t);
    EXPECT_TRUE(memcmp(t, correct_char, 4) == 0);
}

TEST(TestUtils, TestCharToInt) {
    char input[4] = {21, -51, 91, 7};
    int correct_x = 123456789;

    int x = charToInt(input);
    EXPECT_EQ(x, correct_x);
}

TEST(TestUtils, TestWriteLastLine) {
    std::string filepath = "../test_data/servers/server_3/test.txt";
    std::string content_to_write = "lamductan 123";
    _writeToFile(filepath, content_to_write);
    EXPECT_EQ(content_to_write, _readLastLineOfFile(filepath));
}

TEST(TestUtils, TestReadLastLineFile1) {
    std::string filepath = "../test_data_init/servers/server_0/f1.txt";
    std::string correct_last_line = "This is f1 file.";
    EXPECT_EQ(correct_last_line, _readLastLineOfFile(filepath));
}

TEST(TestUtils, TestGenerateRandomInputFile) {
    const char* input_filename = "../test_data/test_input.txt";
    int n_servers = 3;
    int n_requests = 10;
    std::vector<std::string> list_files = getListFiles(
        "../test_data/servers/server_0");
    _generateRandomInputFile(
        list_files, input_filename, n_requests, n_servers);
}
