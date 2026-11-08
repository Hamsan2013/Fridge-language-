@echo off
SET FRIDGE_DIR=%~dp0..
SET EXE_PATH="%FRIDGE_DIR%\bin\fridge.exe"
SET ICO_PATH="%FRIDGE_DIR%\resources\fridge.ico"

echo Registering .frg and .uzip file extensions with FRIDGE icon...

assoc .frg=FridgeSourceFile
ftype FridgeSourceFile=%EXE_PATH% run "%%1"
reg add "HKCR\FridgeSourceFile\DefaultIcon" /ve /d "%ICO_PATH%,0" /f

assoc .uzip=FridgePackageFile
ftype FridgePackageFile=%EXE_PATH% run "%%1"
reg add "HKCR\FridgePackageFile\DefaultIcon" /ve /d "%ICO_PATH%,0" /f

echo File associations and icons applied successfully!
pause
