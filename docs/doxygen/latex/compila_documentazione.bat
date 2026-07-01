@echo off
chcp 65001 >nul
echo ========================================
echo Rigenerazione Documentazione UNO
echo ========================================

echo.
echo [1/3] Rigenerazione HTML e LaTeX con Doxygen...
cd ..
doxygen Doxyfile
if errorlevel 1 (
    echo ERRORE: Doxygen ha fallito. Verificare che doxygen sia installato.
    pause
    exit /b 1
)

echo.
echo [2/3] Compilazione LaTeX con MiKTeX...
cd latex
"C:\Users\pasqu\AppData\Local\Programs\MiKTeX\miktex\bin\x64\pdflatex.exe" -interaction=nonstopmode refman.tex 2>&1 | findstr /C:"Error" /C:"Fatal" && (
    echo Tentativo di installazione pacchetti mancanti...
    "C:\Users\pasqu\AppData\Local\Programs\MiKTeX\miktex\bin\x64\mpm.exe" --install=multirow
    "C:\Users\pasqu\AppData\Local\Programs\MiKTeX\miktex\bin\x64\mpm.exe" --install=tabu
    "C:\Users\pasqu\AppData\Local\Programs\MiKTeX\miktex\bin\x64\mpm.exe" --install=hanging
    echo Re-compilazione...
)

"C:\Users\pasqu\AppData\Local\Programs\MiKTeX\miktex\bin\x64\pdflatex.exe" -interaction=nonstopmode refman.tex
"C:\Users\pasqu\AppData\Local\Programs\MiKTeX\miktex\bin\x64\pdflatex.exe" -interaction=nonstopmode refman.tex

echo.
echo [3/3] Verifica output...
if exist refman.pdf (
    echo.
    echo ========================================
    echo SUCCESSO! Il PDF e' stato generato.
    echo ========================================
    echo Apertura automatica...
    start "" "%~dp0refman.pdf"
) else (
    echo ERRORE: Il PDF non e' stato creato. Verificare refman.log.
)

echo.
pause