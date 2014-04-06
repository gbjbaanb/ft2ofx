// MoneyImporterFT.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <list>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <iterator>
using namespace std;

#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/find.hpp>

#include <rpc.h>



bool g_debug = false;


// structure to hold the columns from the csv we want
struct StockData
{
	string symbol, currency, name;
	double price;
	bool fund = false;
};



// reads the csv and extracts the column data we want
// stores it in a list of stocks
list<StockData> parseFile(string filename, int symbol, int currency, int price, int name, int fund)
{
	list<StockData> columns;
	ifstream fin;

	fin.open(filename);
	if (!fin.fail())
	{
		// check eol character (as these .csv files have \r on my download, not the usual \n or \r\n
		// this also eats the first header line.
		char eol_ch = 0;
		while (eol_ch != '\r' && eol_ch != '\n')
			eol_ch = fin.get();

		std::string line;
		while (!fin.eof())
		{
			getline(fin, line, eol_ch);

			StockData sd;

			boost::tokenizer<boost::escaped_list_separator<char> > tok(line, boost::escaped_list_separator<char>('\\', ',', '\"'));
			boost::tokenizer<boost::escaped_list_separator<char> >::iterator it = tok.begin();
			for (size_t col = 0; it != tok.end(); col++, ++it)
			{
				if (col == symbol)
					sd.symbol = *it;
				else if (col == currency)
					sd.currency = *it;
				else if (col == price)
				{
					string price = *it;
					price.erase(std::remove(price.begin(), price.end(), ','), price.end());
					sd.price = stod(price);
				}
				else if (col == name)
				{
					sd.name = *it;
					boost::replace_all(sd.name, "&", "&amp;");
				}
				else if (col == fund)
				{
					try
					{
						if (boost::lexical_cast<int>(*it) != 0)
							sd.fund = true;
					}
					catch (boost::bad_lexical_cast const& e)
					{
						sd.fund = false;
					}
				}
			}

			// no symbol - nothing we can do.
			if (sd.symbol.empty())
				continue;

			if (g_debug)
			{
				cout << sd.symbol << " " << sd.price << " " << sd.currency;
				if (sd.fund)
					cout << " (F)";
			}

			if (sd.currency.empty() || sd.price == 0)
			{
				cout << "   no price data  " << endl;
			}
			else
			{
				// if the price is given in pence, convert it to pounds that's required for import
				if (sd.currency == "GBX")
				{
					sd.currency = "GBP";
					sd.price /= 100;
					if (g_debug)
						cout << "  -- converting price to " << sd.price << endl;
				}
				columns.push_back(sd);
			}
		}
		fin.close();
	}
	return columns;
}



