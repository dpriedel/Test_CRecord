// =====================================================================================
//
//       Filename:  Unit_Test.cpp
//
//    Description:  Driver for TDD related tests 
//
//        Version:  1.0
//        Created:  12/30/2022 09:25:34 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//   Organization:  
//
// =====================================================================================

    /* This file is part of ModernCRecord. */

    /* ModernCRecord is free software: you can redistribute it and/or modify */
    /* it under the terms of the GNU General Public License as published by */
    /* the Free Software Foundation, either version 3 of the License, or */
    /* (at your option) any later version. */

    /* ModernCRecord is distributed in the hope that it will be useful, */
    /* but WITHOUT ANY WARRANTY; without even the implied warranty of */
    /* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
    /* GNU General Public License for more details. */

    /* You should have received a copy of the GNU General Public License */
    /* along with Extractor_Markup.  If not, see <http://www.gnu.org/licenses/>. */


#include <fstream>

/* #include <gmock/gmock.h> */
#include <gtest/gtest.h>

#include <date/date.h>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>

#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/algorithm/for_each_n.hpp>
#include <range/v3/view/take.hpp>


using namespace std::literals::chrono_literals;
using namespace date::literals;
using namespace std::string_literals;
namespace fs = std::filesystem;

using namespace testing;

#include "CRecord.h"
#include "CRecordDescParser.h"

// NOLINTBEGIN(*-magic-numbers)
//

// some specific files for Testing.

// some utility code for generating test data

class RecordDescFileParser : public Test
{

};

TEST_F(RecordDescFileParser, VerifyThrowsIfInputFileNotFound)    //NOLINT
{
    // this is just a basic test to get things started.

    CRecordDescParser my_parser;
  
    ASSERT_THROW(my_parser.ParseRecordDescFile("/tmp/xxx"), std::invalid_argument);
};

TEST_F(RecordDescFileParser, VerifyCanCreateFixedRecord)    //NOLINT
{
    CRecordDescParser my_parser;
  
    auto new_record = my_parser.ParseRecordDescFile("./test_files/file1_Record_Desc");
    ASSERT_TRUE(new_record);

    // verify new record type is FixedRecord
    ASSERT_EQ(new_record.value().index(), e_FixedRecord);
    // test accessing a property
    ASSERT_EQ(std::get<e_FixedRecord>(new_record.value()).GetBufferLen(), 159);
};

TEST_F(RecordDescFileParser, VerifyCanCreateVariableRecord)    //NOLINT
{
    CRecordDescParser my_parser;
  
    auto new_record = my_parser.ParseRecordDescFile("./test_files/file3_Record_Desc");
    ASSERT_TRUE(new_record);

    // verify new record type is FixedRecord
    ASSERT_EQ(new_record.value().index(), e_VariableRecord);
    // test accessing a property
    ASSERT_EQ(std::get<e_VariableRecord>(new_record.value()).GetFieldCount(), 47);
};

TEST_F(RecordDescFileParser, VerifyCanAddAllFixedRecordFields)    //NOLINT
{
    CRecordDescParser my_parser;
  
    auto new_record = my_parser.ParseRecordDescFile("./test_files/file1_Record_Desc");
    ASSERT_TRUE(new_record);

    // verify got all fields
    ASSERT_EQ(std::get<e_FixedRecord>(new_record.value()).GetFields().size(), 9);
};

TEST_F(RecordDescFileParser, VerifyCanAddAllVariableRecordFields)    //NOLINT
{
    CRecordDescParser my_parser;
  
    auto new_record = my_parser.ParseRecordDescFile("./test_files/file3_Record_Desc");
    ASSERT_TRUE(new_record);

    // verify got all fields
    // NOTE: test file has 1 SYNTH field so count is 48.
    ASSERT_EQ(std::get<e_VariableRecord>(new_record.value()).GetFields().size(), 48);
};

TEST_F(RecordDescFileParser, VerifyCanMapDataRecord)    //NOLINT
{
    CRecordDescParser my_parser;
  
    auto new_record = my_parser.ParseRecordDescFile("./test_files/file2_Record_Desc");
    ASSERT_TRUE(new_record);

    // verify got all fields
    // ASSERT_EQ(std::get<e_FixedRecord>(new_record.value()).GetFields().size(), 9);

    std::ifstream file_data = std::ifstream("./test_files/file2_data.dat", std::ios::in | std::ios::binary);

    std::string buffer;
    buffer.reserve(5000);

    auto& fixed_record = std::get<e_FixedRecord>(new_record.value());

    int ctr = 0;
    while (file_data.good() /* && ++ctr < 6 */)
    {
        file_data.getline(buffer.data(), buffer.capacity());
        // fmt::print("buffer: {:30}\n", buffer);
        fixed_record.UseData(std::string_view{buffer.data(), buffer.size()});
        // fmt::print("{}\n", fixed_record);
    }
    file_data.close();
}

TEST_F(RecordDescFileParser, VerifyCanCopyAndMoveThings)    //NOLINT
{
    CRecordDescParser my_parser;
  
    auto new_record = my_parser.ParseRecordDescFile("./test_files/file2_Record_Desc");
    ASSERT_TRUE(new_record);

    auto& fixed_record = std::get<e_FixedRecord>(new_record.value());
    auto record2 = fixed_record;

    // EXPECT_TRUE((record2 <=>  fixed_record) == 0);

    // // verify got all fields
    // // ASSERT_EQ(std::get<e_FixedRecord>(new_record.value()).GetFields().size(), 9);
    //
    // std::ifstream file_data = std::ifstream("./test_files/file2_data.dat", std::ios::in | std::ios::binary);
    //
    // std::string buffer;
    // buffer.reserve(5000);
    //
    // auto& fixed_record = std::get<e_FixedRecord>(new_record.value());
    //
    // int ctr = 0;
    // while (file_data.good() && ++ctr < 6)
    // {
    //     file_data.getline(buffer.data(), buffer.capacity());
    //     // fmt::print("buffer: {:30}\n", buffer);
    //     fixed_record.UseData(std::string_view{buffer.data(), buffer.size()});
    //     fmt::print("{}\n", fixed_record);
    // }
    // file_data.close();
};

// class Timer : public Test
// {
//
// };
//
// TEST_F(Timer, TestCountDownTimer)    //NOLINT
// {
//     
// };
//
// NOLINTEND(*-magic-numbers)

//===  FUNCTION  ======================================================================
//        Name:  InitLogging
// Description:  
//=====================================================================================

void InitLogging ()
{
   spdlog::set_level(spdlog::level::debug);
//    spdlog::get()->set_filter
//    (
//        logging::trivial::severity >= logging::trivial::trace
//    );
}		/* -----  end of function InitLogging  ----- */

int main(int argc, char** argv)
{
    InitLogging();


    InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
