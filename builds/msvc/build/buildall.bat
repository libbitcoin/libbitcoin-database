@ECHO OFF
ECHO.
ECHO Downloading libitcoin-database dependencies from NuGet
CALL nuget.exe install ..\vs2013\libbitcoin-database\packages.config
ECHO.
CALL buildbase.bat ..\vs2013\libbitcoin-database.sln 12
ECHO.
PAUSE