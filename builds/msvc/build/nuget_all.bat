@ECHO OFF
ECHO Downloading libbitcoin vs2026 dependencies from NuGet
CALL nuget.exe install ..\vs2026\libbitcoin-database\packages.config
CALL nuget.exe install ..\vs2026\libbitcoin-database-tools\packages.config
CALL nuget.exe install ..\vs2026\libbitcoin-database-test\packages.config
