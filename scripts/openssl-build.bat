@echo off

echo Install strawberry perl, nasm and add latter to global PATH manually

if [%1] == [] (
  echo Build architecture [32 or 64] argument missing
  exit /b 1
)

set BUILD_ARCH=%1
set BUILD_ARCH_SUFFIX=%BUILD_ARCH%

if %BUILD_ARCH% neq 64 (
  if %BUILD_ARCH% neq 32 (
    echo Build architecture argument should be one of 32 or 64
    exit /b 1
  )
) else (
  set BUILD_ARCH_SUFFIX=64A
)

set OPENSSL_VERSION=1.1.1k
set OPENSSL_PREFIX=C:/openssl-%OPENSSL_VERSION%/win%BUILD_ARCH%

perl Configure VC-WIN%BUILD_ARCH_SUFFIX% --prefix=%OPENSSL_PREFIX% && nmake && nmake install
