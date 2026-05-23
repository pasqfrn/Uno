@echo off
title Avvio Chat C-Grafica
echo [1/3] Compilazione del server C con Mongoose...
gcc server.c mongoose.c -o chat_server.exe -lws2_32
if %errorlevel% neq 0 (
    echo ERRORE: Assicurati di avere mongoose.c e mongoose.h nella cartella!
    pause
    exit
)
echo [2/3] Avvio del server...
start chat_server.exe
timeout /t 2
echo [3/3] Apertura interfaccia grafica...
start http://localhost:8888
echo TUTTO PRONTO! Digita nella pagina web che si e' aperta.
exit