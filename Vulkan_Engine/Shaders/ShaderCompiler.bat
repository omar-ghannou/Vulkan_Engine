@ECHO OFF
set /p VS= Enter the name of the vertex shader (with extension) : 
set /p FS= Enter the name of the fragment shader (with extension): 

setlocal enabledelayedexpansion

echo processing %VS% :
for /f "tokens=1 delims=." %%a in ("%VS%") do (
  glslc %VS% -o %%aVert.spv
  )
echo processing %FS% :
for /f "tokens=1 delims=." %%a in ("%FS%") do (
  glslc %FS% -o %%aFrag.spv
  )


pause