// generates standard OFX from list of data in list
void generateOFX(string filename, list<StockData> columns, double addshares, bool stripsuffix)
{
	ofstream fout(filename + ".ofx", std::ios::out);
	if (!fout.fail())
	{
		time_t now;
		time(&now);
		struct tm tmnow;
		localtime_s(&tmnow, &now);

		string strNow;
		strNow = str(boost::format("%04d%02d%02d%02d%02d%02d") % (tmnow.tm_year + 1900) % (tmnow.tm_mon + 1) % tmnow.tm_mday % tmnow.tm_hour % tmnow.tm_min % tmnow.tm_sec);

		UUID uuid;
		UuidCreate(&uuid);
		RPC_CSTR strUuid;
		UuidToStringA(&uuid, &strUuid);

		// emit some preamble
		fout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl
			<< "<?OFX OFXHEADER=\"200\" VERSION=\"200\" SECURITY=\"NONE\" OLDFILEUID=\"NONE\" NEWFILEUID=\"NONE\"?>" << endl
			<< "<OFX>" << endl
			<< "  <SIGNONMSGSRSV1>" << endl
			<< "    <SONRS>" << endl
			<< "      <STATUS>" << endl
			<< "        <CODE>0</CODE>" << endl
			<< "        <SEVERITY>INFO</SEVERITY>" << endl
			<< "        <MESSAGE>Successful Sign On</MESSAGE>" << endl
			<< "      </STATUS>" << endl
			<< "      <DTSERVER>" << strNow << "</DTSERVER>" << endl
			<< "      <LANGUAGE>ENG</LANGUAGE>" << endl
			<< "    </SONRS>" << endl
			<< "  </SIGNONMSGSRSV1>" << endl
			<< "  <INVSTMTMSGSRSV1>" << endl
			<< "    <INVSTMTTRNRS>" << endl
			<< "      <TRNUID>" << strUuid << "</TRNUID>" << endl
			<< "      <STATUS>" << endl
			<< "        <CODE>0</CODE>" << endl
			<< "        <SEVERITY>INFO</SEVERITY>" << endl
			<< "      </STATUS>" << endl
			<< "      <INVSTMTRS>" << endl
			<< "        <DTASOF>" << strNow << "</DTASOF>" << endl
			<< "        <CURDEF>GBP</CURDEF>" << endl
			<< "        <INVACCTFROM>" << endl
			<< "          <BROKERID>le.com</BROKERID>" << endl
			<< "          <ACCTID>0123456789</ACCTID>" << endl
			<< "        </INVACCTFROM>" << endl
			<< "        <INVPOSLIST>" << endl;

		// emit a 'posstock' block for each stock
		for (StockData& i : columns)
		{
			// strip :LSE or :GBX off the symbols, if configured
			if (stripsuffix)
			{
				if (i.symbol.find_last_of(':') != std::string::npos)
					i.symbol.erase(i.symbol.find_last_of(':'));
			}

			if (i.fund)
				fout << "          <POSMF>" << endl;
			else
				fout << "          <POSSTOCK>" << endl;

			fout << "            <INVPOS>" << endl
				<< "              <SECID>" << endl
				<< "                <UNIQUEID>" << i.symbol << "</UNIQUEID>" << endl
				<< "                <UNIQUEIDTYPE>TICKER</UNIQUEIDTYPE>" << endl
				<< "              </SECID>" << endl
				<< "              <HELDINACCT>OTHER</HELDINACCT>" << endl
				<< "              <POSTYPE>LONG</POSTYPE>" << endl
				<< "              <UNITS>" << addshares << "</UNITS>" << endl
				<< "              <UNITPRICE>" << i.price << "</UNITPRICE>" << endl
				<< "              <MKTVAL>" << i.price * addshares << "</MKTVAL>" << endl
				<< "              <DTPRICEASOF>" << strNow << "</DTPRICEASOF>" << endl
				<< "              <CURRENCY>" << endl
				<< "                <CURRATE>1.00</CURRATE>" << endl
				<< "                <CURSYM>" << i.currency << "</CURSYM>" << endl
				<< "              </CURRENCY>" << endl
				<< "              <MEMO>Price as of date based on closing price</MEMO>" << endl
				<< "            </INVPOS>" << endl
				<< "            <REINVDIV>Y</REINVDIV>" << endl;

			if (i.fund)
				fout << "            <REINVCG>Y</REINVCG>" << endl
					 << "          </POSMF>" << endl;
			else
				fout << "          </POSSTOCK>" << endl;
		}

		fout << "        </INVPOSLIST>" << endl
			<< "      </INVSTMTRS>" << endl
			<< "    </INVSTMTTRNRS>" << endl
			<< "  </INVSTMTMSGSRSV1>" << endl
			<< "  <SECLISTMSGSRSV1>" << endl
			<< "    <SECLIST>" << endl;

		// emit a 'stockinfo' block for each stock
		for (StockData& i : columns)
		{
			if (i.fund)
				fout << "      <MFINFO>" << endl;
			else
				fout << "      <STOCKINFO>" << endl;

			fout << "        <SECINFO>" << endl
				<< "          <SECID>" << endl
				<< "            <UNIQUEID>" << i.symbol << "</UNIQUEID>" << endl
				<< "            <UNIQUEIDTYPE>TICKER</UNIQUEIDTYPE>" << endl
				<< "          </SECID>" << endl
				<< "          <SECNAME>" << i.name << "</SECNAME>" << endl
				<< "          <TICKER>" << i.symbol << "</TICKER>" << endl
				<< "          <UNITPRICE>" << i.price << "</UNITPRICE>" << endl
				<< "          <DTASOF>" << strNow << "</DTASOF>" << endl
				<< "          <CURRENCY>" << endl
				<< "            <CURRATE>1.00</CURRATE>" << endl
				<< "            <CURSYM>" << i.currency << "</CURSYM>" << endl
				<< "          </CURRENCY>" << endl
				<< "          <MEMO>Price as of date based on closing price</MEMO>" << endl
				<< "        </SECINFO>" << endl;

			if (i.fund)
				fout << "        <MFTYPE>OPENEND</MFTYPE>" << endl
					 << "      </MFINFO>" << endl;
			else
				fout << "      </STOCKINFO>" << endl;
		}

		// and close it all up
		fout << "    </SECLIST>" << endl
			<< "  </SECLISTMSGSRSV1>" << endl
			<< "</OFX>";

		RpcStringFreeA(&strUuid);
	}
}




