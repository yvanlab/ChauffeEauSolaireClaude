@echo off
REM ========================================
REM ESP32 Solar Spa Controller - Simulator
REM ========================================

echo.
echo ========================================
echo   ESP32 Spa Controller - Simulator
echo ========================================
echo.

REM Change to the script directory
cd /d "%~dp0"

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python n'est pas installe ou n'est pas dans le PATH
    echo.
    echo Installez Python depuis: https://www.python.org/downloads/
    echo.
    pause
    exit /b 1
)

REM Check if web_simulator.py exists
if not exist "web_simulator.py" (
    echo ERROR: web_simulator.py introuvable
    echo Assurez-vous d'etre dans le bon repertoire.
    echo.
    pause
    exit /b 1
)

REM Check if data/index.html exists
if not exist "data\index.html" (
    echo ATTENTION: data\index.html introuvable
    echo L'interface web ne pourra pas etre chargee.
    echo.
)

echo Lancement du simulateur...
echo.
echo Une fois demarre, ouvrez votre navigateur a:
echo   http://localhost:8080
echo.
echo Appuyez sur Ctrl+C pour arreter le simulateur
echo.
echo ========================================
echo.

REM Launch the simulator
python web_simulator.py

REM If the simulator exits, pause so user can see any error messages
echo.
echo Simulateur arrete.
pause
