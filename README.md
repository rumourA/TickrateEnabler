# TickrateEnabler

Adds the -tickrate launch option without sourcehooks or including the sdk

# Build example for linux 64bit

g++ -std=c++17 main.cpp -o TickrateEnabler64.so -fPIC -s -shared -m64
-m32 for 32 bit builds


# Build example for windows 64bit

g++ -std=c++17 main.cpp -o TickrateEnabler64.dll -fPIC -s -shared -m64
-m32 for 32 bit builds


or just compile in Visual studio or something