int _tmain(int argc, _TCHAR* argv[])
{
	po::variables_map vm;

	try {
		po::options_description desc("Allowed options");
		desc.add_options()
			("help", "produce help message")
			("file", po::value<string>()->default_value(""), "set downloaded csv file")
//			("url", po::value<string>()->default_value(""), "set url to auto-download csv")
			("addshares", po::value<double>()->default_value(0.0), "sets each entry to this number of shares (eg 0.001), default 0")
			("stripsuffix", po::value<bool>()->default_value(false), "strips :LSE or :GBX or similar off symbols, default leave")
			("colname", po::value<int>(), "set column in csv to read stock name from")
			("colsym", po::value<int>(), "set column in csv to read symbol from")
			("colcurrency", po::value<int>(), "set column in csv to read currency from")
			("colprice", po::value<int>(), "set column in csv to read price from")
			("colfund", po::value<int>()->default_value(-1), "set column in csv to select managed fund, default notused.")
			("debug", po::value<bool>()->default_value(false), "turns on debugging to console")
			;

		const _TCHAR* config_filename = "csv2ofx.config";
		try
		{
			po::store(po::parse_config_file<_TCHAR>(config_filename, desc), vm);
		}
		catch (boost::program_options::error)
		{
		}
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help") || !vm.count("file")) 
		{
			cout << desc << "\n";
			return 0;
		}

		g_debug = vm["debug"].as<bool>();

		if (vm["addshares"].as<double>())
		{
			{
				ifstream config(config_filename);
				ofstream config_out("csv2ofx.temp");
				if (config.fail())
					config_out << "addshares=" << vm["addshares"].as<double>() + 0.001 << endl;
				else
				{
					while (!config.eof())
					{
						string line;
						getline(config, line);
						if (boost::find_first(line, "addshares"))
							config_out << "addshares=" << vm["addshares"].as<double>() + 0.001 << endl;
						else
							config_out << line;
					}
				}
			}
			remove(config_filename);
			rename("csv2ofx.temp", config_filename);
		}

		// lets convert this data
		list<StockData> columns = parseFile(vm["file"].as<string>(), 
											vm["colsym"].as<int>(), vm["colcurrency"].as<int>(), vm["colprice"].as<int>(), vm["colname"].as<int>(), vm["colfund"].as<int>());
		if (columns.size() > 0)
		{
			if (g_debug)
				cout << "generating ofx" << endl;

			// turn it into an ofx in the same directory as the source
			string outputfile = vm["file"].as<string>();
			generateOFX(outputfile.substr(0, outputfile.find_last_of('.')), columns, vm["addshares"].as<double>(), vm["stripsuffix"].as<bool>());
		}
	}
	catch (exception& e) {
		cerr << "error: " << e.what() << "\n";
		return 1;
	}
	catch (...) {
		cerr << "Exception of unknown type!\n";
	}

	return 0;
}

