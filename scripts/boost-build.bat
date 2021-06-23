@echo off

if [%1] == [] (
  echo Build architecture [32 or 64] argument missing
  exit /b 1
)

set BUILD_ARCH=%1

if %BUILD_ARCH% neq 64 (
  if %BUILD_ARCH% neq 32 (
    echo Build architecture argument should be one of 32 or 64
    exit /b 1
  )
)

set BOOST_VERSION=1.76.0
set BOOST_PREFIX=C:/boost-%BOOST_VERSION%/win%BUILD_ARCH%

cmd /c .\b2 clean
cmd /c .\bootstrap.bat

.\b2 -j16 --prefix=%BOOST_PREFIX% --build-dir=build --without-python toolset=msvc variant=release variant=debug threading=multi link=static address-model=%BUILD_ARCH% install
