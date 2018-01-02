@ECHO OFF
ECHO Downloading libbitcoin vs2017 dependencies from NuGet
CALL nuget.exe install ..\vs2017\libbitcoin-database\packages.config
CALL nuget.exe install ..\vs2017\libbitcoin-database-tools\packages.config
CALL nuget.exe install ..\vs2017\libbitcoin-database-test\packages.config
ECHO.
ECHO Downloading libbitcoin vs2015 dependencies from NuGet
CALL nuget.exe install ..\vs2015\libbitcoin-database\packages.config
CALL nuget.exe install ..\vs2015\libbitcoin-database-tools\packages.config
CALL nuget.exe install ..\vs2015\libbitcoin-database-test\packages.config
ECHO.
ECHO Downloading libbitcoin vs2013 dependencies from NuGet
CALL nuget.exe install ..\vs2013\libbitcoin-database\packages.config
CALL nuget.exe install ..\vs2013\libbitcoin-database-tools\packages.config
CALL nuget.exe install ..\vs2013\libbitcoin-database-test\packages.config
