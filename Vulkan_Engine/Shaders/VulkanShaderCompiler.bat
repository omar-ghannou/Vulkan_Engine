@ECHO OFF
set SHDR=%1
set TYP=%2

shift
shift

setlocal enabledelayedexpansion

echo processing %SHDR% ...
for /f "tokens=1 delims=." %%a in ("%SHDR%") do (
  glslc Shaders/%SHDR% -o Shaders/SPIR-V/%%a%TYP%.spv
  )

endlocal