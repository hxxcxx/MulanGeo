@echo off
setlocal EnableDelayedExpansion

:: ============================================================
:: build-wasm.bat
:: 使用 emsdk 将 MulanGeo Engine 编译为 WebAssembly
:: 用法: script\build-wasm.bat [Debug|Release]
:: ============================================================

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

:: 规范化项目根目录路径（消除 .. 避免 call 路径解析失败）
set SCRIPT_DIR=%~dp0
for %%A in ("%SCRIPT_DIR%..") do set ROOT_DIR=%%~fA
set WASM_SRC_DIR=%ROOT_DIR%\src\build-wasm
set BUILD_DIR=%ROOT_DIR%\build-wasm-%BUILD_TYPE%

echo [INFO] Root: %ROOT_DIR%
echo [INFO] Build dir: %BUILD_DIR%

:: ── 1. 定位 / 拉取 emsdk ─────────────────────────────────────
set EMSDK_DIR=%ROOT_DIR%\src\emsdk

if not exist "%EMSDK_DIR%\emsdk.bat" (
    if defined EMSDK (
        set EMSDK_DIR=!EMSDK!
        echo [INFO] Using external EMSDK: !EMSDK_DIR!
    ) else (
        echo [INFO] emsdk not found, cloning from GitHub...
        where git >nul 2>&1
        if errorlevel 1 (
            echo [ERROR] git not found in PATH.
            exit /b 1
        )
        git clone --depth=1 https://github.com/emscripten-core/emsdk.git "%EMSDK_DIR%"
        if errorlevel 1 (
            echo [ERROR] Failed to clone emsdk repository.
            exit /b 1
        )
    )
)

:: 若未安装工具链则先 install + activate
if not exist "%EMSDK_DIR%\upstream\emscripten\emcc.bat" (
    echo [INFO] Installing Emscripten toolchain...
    call "%EMSDK_DIR%\emsdk.bat" install latest
    if errorlevel 1 ( echo [ERROR] emsdk install failed. & exit /b 1 )
    call "%EMSDK_DIR%\emsdk.bat" activate latest
    if errorlevel 1 ( echo [ERROR] emsdk activate failed. & exit /b 1 )
)

echo [INFO] Using emsdk: %EMSDK_DIR%

:: ── 2. 激活 emsdk 环境（忽略其返回码，它会清理一些变量导致误判）──
call "%EMSDK_DIR%\emsdk_env.bat" >nul 2>&1

:: 验证 emcmake 可用
where emcmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] emcmake not found in PATH. Run emsdk install/activate first.
    exit /b 1
)

:: ── 3. 定位 Ninja（Visual Studio 自带）──────────────────────
set NINJA_EXE=

if exist "%EMSDK_DIR%\upstream\bin\ninja.exe" (
    set NINJA_EXE=%EMSDK_DIR%\upstream\bin\ninja.exe
)

if "!NINJA_EXE!"=="" (
    for /d %%Y in ("C:\Program Files\Microsoft Visual Studio\*") do (
        for /d %%E in ("%%Y\*") do (
            if exist "%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" (
                set NINJA_EXE=%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe
            )
        )
    )
)

if "!NINJA_EXE!"=="" (
    where ninja >nul 2>&1
    if not errorlevel 1 (
        for /f "delims=" %%P in ('where ninja') do set NINJA_EXE=%%P
    )
)

if "!NINJA_EXE!"=="" (
    echo [ERROR] Ninja not found. Install Visual Studio with "C++ CMake tools for Windows".
    exit /b 1
)
echo [INFO] Ninja: !NINJA_EXE!

:: ── 4. 定位 GLM ──────────────────────────────────────────────
set GLM_DIR=
if exist "%ROOT_DIR%\vcpkg_installed\wasm32-emscripten\include" (
    set GLM_DIR=%ROOT_DIR%\vcpkg_installed\wasm32-emscripten\include
) else if exist "%ROOT_DIR%\vcpkg_installed\x64-windows\include" (
    set GLM_DIR=%ROOT_DIR%\vcpkg_installed\x64-windows\include
)
if not "!GLM_DIR!"=="" echo [INFO] GLM: !GLM_DIR!

:: ── 5. 创建构建目录 ──────────────────────────────────────────
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: ── 6. CMake Configure ───────────────────────────────────────
echo [INFO] Configuring...

set EM_CMAKE=%EMSDK_DIR%\upstream\emscripten\emcmake.bat
if not exist "!EM_CMAKE!" set EM_CMAKE=emcmake

if not "!GLM_DIR!"=="" (
    call "!EM_CMAKE!" cmake -S "%WASM_SRC_DIR%" -B "%BUILD_DIR%" -G Ninja "-DCMAKE_MAKE_PROGRAM=!NINJA_EXE!" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% "-DGLM_DIR=!GLM_DIR!"
) else (
    call "!EM_CMAKE!" cmake -S "%WASM_SRC_DIR%" -B "%BUILD_DIR%" -G Ninja "-DCMAKE_MAKE_PROGRAM=!NINJA_EXE!" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
)
set CONFIGURE_RESULT=%ERRORLEVEL%
echo [INFO] Configure exit code: %CONFIGURE_RESULT%
if %CONFIGURE_RESULT% NEQ 0 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

:: ── 7. Build ─────────────────────────────────────────────────
echo [INFO] Building...
cmake --build "%BUILD_DIR%" -- -j%NUMBER_OF_PROCESSORS%
set BUILD_RESULT=%ERRORLEVEL%
if %BUILD_RESULT% NEQ 0 (
    echo [ERROR] Build failed.
    exit /b 1
)

:: ── 8. 输出结果 ──────────────────────────────────────────────
echo.
echo [SUCCESS] Build complete! Output:
if exist "%BUILD_DIR%\MulanGeoWasm.html" echo   %BUILD_DIR%\MulanGeoWasm.html
if exist "%BUILD_DIR%\MulanGeoWasm.js"   echo   %BUILD_DIR%\MulanGeoWasm.js
if exist "%BUILD_DIR%\MulanGeoWasm.wasm" echo   %BUILD_DIR%\MulanGeoWasm.wasm

:: ── 9. 启动本地服务器并自动打开浏览器 ──────────────────────
echo.
echo [INFO] Starting local HTTP server on http://localhost:8080 ...

:: 延迟 1 秒后自动打开浏览器（给服务器启动留出时间）
start "" /b cmd /c "ping 127.0.0.1 -n 2 >nul && start http://localhost:8080/MulanGeoWasm.html"

cd /d "%BUILD_DIR%"
python -m http.server 8080

endlocal