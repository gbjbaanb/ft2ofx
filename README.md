# ft2ofx

Hi after FT changed the columns for its csv stock download feature (thanks to the FT for the service!) the old program I used to convert the csv into an ofx suitable for import to MS Money stopped doing the import, so as it wasn't open source, I figured it was best to knock out a quick program to do this for me.

This is the first stab at it. Its quick and dirty and I'll work on user friendliness later. First things first - getting it to work.

So it works, its a windows command line exe so you can run it in a cmd window, or from a batch file (or an explorer shortcut)

If you open a cmd window, and type 'csv2ofx.exe --help' it'll show you the options. Enter them and it will turn a csv into an ofx in the same directory as the csv file.

eg: csv2ofx.exe --colsym 1 --colprice 2 --colcurrency 3 --colname 0 --colfund 12 --file c:\downloads\Money2014-03-190555PM.csv

The 4 colxx parameters tell it which columns in the csv are used for which data (symbol, currency, price and name). In my default portfolio download, these are set as above.

Then give it the csv and let it go.

Download the .exe and .cmd file from the downloads section.

# Documentation
To work it:

Get the csv2ofx.exe and the csv2ft.cmd file Edit the csv2ft.cmd file to use the paths you have put the exe, and where you download the FT csv to.

Configure your portfolio view:

The --colxxx bits in the csv2ft file tell the program which columns in the csv to use for which bits (the first column is number 0), so you need to edit your portfolio view to contain symbol, price, currency, name (which should be straightforward for you default view, but you'll have to edit the fund one), in the fund view add an extra column for 'institutional share class' - and set this to be the --colfund, it tells the converter that the line is a fund rather than a stock.

Download CSV
Download your FT csv (I have a shortcut, when I download one from the website, right click on it in firefox and it'll show you the 'download location', cut and paste this url into an 'internet shortcut' - you can get one by dragging the little icon at the left of your address bar in your browser to your computer or desktop or whereever)

Run app
Double-click the csv2ft.cmd and it will look for all files called Money.csv in that directory, and will convert them.
See the Configuration page for details of how to configure the program.

Import
Then you double-click the .ofx that will be in the same directory as your downloaded csv file to load it into Money.
