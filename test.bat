call node --version
call npm --version
SET GYP_MSVS_VERSION=2013
SET PATH=%PATH%;%APPDATA%\npm
SET PATH=%PATH%;C:\Program Files (x86)\Git\bin
SET PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin
call npm install https://github.com/mongodb-js/node-pre-gyp/archive/v0.6.5-appveyor.tar.gz
call npm install https://github.com/mongodb-js/node-gyp/archive/v1.04-appveyor.tar.gz
call npm install --build-from-source
call npm test
