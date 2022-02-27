@ECHO OFF
ECHO Downloading libbitcoin vs2022 dependencies from NuGet
CALL nuget.exe install ..\vs2022\libbitcoin-database\packages.config
CALL nuget.exe install ..\vs2022\libbitcoin-database-tools\packages.config
CALL nuget.exe install ..\vs2022\libbitcoin-database-test\packages.config
ECHO.
ECHO Downloading libbitcoin vs2019 dependencies from NuGet
CALL nuget.exe install ..\vs2019\libbitcoin-database\packages.config
CALL nuget.exe install ..\vs2019\libbitcoin-database-tools\packages.config
CALL nuget.exe install ..\vs2019\libbitcoin-database-test\packages.config
ECHO.
ECHO Downloading libbitcoin vs2017 dependencies from NuGet
CALL nuget.exe install ..\vs2017\libbitcoin-database\packages.config
CALL nuget.exe install ..\vs2017\libbitcoin-database-tools\packages.config
CALL nuget.exe install ..\vs2017\libbitcoin-database-test\packages.config
