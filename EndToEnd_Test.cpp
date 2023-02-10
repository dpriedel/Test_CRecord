
// =====================================================================================
//
//       Filename:  EndToEndTest.cpp
//
//    Description:  Driver program for end-to-end tests
//
//        Version:  1.0
//        Created:  2021-11-20 09:07 AM 
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

#include "PF_CollectDataApp.h"
#include "utilities.h"

#include <filesystem>
#include <fstream>
#include <future>

#include <range/v3/algorithm/for_each.hpp>

#include <range/v3/view/drop.hpp>
#include <range/v3/algorithm/equal.hpp>
#include <range/v3/algorithm/find_if.hpp>

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <pqxx/pqxx>
#include <pqxx/transaction.hxx>

#include <gmock/gmock.h>

#include <pybind11/embed.h> // everything needed for embedding
namespace py = pybind11;
using namespace py::literals;

using namespace std::literals::chrono_literals;
using namespace date::literals;
using namespace std::string_literals;

namespace fs = std::filesystem;

const fs::path APPL_EOD_CSV{"./test_files/AAPL_close.dat"};
const fs::path SPY_EOD_CSV{"./test_files/SPY.csv"};

using namespace testing;

std::shared_ptr<spdlog::logger> DEFAULT_LOGGER;

std::optional<int> FindColumnIndex (std::string_view header, std::string_view column_name, char delim)
{
    auto fields = rng_split_string<std::string_view>(header, delim);
    auto do_compare([&column_name](const auto& field_name)
    {
        // need case insensitive compare
        // found this on StackOverflow (but modified for my use)
        // (https://stackoverflow.com/questions/11635/case-insensitive-string-comparison-in-c)

        if (column_name.size() != field_name.size())
        {
            return false;
        }
        return ranges::equal(column_name, field_name, [](unsigned char a, unsigned char b) { return tolower(a) == tolower(b); });
    });

    if (auto found_it = ranges::find_if(fields, do_compare); found_it != ranges::end(fields))
    {
        return ranges::distance(ranges::begin(fields), found_it);
    }
    return {};

}		// -----  end of method PF_CollectDataApp::FindColumnIndex  ----- 

class ProgramOptions : public Test
{
};

