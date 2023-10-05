@echo off
set "apikey=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"
set "repo=CommonLib"
set "github_api_version=2022-11-28"
set "libname=common"

curl -L -H "Accept: application/vnd.github+json" -H "Authorization: Bearer %apikey%" -H "X-GitHub-Api-Version: %github_api_version%" --output "%libname%.zip" https://api.github.com/repos/KuroShinigami318/%repo%/zipball/v2.0.1
tar -xf %libname%.zip

rem hard code here
rename KuroShinigami318-CommonLib-1675a36544f35b2b891041a722a7d85c1be95b78 %libname%
if exist include rmdir include /s /q
mkdir include
move %libname% include/%libname%
del %libname%.zip