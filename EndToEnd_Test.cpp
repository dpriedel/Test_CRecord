
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

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <ranges>
#include <vector>

namespace rng = std::ranges;
namespace vws = std::ranges::views;

#include <date/chrono_io.h>

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

std::chrono::year_month_day StringToDateYMD(std::string_view input_format, std::string_view the_date)
{
    std::istringstream in{the_date.data()};
    date::year_month_day result{};
    date::from_stream(in, input_format.data(), result);
    BOOST_ASSERT_MSG(! in.fail() && ! in.bad(), std::format("Unable to parse given date: {}", the_date).c_str());
    std::chrono::year_month_day result1(std::chrono::year{result.year().operator int()}, std::chrono::month{result.month().operator unsigned()}, std::chrono::day{result.day().operator unsigned()});
    return result1;
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
        spdlog::error(std::format("Something fundamental went wrong: {}", theProblem.what()));
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
        std::array<char, 10> symbol_ = {};
        std::chrono::year_month_day date_ = {};
        float_t open_ = 0.0;
        float_t high_ = 0.0;
        float_t low_ = 0.0;
        float_t close_ = 0.0;
        int32_t volume_ = 0;
    };

    // std::print("empty StockData: {}\n", StockData{});

	try
    {
        CRecordDescParser my_parser;
    
        // use some simple daily stock data...
        // TODO: need a file with multiple symbols for multiple days !

        auto new_record = my_parser.ParseRecordDescFile("./test_files/file4_Record_Desc");
        ASSERT_TRUE(new_record);

        auto& stock_data_record = std::get<e_VariableRecord>(new_record.value());

        /* verify got all fields */

        EXPECT_EQ(stock_data_record.GetFields().size(), 9);

        std::string buffer{};
        buffer.reserve(500);

        std::ifstream file_data = std::ifstream("./test_files/file5_data.dat", std::ios::in | std::ios::binary);

        // let's read our header record and pick up field names. 

        std::getline(file_data, buffer);
        stock_data_record.UseData(std::string_view{buffer.data(), buffer.size()});

        std::vector<StockData> stock_data_history;
        
        while (std::getline(file_data, buffer))
        {
            stock_data_record.UseData(std::string_view{buffer.data(), buffer.size()});
            StockData new_data;
            new_data.symbol_.fill('\0');
            stock_data_record["Code"].copy(new_data.symbol_.data(), new_data.symbol_.size() - 1);
            new_data.date_ = StringToDateYMD("%Y-%m-%d", stock_data_record["Date"]);
            new_data.open_ = stock_data_record.ConvertFieldToNumber<float_t>("Open");
            new_data.high_ = stock_data_record.ConvertFieldToNumber<float_t>("High");
            new_data.low_ = stock_data_record.ConvertFieldToNumber<float_t>("Low");
            new_data.close_ = stock_data_record.ConvertFieldToNumber<float_t>("Close");
            new_data.volume_ = stock_data_record.ConvertFieldToNumber<int32_t>("Volume");

            stock_data_history.push_back(new_data);
            // std::print("{}\n", stock_data_record);
        }
        file_data.close();

        // I peeked so I know this number.
        EXPECT_EQ(stock_data_history.size(), 5315);

        // now, we'll calculate the on-balance volue for the data we collected above.
        // use the formula from https://www.investopedia.com/terms/o/onbalancevolume.asp

        struct Obv_Data
        {
            std::array<char, 10> symbol_ = {};
            std::chrono::year_month_day date_ = {};
            int64_t obv_ = 0;
        };
        
        rng::sort(stock_data_history, [](const auto& a, const auto& b)
          {
              if (a.symbol_ < b.symbol_)
              {
                  return true;
              } 
              if (a.symbol_ == b.symbol_)
              {
                  return a.date_ < b.date_;
              }
              return false;
          });

        std::vector<Obv_Data> obv_history;
        obv_history.reserve(stock_data_history.size());

        auto each_symbol = vws::chunk_by([](const auto&a, const auto& b) { return a.symbol_ == b.symbol_; });
        for (const auto& stock_days : stock_data_history | each_symbol)
        {
            int64_t prev_obv = 0;
            int64_t obv = 0;
            for (const auto vals: stock_days | vws::slide(2))
            {
                if (vals[1].close_ > vals[0].close_)
                {
                    obv = prev_obv + vals[1].volume_;
                }
                else if (vals[1].close_ < vals[0].close_)
                {
                    obv = prev_obv - vals[1].volume_;
                }
                else 
                {
                    obv = prev_obv;
                }
                Obv_Data new_data{.symbol_=stock_days[0].symbol_, .date_=vals[1].date_, .obv_=obv};
                obv_history.push_back(new_data);

                prev_obv = obv;
            }
        }

        rng::for_each(obv_history | vws::take(50), [](const Obv_Data& val)
                         { std::cout << "symbol: " << val.symbol_.data() << ". date: " << val.date_ << ".  obv: " << val.obv_ << '\n'; });
    }

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(std::format("Something fundamental went wrong: {}", theProblem.what()));
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
