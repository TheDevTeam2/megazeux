call VersionSettings.bat

:: Build and package
usr\bin\bash -l /dk-psp-build.sh %stable_branch%

pause
