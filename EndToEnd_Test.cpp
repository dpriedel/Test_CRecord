
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

#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

// #include <future>
//
// #include <range/v3/algorithm/for_each.hpp>
//
// #include <range/v3/view/drop.hpp>
// #include <range/v3/algorithm/equal.hpp>
// #include <range/v3/algorithm/find_if.hpp>

#include <date/date.h>

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <gmock/gmock.h>

#include "utilities.h"
#include "CRecord.h"
#include "CRecordDescParser.h"

namespace fs = std::filesystem;

const fs::path APPL_EOD_CSV{"./test_files/AAPL_close.dat"};
const fs::path SPY_EOD_CSV{"./test_files/SPY.csv"};

using namespace testing;

std::shared_ptr<spdlog::logger> DEFAULT_LOGGER;

// ===  FUNCTION  ======================================================================
//         Name:  StringToDateYMD
//  Description:  
// =====================================================================================

date::year_month_day StringToDateYMD(std::string_view input_format, std::string_view the_date)
{
    std::istringstream in{the_date.data()};
    date::year_month_day result{};
    date::from_stream(in, input_format.data(), result);
    BOOST_ASSERT_MSG(! in.fail() && ! in.bad(), fmt::format("Unable to parse given date: {}", the_date).c_str());
    return result;
}		// -----  end of method StringToDateYMD  ----- 

class TestInstantiateCRecord : public Test
{
};

TEST_F(TestInstantiateCRecord, VerifyCompileLinkRunUsinglibCRecord)    //NOLINT
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
}

TEST_F(TestInstantiateCRecord, UseCRecordToActuallyDoSomething)    //NOLINT
{
    struct StockData
    {
        std::array<char, 10> symbol = {};
        date::year_month_day date = {};
        float_t open = 0.0;
        float_t high = 0.0;
        float_t low = 0.0;
        float_t close = 0.0;
        int32_t volume = 0;

        const size_t symbol_len = sizeof(symbol);
    };

    // fmt::print("empty StockData: {}\n", StockData{});

	try
    {
        CRecordDescParser my_parser;
    
        auto new_record = my_parser.ParseRecordDescFile("./test_files/file4_Record_Desc");
        ASSERT_TRUE(new_record);

        /* verify got all fields */

        auto& stock_data_record = std::get<e_VariableRecord>(new_record.value());

        EXPECT_EQ(stock_data_record.GetFields().size(), 9);

        std::string buffer{};
        buffer.reserve(500);

        std::ifstream file_data = std::ifstream("./test_files/file4_data.dat", std::ios::in | std::ios::binary);

        // let's read our header record and pick up field names. 

        std::getline(file_data, buffer);
        stock_data_record.UseData(std::string_view{buffer.data(), buffer.size()});

        std::vector<StockData> stock_data_history;
        
        auto to_number = [](std::string_view in_value, auto& out_value) {
            auto [ptr, ec] { std::from_chars(in_value.begin(), in_value.end(), out_value) };
            if (ec == std::errc())
            {
                // nothing to do
            }
            else if (ec == std::errc::invalid_argument)
            {
                std::cout << "That isn't a number.\n";
            }
            else if (ec == std::errc::result_out_of_range)
            {
                std::cout << "This number is larger than an int.\n";
            }
        };
        while (std::getline(file_data, buffer))
        {
            stock_data_record.UseData(std::string_view{buffer.data(), buffer.size()});
            StockData new_data;
            new_data.symbol.fill('\0');
            stock_data_record["Code"].copy(new_data.symbol.data(), new_data.symbol.size() - 1);
            new_data.date = StringToDateYMD("%Y-%m-%d", stock_data_record["Date"]);
            to_number(stock_data_record["Open"], new_data.open);
            to_number(stock_data_record["High"], new_data.high);
            to_number(stock_data_record["Low"], new_data.low);
            to_number(stock_data_record["Close"], new_data.close);
            to_number(stock_data_record["Volume"], new_data.volume);

            stock_data_history.push_back(new_data);
            // fmt::print("{}\n", stock_data_record);
        }
        file_data.close();

        // I peeked so I know this number.

        EXPECT_EQ(stock_data_history.size(), 5315);
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
