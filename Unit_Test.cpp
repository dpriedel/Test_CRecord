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


/* #include <gmock/gmock.h> */
#include <gtest/gtest.h>

#include <date/date.h>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>


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

TEST_F(RecordDescFileParser, VerifyCanReadRecordDescFile)    //NOLINT
{
    // this is just a basic test to get things started.

    CRecordDescParser my_parser;
  
    ASSERT_NO_THROW(my_parser.ParseRecordDescFile("./test_files/file1_Record_Desc"));
};

class Timer : public Test
{

};

TEST_F(Timer, TestCountDownTimer)    //NOLINT
{
    
};

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
