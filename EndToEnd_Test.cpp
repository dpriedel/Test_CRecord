
// =====================================================================================
//
//       Filename:  EndToEndTest.cpp
//
//    Description:  Driver program for end-to-end tests
//
//        Version:  1.0
//        Created:  2023-02-10 14:48
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:
//
// =====================================================================================



// =====================================================================================
//        Class:
//  Description:
// =====================================================================================


#include <filesystem>
#include <fstream>
// #include <future>
//
// #include <range/v3/algorithm/for_each.hpp>
//
// #include <range/v3/view/drop.hpp>
// #include <range/v3/algorithm/equal.hpp>
// #include <range/v3/algorithm/find_if.hpp>

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <gmock/gmock.h>

#include "CRecord.h"
#include "CRecordDescParser.h"

namespace fs = std::filesystem;

const fs::path APPL_EOD_CSV{"./test_files/AAPL_close.dat"};
const fs::path SPY_EOD_CSV{"./test_files/SPY.csv"};

using namespace testing;

std::shared_ptr<spdlog::logger> DEFAULT_LOGGER;

class TestInstantiateCRecord : public Test
{
};

TEST_F(TestInstantiateCRecord, UseLibraryVersionForFixedRecord)    //NOLINT
{
	try
    {
        CRecordDescParser my_parser;

        auto new_record = my_parser.ParseRecordDescFile("./test_files/file1_Record_Desc");
        ASSERT_TRUE(new_record);

        // verify new record type is FixedRecord
        ASSERT_EQ(new_record.value().index(), e_FixedRecord);
        // test accessing a property
        ASSERT_EQ(std::get<e_FixedRecord>(new_record.value()).GetBufferLen(), 159);
    }

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(fmt::format("Something fundamental went wrong: {}", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
   EXPECT_TRUE(fs::exists("/tmp/test_charts/SPY_10X3_linear_eod.json"));
   ASSERT_TRUE(fs::exists("/tmp/test_charts/SPY_10X1_linear_eod.json"));
}

void InitLogging ()
{
    DEFAULT_LOGGER = spdlog::default_logger();

    //    nothing to do for now.
//    logging::core::get()->set_filter
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
