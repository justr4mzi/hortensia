@echo off
setlocal
cd /d "%~dp0"
set "EDEN_NX2_SVC_TRACE=1"
set "EDEN_NX2_MEM_TRACE=1"
set "EDEN_NX2_IPC_TRACE=1"
echo Orchid 0.9.0 Xenoblade Test 1 - diagnostics
echo Reproduce the problem, close Orchid, then save the log before restarting.
"%~dp0orchid.exe" %*
endlocal
