@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

rem ====== 配置服务路径和目录 ======
set "EXE_1=C:\Users\段伟\source\repos\ChatServer_1\x64\Debug\ChatServer.exe"
set "DIR_1=C:\Users\段伟\source\repos\ChatServer_1"

set "EXE_2=C:\Users\段伟\source\repos\ChatServer_2\x64\Debug\ChatServer.exe"
set "DIR_2=C:\Users\段伟\source\repos\ChatServer_2"

set "EXE_3=C:\Users\段伟\source\repos\GateServer\x64\Debug\GateServer.exe"
set "DIR_3=C:\Users\段伟\source\repos\GateServer"

set "EXE_4=C:\Users\段伟\source\repos\resServer\x64\Debug\day26-multithread-res-server.exe"
set "DIR_4=C:\Users\段伟\source\repos\resServer"

set "EXE_5=C:\Users\段伟\source\repos\StatusServer\x64\Debug\StatusServer.exe"
set "DIR_5=C:\Users\段伟\source\repos\StatusServer"

rem ====== 构建 wt 命令 ======
set "WT_EXE=%LocalAppData%\Microsoft\WindowsApps\wt.exe"
set "WT_CMD="

for %%I in (1 2 3 4 5) do (
    set "exe=!EXE_%%I!"
    set "dir=!DIR_%%I!"

    if exist !exe! (
        echo  已找到：!exe!
        for %%A in (!exe!) do set "name=%%~nA"

        rem  注意：此处不加多余引号或转义
        set "WT_CMD=!WT_CMD! new-tab -d !dir! --title !name! powershell -NoExit -Command .\\x64\\Debug\\!name!.exe ;"
    ) else (
        echo  未找到文件：!exe!
    )
)

rem ====== 启动 Windows Terminal ======
echo.
echo  正在启动多个服务...
start "" "%WT_EXE%" !WT_CMD!
