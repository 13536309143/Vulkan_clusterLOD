@echo off
setlocal

cd /d "%~dp0"

set EXE=_bin\Release\t1.exe
if not exist "%EXE%" (
  echo Missing %EXE%
  echo Build first: cmake --build build --config Release
  exit /b 1
)

set COMMON_ARGS=--processingonly 1 --processingpartial 1 --processingthreadpct 0.75

for %%M in (a b c o1778 o3049) do (
  echo.
  echo ===== Preprocess %%M.glb with semantic LOD policies =====
  "%EXE%" --scene "_downloaded_resources\%%M.glb" %COMMON_ARGS%
  if errorlevel 1 (
    echo Failed while preprocessing %%M.glb
    exit /b 1
  )
)

echo.
echo Done. Cache files are written next to the GLB files as .zippp.
