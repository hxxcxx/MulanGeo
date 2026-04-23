@echo off
setlocal EnableDelayedExpansion

:: ============================================================
:: build-wasm.bat
:: 使用 emsdk 将 MulanGeo Engine 编译为 WebAssembly
:: 用法: script\build-wasm.bat [Debug|Release]
:: ============================================================

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

:: 脚本所在目录的上级即为项目根目录（用 FOR 规范化，消除 .. 避免 call 路径解析失败）
set SCRIPT_DIR=%~dp0
for %%A in ("%SCRIPT_DIR%..") do set ROOT_DIR=%%~fA
set WASM_SRC_DIR=%ROOT_DIR%\src\build-wasm
set BUILD_DIR=%ROOT_DIR%\build-wasm-%BUILD_TYPE%

:: ── 1. 定位 / 拉取 emsdk ─────────────────────────────────────
:: 优先使用项目内置 emsdk（src/emsdk），其次使用环境变量 EMSDK
set EMSDK_DIR=%ROOT_DIR%\src\emsdk

if not exist "%EMSDK_DIR%\emsdk.bat" (
    if defined EMSDK (
        :: 使用外部已有的 emsdk
        set EMSDK_DIR=%EMSDK%
        echo [INFO] Using external EMSDK: !EMSDK_DIR!
    ) else (
        :: 自动 clone emsdk 到 src/emsdk
        echo [INFO] emsdk not found, cloning from GitHub...
        where git >nul 2>&1
        if errorlevel 1 (
            echo [ERROR] git not found in PATH. Please install Git and try again.
            exit /b 1
        )
        git clone --depth=1 https://github.com/emscripten-core/emsdk.git "%EMSDK_DIR%"
        if errorlevel 1 (
            echo [ERROR] Failed to clone emsdk repository.
            exit /b 1
        )
        echo [INFO] emsdk cloned to: %EMSDK_DIR%
    )
)

:: 若刚 clone 或首次使用，需要 install + activate latest
if not exist "%EMSDK_DIR%\upstream\emscripten\emcc.bat" (
    echo [INFO] Installing and activating latest Emscripten toolchain...
    call "%EMSDK_DIR%\emsdk.bat" install latest
    if errorlevel 1 (
        echo [ERROR] emsdk install failed.
        exit /b 1
    )
    call "%EMSDK_DIR%\emsdk.bat" activate latest
    if errorlevel 1 (
        echo [ERROR] emsdk activate failed.
        exit /b 1
    )
    echo [INFO] Emscripten toolchain installed and activated.
)

echo [INFO] Using emsdk: %EMSDK_DIR%

:: ── 2. 激活 emsdk 环境 ───────────────────────────────────────
call "%EMSDK_DIR%\emsdk_env.bat"
if errorlevel 1 (
    echo [ERROR] Failed to activate emsdk environment.
    exit /b 1
)

:: 验证 emcmake 可用
where emcmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] emcmake not found in PATH after emsdk_env.bat.
    echo   Try running: %EMSDK_DIR%\emsdk install latest ^&^& %EMSDK_DIR%\emsdk activate latest
    exit /b 1
)

:: ── 3. 定位 Ninja（使用 Visual Studio 自带）────────────────
set NINJA_EXE=

:: emsdk 自带 ninja（upstream/bin/ninja.exe）
if exist "%EMSDK_DIR%\upstream\bin\ninja.exe" (
    set NINJA_EXE=%EMSDK_DIR%\upstream\bin\ninja.exe
    echo [INFO] Using emsdk ninja: !NINJA_EXE!
)

:: Visual Studio 自带 ninja（遍历所有版本/SKU）
if "!NINJA_EXE!"=="" (
    for /d %%Y in ("C:\Program Files\Microsoft Visual Studio\*") do (
        for /d %%E in ("%%Y\*") do (
            if exist "%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" (
                set NINJA_EXE=%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe
            )
        )
    )
    if not "!NINJA_EXE!"=="" echo [INFO] Using Visual Studio ninja: !NINJA_EXE!
)

:: 系统 PATH 中的 ninja（兜底）
if "!NINJA_EXE!"=="" (
    where ninja >nul 2>&1
    if not errorlevel 1 (
        for /f "delims=" %%P in ('where ninja') do set NINJA_EXE=%%P
        echo [INFO] Using system ninja: !NINJA_EXE!
    )
)

if "!NINJA_EXE!"=="" (
    echo [ERROR] Ninja not found. Please install Visual Studio with "C++ CMake tools for Windows" component.
    exit /b 1
)

:: ── 3. 定位 GLM ──────────────────────────────────────────────
:: 尝试从 vcpkg wasm32-emscripten triplet 获取 glm
set GLM_DIR=
if exist "%ROOT_DIR%\vcpkg_installed\wasm32-emscripten\include" (
    set GLM_DIR=%ROOT_DIR%\vcpkg_installed\wasm32-emscripten\include
    echo [INFO] Found vcpkg GLM for wasm32-emscripten: !GLM_DIR!
) else if exist "%ROOT_DIR%\vcpkg_installed\x64-windows\include" (
    :: header-only 库，x64 头文件同样可用于 WASM 编译
    set GLM_DIR=%ROOT_DIR%\vcpkg_installed\x64-windows\include
    echo [INFO] Using x64-windows GLM headers for WASM: !GLM_DIR!
) else (
    echo [WARN] GLM not found in vcpkg_installed. Build may fail if GLM is missing.
    echo   To fix: vcpkg install glm:wasm32-emscripten
)

:: ── 4. 创建构建目录 ──────────────────────────────────────────
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: ── 5. CMake Configure（通过 emcmake 包装）───────────────────
echo [INFO] Configuring CMake for WebAssembly (%BUILD_TYPE%)...
echo [INFO] Build directory: %BUILD_DIR%

set CMAKE_EXTRA=
if not "%GLM_DIR%"=="" (
    set CMAKE_EXTRA=-DGLM_DIR="%GLM_DIR%"
)

emcmake cmake -S "%WASM_SRC_DIR%" ^
              -B "%BUILD_DIR%" ^
              -G Ninja ^
              -DCMAKE_MAKE_PROGRAM="%NINJA_EXE%" ^
              -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
              %CMAKE_EXTRA%

if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

:: ── 6. Build ─────────────────────────────────────────────────
echo [INFO] Building...
cmake --build "%BUILD_DIR%" -- -j%NUMBER_OF_PROCESSORS%

if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

:: ── 7. 输出结果 ──────────────────────────────────────────────
echo.
echo [SUCCESS] Build complete!
echo   Output directory: %BUILD_DIR%
echo   Files:
if exist "%BUILD_DIR%\MulanGeoWasm.html" echo     - MulanGeoWasm.html
if exist "%BUILD_DIR%\MulanGeoWasm.js"   echo     - MulanGeoWasm.js
if exist "%BUILD_DIR%\MulanGeoWasm.wasm" echo     - MulanGeoWasm.wasm
echo.
echo   To run locally (requires a local HTTP server, e.g. Python):
echo     cd "%BUILD_DIR%"
echo     python -m http.server 8080
echo     then open http://localhost:8080/MulanGeoWasm.html

endlocal