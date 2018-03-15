
del Staging\*.exe
del Staging\*.dll
del Staging\*.wav
copy sample\input.wav Staging
copy libsndfile\bin64\libsndfile-1.dll Staging
copy /Y libsndfile\lib64\libsndfile-1.lib .
g++ -std=c++17 FileUpsampler.cpp libsndfile-1.lib -o Staging\FileUpsampler.exe
del libsndfile-1.lib
Staging\FileUpsampler.exe Staging\input.wav Staging\output.wav