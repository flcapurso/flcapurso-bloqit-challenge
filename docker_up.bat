@echo off

if "%1"=="dev" (
    docker compose -f docker-compose.yml -f docker-compose.dev.yml up --build
    goto :eof
)

if "%1"=="prod" (
    docker compose -f docker-compose.yml -f docker-compose.prod.yml up --build
    goto :eof
)

if "%1"=="down" (
    docker compose down --remove-orphans
    goto :eof
)

echo Usage:
echo   script.bat dev
echo   script.bat prod
echo   script.bat down