TEST_F(ProgramOptions, TestMixAndMatchOptions)    //NOLINT
{
    if (fs::exists("/tmp/test_charts/SPY_10X3_linear_eod.json"))
    {
        fs::remove("/tmp/test_charts/SPY_10X3_linear_eod.json");
    }
    if (fs::exists("/tmp/test_charts/SPY_10X1_linear_eod.json"))
    {
        fs::remove("/tmp/test_charts/SPY_10X1_linear_eod.json");
    }

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "-s", "SpY",
        "-s", "aapL",
        "--symbol", "IWr",
        "--new-data-source", "file",
        "--new-data-dir", "./test_files",
        "--source-format", "csv",
        "--mode", "load",
        "--interval", "eod",
        "--scale", "linear",
        "--price-fld-name", "Close",
        "--destination", "file",
        "--output-chart-dir", "/tmp/test_charts",
        "--boxsize", "10",
        "--boxsize", "1",
        "--reversal", "3",
        "--reversal", "1"
	};

	try
	{
        PF_CollectDataApp myApp(tokens);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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

TEST_F(ProgramOptions, TestProblemOptions)    //NOLINT
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "-s", "qqqq",
        "-s", "spy",
        "--new-data-source", "streaming",
        "--new-data-dir", "./test_files",
        "--source-format", "csv",
        "--mode", "load",
        "--interval", "live",
        "--scale", "linear",
        "--price-fld-name", "close",
        "--destination", "file",
        "--output-chart-dir", "/tmp/test_charts",
        "--use-ATR",
        "--boxsize", ".1",
        "--boxsize", ".01",
        "--reversal", "1",
        "--reversal", "3"
	};

	try
	{
        PF_CollectDataApp myApp(tokens);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            ASSERT_THROW(myApp.Run(), std::invalid_argument);
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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

class SingleFileEndToEnd : public Test
{
};


TEST_F(SingleFileEndToEnd, VerifyCanLoadCSVDataAndSaveToChartFile)    //NOLINT
{
    if (fs::exists("/tmp/test_charts/SPY_10X3_linear_eod.json"))
    {
        fs::remove("/tmp/test_charts/SPY_10X3_linear_eod.json");
    }
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--symbol", "SPY",
        "--new-data-source", "file",
        "--new-data-dir", "./test_files3",
        "--source-format", "csv",
        "--mode", "load",
        "--interval", "eod",
        "--scale", "linear",
        "--price-fld-name", "Close",
        "--destination", "file",
        "--output-chart-dir", "/tmp/test_charts",
        "--boxsize", "10",
        "--reversal", "3"
	};

	try
	{
        PF_CollectDataApp myApp(tokens);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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
    ASSERT_TRUE(fs::exists("/tmp/test_charts/SPY_10X3_linear_eod.json"));
}

TEST_F(SingleFileEndToEnd, VerifyCanConstructChartFileFromPieces)    //NOLINT
{
    if (fs::exists("/tmp/test_charts/SPY_10X3_linear_eod.json"))
    {
        fs::remove("/tmp/test_charts/SPY_10X3_linear_eod.json");
    }
    if (fs::exists("/tmp/test_charts2/SPY_10X3_linear_eod.json"))
    {
        fs::remove("/tmp/test_charts2/SPY_10X3_linear_eod.json");
    }
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

    // the first test run constructs the data file all at once 

    PF_Chart whole_chart;

	std::vector<std::string> tokens{"the_program",
        "--symbol", "SPY",
        "--new-data-source", "file",
        "--new-data-dir", "./test_files",
        "--source-format", "csv",
        "--mode", "load",
        "--interval", "eod",
        "--scale", "linear",
        "--price-fld-name", "Close",
        "--destination", "file",
        "--output-chart-dir", "/tmp/test_charts",
        "--boxsize", "10",
        "--reversal", "3"
	};

	try
	{
        PF_CollectDataApp myApp(tokens);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            whole_chart = myApp.GetCharts()[0].second;
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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

    // now construct the data file from 2 input files which, together, contain the same 
    // data as the 1 file used above. 

    PF_Chart half_chart;

	std::vector<std::string> tokens2{"the_program",
        "--symbol", "SPY",
        "--new-data-source", "file",
        "--new-data-dir", "./test_files2",
        "--source-format", "csv",
        "--mode", "load",
        "--interval", "eod",
        "--scale", "linear",
        "--price-fld-name", "Close",
        "--destination", "file",
        "--output-chart-dir", "/tmp/test_charts2",
        "--boxsize", "10",
        "--reversal", "3"
	};

	try
	{
        PF_CollectDataApp myApp(tokens2);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            half_chart = myApp.GetCharts()[0].second;
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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
    EXPECT_TRUE(fs::exists("/tmp/test_charts2/SPY_10X3_linear_eod.json"));

    // now continue constructing the data file from 2 input files which, together, contain the same 
    // data as the 1 file used above. 

    PF_Chart franken_chart;

	std::vector<std::string> tokens3{"the_program",
        "--symbol", "SPY",
        "--new-data-source", "file",
        "--new-data-dir", "./test_files3",
        "--chart-data-dir", "/tmp/test_charts2",
        "--source-format", "csv",
        "--mode", "update",
        "--interval", "eod",
        "--scale", "linear",
        "--price-fld-name", "Close",
        "--destination", "file",
        "--output-chart-dir", "/tmp/test_charts2",
        "--boxsize", "10",
        "--reversal", "3"
	};

	try
	{
        PF_CollectDataApp myApp(tokens3);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            franken_chart = myApp.GetCharts()[0].second;
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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

    std::cout << "\n\n whole chart:\n" << whole_chart;
    std::cout << "\n\n half chart:\n" << half_chart;
    std::cout << "\n\n franken chart:\n" << franken_chart << '\n';;

    EXPECT_TRUE(fs::exists("/tmp/test_charts2/SPY_10X3_linear_eod.json"));
    ASSERT_TRUE(whole_chart == franken_chart);
}

class LoadAndUpdate : public Test
{
};

TEST_F(LoadAndUpdate, VerifyUpdateWorksWhenNoPreviousChartData)    //NOLINT
{
    if (fs::exists("/tmp/test_charts_updates"))
    {
        fs::remove_all("/tmp/test_charts_updates");
    }
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "-s", "ADIV", "-s", "ADRU", "-s", "AIA", "-s", "AWF", "-s", "CBRL",
        "--new-data-source", "file",
        "--new-data-dir", "./test_files_update_EOD",
        "--source-format", "csv",
        "--mode", "update",
        "--interval", "eod",
        "--scale", "linear",
        "--price-fld-name", "adjClose",
        "--destination", "file",
        "--chart-data-dir", "./test_files_update_charts",
        "--output-chart-dir", "/tmp/test_charts_updates",
        "--use-ATR",
        "--boxsize", ".1",
        "--reversal", "3", "--reversal", "1"
	};

	try
	{
        PF_CollectDataApp myApp(tokens);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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

    EXPECT_TRUE(fs::exists("/tmp/test_charts_updates/ADIV_0.1X3_linear_eod.json"));
    ASSERT_TRUE(fs::exists("/tmp/test_charts_updates/CBRL_0.1X3_linear_eod.svg"));
}

class Database : public Test
{
	public:

        void SetUp() override
        {
		    pqxx::connection c{"dbname=finance user=data_updater_pg"};
		    pqxx::work trxn{c};

		    // make sure the DB is empty before we start

		    trxn.exec("DELETE FROM test_point_and_figure.pf_charts");
		    trxn.commit();
        }

	int CountRows()
	{
	    pqxx::connection c{"dbname=finance user=data_updater_pg"};
	    pqxx::work trxn{c};

	    // make sure the DB is empty before we start

	    auto row = trxn.exec1("SELECT count(*) FROM test_point_and_figure.pf_charts");
	    trxn.commit();
		return row[0].as<int>();
	}

};

TEST_F(Database, LoadDataFromDB)    //NOLINT
{
    if (fs::exists("/tmp/test_charts"))
    {
        fs::remove_all("/tmp/test_charts");
    }

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--symbol", "SPY",      // want to use SP500 indicator but need to do more setup first
        "--symbol", "AAPL",
        "--symbol-list", "IWR,iwm,t",
        "--new-data-source", "database",
        "--mode", "load",
        "--scale", "linear",
        "--price-fld-name", "adjclose",
        "--destination", "file",
        "--output-chart-dir", "/tmp/test_charts",
        "--boxsize", ".1",
        "--boxsize", "1",
        "--reversal", "1",
        "-r", "3",
        "--db-user", "data_updater_pg",
        "--db-name", "finance",
        "--db-data-source", "new_stock_data.current_data",
        "--begin-date", "2017-01-01",
        "--use-ATR",
        "--max-graphic-cols", "150"
	};

	try
	{
        PF_CollectDataApp myApp(tokens);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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
    EXPECT_TRUE(fs::exists("/tmp/test_charts/SPY_1X3_linear_eod.json"));
    ASSERT_TRUE(fs::exists("/tmp/test_charts/SPY_0.1X1_linear_eod.json"));
}

TEST_F(Database, DISABLED_BulkLoadDataFromDB)    //NOLINT
{
    if (fs::exists("/tmp/test_charts3"))
    {
        fs::remove_all("/tmp/test_charts3");
    }

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--symbol-list", "ALL",
        // "-s", "ACY",
        "--new-data-source", "database",
        "--mode", "load",
        "--scale", "percent",
        "--price-fld-name", "adjclose",
        "--destination", "file",
        "--output-chart-dir", "/tmp/test_charts3",
        "--graphics-format", "svg",
        // "--boxsize", ".1",
        "--boxsize", ".01",
        "--boxsize", ".001",
        "--reversal", "1",
        "-r", "3",
        "--db-user", "data_updater_pg",
        "--db-name", "finance",
        "--db-data-source", "new_stock_data.current_data",
        "--begin-date", "2022-06-01",
        // "--use-ATR",
        "--exchange", "NYSE",
        "--max-graphic-cols", "150"
	};

	try
	{
        PF_CollectDataApp myApp(tokens);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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
    EXPECT_TRUE(fs::exists("/tmp/test_charts3/SPY_0.01%X1_percent_eod.csv"));
    ASSERT_TRUE(fs::exists("/tmp/test_charts3/SPY_0.001%X1_percent_eod.json"));
}

TEST_F(Database, UpdateUsingDataFromDB)    //NOLINT
{
    if (fs::exists("/tmp/test_charts2"))
    {
        fs::remove_all("/tmp/test_charts2");
    }

    fs::create_directories("/tmp/test_charts2");

    // construct a chart using some test data and save it.
    fs::path csv_file_name{SPY_EOD_CSV};
    const std::string file_content_csv = LoadDataFileForUse(csv_file_name);

    const auto symbol_data_records = split_string<std::string_view>(file_content_csv, '\n');
    const auto header_record = symbol_data_records.front();

    auto date_column = FindColumnIndex(header_record, "date", ',');
    BOOST_ASSERT_MSG(date_column.has_value(), fmt::format("Can't find 'date' field in header record: {}.", header_record).c_str());
    
    auto close_column = FindColumnIndex(header_record, "Close", ',');
    BOOST_ASSERT_MSG(close_column.has_value(), fmt::format("Can't find price field: 'Close' in header record: {}.", header_record).c_str());

    PF_Chart new_chart{"SPY", 10, 1};

    ranges::for_each(symbol_data_records | ranges::views::drop(1), [&new_chart, close_col = close_column.value(), date_col = date_column.value()](const auto record)
        {
            const auto fields = split_string<std::string_view> (record, ',');
            new_chart.AddValue(DprDecimal::DDecQuad(fields[close_col]), StringToUTCTimePoint("%Y-%m-%d", fields[date_col]));
        });
    std::cout << "new chart at after loading initial data: \n\n" << new_chart << "\n\n";

    fs::path chart_file_path = fs::path{"/tmp/test_charts2"} / (new_chart.MakeChartFileName("eod", "json"));
    std::ofstream new_file{chart_file_path, std::ios::out | std::ios::binary};
    BOOST_ASSERT_MSG(new_file.is_open(), fmt::format("Unable to open file: {} to write updated data.", chart_file_path).c_str());
    new_chart.ConvertChartToJsonAndWriteToStream(new_file);
    new_file.close();


	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--symbol", "SPY",      // want to use SP500 indicator but need to do more setup first
        "--symbol", "AAPL",      // want to use SP500 indicator but need to do more setup first
        "--symbol", "GOOG",      // want to use SP500 indicator but need to do more setup first
        "--new-data-source", "database",
        "--mode", "update",
        "--scale", "linear",
        "--price-fld-name", "adjclose",
        "--destination", "file",
        "--chart-data-source", "file",
        "--chart-data-dir", "/tmp/test_charts2",
        "--output-chart-dir", "/tmp/test_charts2",
        "--boxsize", "10",
        "--reversal", "1",
        "--db-user", "data_updater_pg",
        "--db-name", "finance",
        "--db-data-source", "new_stock_data.current_data",
        "--begin-date", "2021-11-24",
        "--max-graphic-cols", "150"
	};

    PF_Chart updated_chart;

	try
	{
        PF_CollectDataApp myApp(tokens);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            updated_chart = myApp.GetCharts()[0].second;
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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

    std::cout << "updated chart at after loading initial data: \n\n" << updated_chart << "\n\n";

    EXPECT_TRUE(fs::exists("/tmp/test_charts2/SPY_10X1_linear_eod.json"));
    ASSERT_NE(new_chart, updated_chart);
}

TEST_F(Database, UpdateDatainDBUsingNewDataFromDB)    //NOLINT
{
    if (fs::exists("/tmp/test_charts9"))
    {
        fs::remove_all("/tmp/test_charts9");
    }
    // construct a chart using some test data and save it.
    fs::path csv_file_name{SPY_EOD_CSV};
    const std::string file_content_csv = LoadDataFileForUse(csv_file_name);

    const auto symbol_data_records = split_string<std::string_view>(file_content_csv, '\n');
    const auto header_record = symbol_data_records.front();

    auto date_column = FindColumnIndex(header_record, "date", ',');
    BOOST_ASSERT_MSG(date_column.has_value(), fmt::format("Can't find 'date' field in header record: {}.", header_record).c_str());
    
    auto close_column = FindColumnIndex(header_record, "Close", ',');
    BOOST_ASSERT_MSG(close_column.has_value(), fmt::format("Can't find price field: 'Close' in header record: {}.", header_record).c_str());

    PF_Chart new_chart{"SPY", 10, 1};

    ranges::for_each(symbol_data_records | ranges::views::drop(1), [&new_chart, close_col = close_column.value(), date_col = date_column.value()](const auto record)
        {
            const auto fields = split_string<std::string_view> (record, ',');
            new_chart.AddValue(DprDecimal::DDecQuad(fields[close_col]), StringToUTCTimePoint("%Y-%m-%d", fields[date_col]));
        });
    std::cout << "new chart at after loading initial data: \n\n" << new_chart << "\n\n";

    PF_DB::DB_Params db_info{.user_name_="data_updater_pg", .db_name_="finance", .db_mode_="test"};
    PF_DB pf_db(db_info);

    new_chart.StoreChartInChartsDB(pf_db, "eod");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--symbol", "SPY",      // want to use SP500 indicator but need to do more setup first
        "--new-data-source", "database",
        "--mode", "update",
        "--scale", "linear",
        "--price-fld-name", "adjclose",
        // "--destination", "database",
        "--destination", "file",
        "--chart-data-source", "database",
        "--output-chart-dir", "/tmp/test_charts9",
        "--output-graph-dir", "/tmp/test_charts9",
        "--graphics-format", "svg",
        // "--boxsize", "10",
        "--boxsize", "5",
        // "--reversal", "1",
        "--reversal", "3",
        "--db-user", "data_updater_pg",
        "--db-name", "finance",
        "--db-data-source", "new_stock_data.current_data",
        "--begin-date", "2021-11-24",
        "--max-graphic-cols", "150",
        "-l", "debug"
	};

    PF_Chart updated_chart;

	try
	{
        PF_CollectDataApp myApp(tokens);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            updated_chart = myApp.GetCharts()[0].second;
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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

    std::cout << "updated chart at after loading initial data: \n\n" << updated_chart << "\n\n";

    EXPECT_EQ(CountRows(), 4);
    ASSERT_NE(new_chart, updated_chart);
}

TEST_F(Database, BulkLoadDataFromDBAndStoreChartsInDB)    //NOLINT
{
    if (fs::exists("/tmp/test_charts3"))
    {
        fs::remove_all("/tmp/test_charts3");
    }

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--symbol-list", "ALL",
        // "-s", "ACY",
        "--new-data-source", "database",
        "--mode", "load",
        "--scale", "percent",
        "--price-fld-name", "adjclose",
        "--destination", "database",
        "--output-graph-dir", "/tmp/test_charts3",
        "--graphics-format", "csv",
        "--boxsize", ".1",
        "--boxsize", ".01",
        // "--boxsize", ".001",
        "--reversal", "1",
        "-r", "3",
        "--db-user", "data_updater_pg",
        "--db-name", "finance",
        "--db-data-source", "new_stock_data.current_data",
        "--begin-date", "2022-01-01",
        "--use-ATR",
        "--exchange", "NYSE",
        "--max-graphic-cols", "150"
	};

	try
	{
        PF_CollectDataApp myApp(tokens);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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
    EXPECT_TRUE(fs::exists("/tmp/test_charts3/SPY_0.01%X1_percent_eod.csv"));
    ASSERT_TRUE(fs::exists("/tmp/test_charts3/SPY_0.001%X1_percent_eod.json"));
}

TEST_F(Database, DailyScan)    //NOLINT
{
    // construct a chart using some test data and save it.
    fs::path csv_file_name{SPY_EOD_CSV};
    const std::string file_content_csv = LoadDataFileForUse(csv_file_name);

    const auto symbol_data_records = split_string<std::string_view>(file_content_csv, '\n');
    const auto header_record = symbol_data_records.front();

    auto date_column = FindColumnIndex(header_record, "date", ',');
    BOOST_ASSERT_MSG(date_column.has_value(), fmt::format("Can't find 'date' field in header record: {}.", header_record).c_str());
    
    auto close_column = FindColumnIndex(header_record, "Close", ',');
    BOOST_ASSERT_MSG(close_column.has_value(), fmt::format("Can't find price field: 'Close' in header record: {}.", header_record).c_str());

    PF_Chart new_chart{"SPY", 10, 1};

    ranges::for_each(symbol_data_records | ranges::views::drop(1), [&new_chart, close_col = close_column.value(), date_col = date_column.value()](const auto record)
        {
            const auto fields = split_string<std::string_view> (record, ',');
            new_chart.AddValue(DprDecimal::DDecQuad(fields[close_col]), StringToUTCTimePoint("%Y-%m-%d", fields[date_col]));
        });
    std::cout << "new chart at after loading initial data: \n\n" << new_chart << "\n\n";

    PF_DB::DB_Params db_info{.user_name_="data_updater_pg", .db_name_="finance", .db_mode_="test"};
    PF_DB pf_db(db_info);

    new_chart.StoreChartInChartsDB(pf_db, "eod");

    // save for comparison later 
    
    PF_Chart saved_chart{new_chart};

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "daily-scan",
        "--price-fld-name", "adjclose",
        "--db-user", "data_updater_pg",
        "--db-name", "finance",
        "--db-data-source", "new_stock_data.current_data",
        "--begin-date", "2021-11-24",
        "-l", "debug"
	};

	try
	{
        PF_CollectDataApp myApp(tokens);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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

    EXPECT_EQ(CountRows(), 1);

    // let's see what is in the DB

    auto updated_chart = PF_Chart::MakeChartFromDB(pf_db, saved_chart.GetChartParams(), "eod");
    std::cout << "updated chart at after after running daily scan: \n\n" << updated_chart << "\n\n";

    ASSERT_NE(saved_chart, updated_chart);
}

class StreamData : public Test
{
};

TEST_F(StreamData, VerifyConnectAndDisconnect)    //NOLINT
{
    if (fs::exists("/tmp/test_charts"))
    {
        fs::remove_all("/tmp/test_charts");
    }

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--symbol", "SPY",
        "--symbol", "AAPL",
        "--new-data-source", "streaming",
        "--mode", "load",
        "--interval", "live",
        "--scale", "linear",
        "--price-fld-name", "close",
        "--destination", "file",
        "--output-chart-dir", "/tmp/test_charts",
        "--boxsize", "0.1",
        "--boxsize", "0.05",
        "--reversal", "1"
	};

	try
	{
        PF_CollectDataApp myApp(tokens);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        auto now = date::zoned_seconds(date::current_zone(), floor<std::chrono::seconds>(std::chrono::system_clock::now()));
        auto then = date::zoned_seconds(date::current_zone(), floor<std::chrono::seconds>(std::chrono::system_clock::now()) + 15s);

        int counter = 0;
        auto timer = [&counter] (const auto& stop_at)
            { 
                while (true)
                {
                    std::cout << "ding...\n";
                    ++counter;
                    auto now = date::zoned_seconds(date::current_zone(), floor<std::chrono::seconds>(std::chrono::system_clock::now()));
                    if (now.get_sys_time() >= stop_at.get_sys_time())
                    {
                        PF_CollectDataApp::SetSignal();
                        break;
                    }
                    std::this_thread::sleep_for(1s);
                }
            };

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            // add an external timer here.
            auto timer_task = std::async(std::launch::async, timer, then);

            myApp.Run();
            myApp.Shutdown();

            timer_task.get();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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
    ASSERT_TRUE(fs::exists("/tmp/test_charts/SPY_0.05X1_linear.json"));
}

TEST_F(StreamData, VerifySignalHandling)    //NOLINT
{
    if (fs::exists("/tmp/test_charts"))
    {
        fs::remove_all("/tmp/test_charts");
    }
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--symbol-list", "SPY,aapl",
        "--new-data-source", "streaming",
        "--mode", "load",
        "--interval", "live",
        "--scale", "linear",
        "--price-fld-name", "close",
        "--destination", "file",
        "--output-chart-dir", "/tmp/test_charts",
        "--boxsize", "0.05",
        "--reversal", "1"
	};

	try
	{
        PF_CollectDataApp myApp(tokens);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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
    EXPECT_TRUE(fs::exists("/tmp/test_charts/SPY_0.05X1_linear.json"));
    EXPECT_TRUE(fs::exists("/tmp/test_charts/SPY_0.05X1_linear.svg"));
    EXPECT_TRUE(fs::exists("/tmp/test_charts/AAPL_0.05X1_linear.json"));
    EXPECT_TRUE(fs::exists("/tmp/test_charts/AAPL_0.05X1_linear.svg"));
}

TEST_F(StreamData, TryLogarithmicCharts)    //NOLINT
{
    if (fs::exists("/tmp/test_charts_log"))
    {
        fs::remove_all("/tmp/test_charts_log");
    }
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--symbol", "GOOG",
        "--symbol", "AAPL",
        "--new-data-source", "streaming",
        "--mode", "load",
        "--interval", "live",
        "--scale", "percent",
        "--price-fld-name", "close",
        "--destination", "file",
        "--output-chart-dir", "/tmp/test_charts_log",
        "--boxsize", "0.01",
        "--reversal", "1"
	};

	try
	{
        PF_CollectDataApp myApp(tokens);

		const auto *test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(fmt::format("\n\nTest: {}  test case: {} \n\n", test_info->name(), test_info->test_suite_name()));

        bool startup_OK = myApp.Startup();

        auto now = date::zoned_seconds(date::current_zone(), floor<std::chrono::seconds>(std::chrono::system_clock::now()));
        auto then = date::zoned_seconds(date::current_zone(), floor<std::chrono::seconds>(std::chrono::system_clock::now()) + 15s);

        int counter = 0;
        auto timer = [&counter] (const auto& stop_at)
            { 
                while (true)
                {
                    std::cout << "ding...\n";
                    ++counter;
                    auto now = date::zoned_seconds(date::current_zone(), floor<std::chrono::seconds>(std::chrono::system_clock::now()));
                    if (now.get_sys_time() >= stop_at.get_sys_time())
                    {
                        PF_CollectDataApp::SetSignal();
                        break;
                    }
                    std::this_thread::sleep_for(1s);
                }
            };

        if (startup_OK)
        {
            // add an external timer here.
            auto timer_task = std::async(std::launch::async, timer, then);

            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
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
    EXPECT_TRUE(fs::exists("/tmp/test_charts_log/GOOG_0.01%X1_percent.json"));
    EXPECT_TRUE(fs::exists("/tmp/test_charts_log/GOOG_0.01%X1_percent.svg"));
    EXPECT_TRUE(fs::exists("/tmp/test_charts_log/AAPL_0.01%X1_percent.json"));
    EXPECT_TRUE(fs::exists("/tmp/test_charts_log/AAPL_0.01%X1_percent.svg"));
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

    py::scoped_interpreter guard{false}; // start the interpreter and keep it alive

    py::print("Hello, World!"); // use the Python API

    py::exec(R"(
        import PF_DrawChart_prices as PF_DrawChart
        )"
    );
	InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
