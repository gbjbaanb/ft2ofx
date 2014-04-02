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

//#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>



bool g_debug = false;


// structure to hold the columns from the csv we want
struct StockData
{
	string symbol, currency, price, name;
	bool fund;
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
					sd.price = *it;
					sd.price.erase(std::remove(sd.price.begin(), sd.price.end(), ','), sd.price.end());
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
			if (g_debug)
			{
				cout << sd.symbol << " " << sd.price << " " << sd.currency;
				if (sd.fund)
					cout << " (F)";
			}

			if (sd.symbol.empty() || sd.currency.empty() || sd.price.empty() || sd.price[0] == '0' || sd.price == "n/a")
			{
				if (g_debug)
					cout << "   no data  " << endl;
			}
			else
			{
				// if the price is given in pence, convert it to pounds that's required for import
				if (sd.currency == "GBX")
				{
					double gbp = stod(sd.price);
					gbp /= 100;
					sd.price = to_string(gbp);
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
void generateOFX(string filename, list<StockData> columns)
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

		boost::uuids::random_generator gen;
		boost::uuids::uuid uuid = gen();

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
			<< "      <TRNUID>" << to_string(uuid) << "</TRNUID>" << endl
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
				<< "              <UNITS>0.000</UNITS>" << endl
				<< "              <UNITPRICE>" << i.price << "</UNITPRICE>" << endl
				<< "              <MKTVAL>0.00</MKTVAL>" << endl
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
			("url", po::value<string>()->default_value(""), "set url to auto-download csv")
			("colname", po::value<int>(), "set column in csv to read stock name from")
			("colsym", po::value<int>(), "set column in csv to read symbol from")
			("colcurrency", po::value<int>(), "set column in csv to read currency from")
			("colprice", po::value<int>(), "set column in csv to read price from")
			("colfund", po::value<int>(), "set column in csv to select managed fund")
			("debug", po::value<bool>()->default_value(false), "turns on debugging to console")
			;

		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help") || !vm.count("file")) 
		{
			cout << desc << "\n";
			return 0;
		}

		g_debug = vm["debug"].as<bool>();

		list<StockData> columns;
		if (vm.count("file"))
		{
			// lets convert this data
			columns = parseFile(vm["file"].as<string>(),
				vm["colsym"].as<int>(), vm["colcurrency"].as<int>(), vm["colprice"].as<int>(), vm["colname"].as<int>(), vm["colfund"].as<int>());
		}
		else
		{
			MemoryStruct csv;
			DownloadFile(vm["url"].as<string>(), csv);
			// lets convert this data
			columns = parseFile(vm["file"].as<string>(),
				vm["colsym"].as<int>(), vm["colcurrency"].as<int>(), vm["colprice"].as<int>(), vm["colname"].as<int>(), vm["colfund"].as<int>());
		}

		if (columns.size() > 0)
		{
			if (g_debug)
				cout << "generating ofx" << endl;

			// turn it into an ofx in the same directory as the source
			string outputfile = vm["file"].as<string>();
			generateOFX(outputfile.substr(0, outputfile.find_last_of('.')), columns);
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

