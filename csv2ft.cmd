set PROGRAM=C:\Work\csv2ofx.exe
set DOWNLOAD=C:\Downloads



IF '%1' NEQ '' ( 
	"%PROGRAM%"  --colsym 1 --colprice 2 --colcurrency 3 --colname 0  --colfund 12 --file %1 
) ELSE (
	FOR %%i IN ("%DOWNLOAD%\Money_*.csv") DO   "%PROGRAM%" --colsym 1 --colprice 2 --colcurrency 3 --colname 0 --colfund 12 --debug 1 --file %%i
)
