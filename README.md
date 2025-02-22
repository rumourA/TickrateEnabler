# TickrateEnabler

Adds the -tickrate launch option without sourcehooks or including the sdk

# Build example for linux 64bit

g++ -std=c++17 -fno-threadsafe-statics -fno-exceptions -fno-rtti -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-enforce-eh-specs -fno-extern-tls-init -fno-access-control -fnon-call-exceptions -fno-use-cxa-atexit -fno-gnu-unique -fno-stack-protector -fno-ident -fno-use-cxa-get-exception-ptr -fPIC main.cpp -o TickrateEnabler64.so -shared -m64

# Build example for windows 64bit

g++ -std=c++17 -fno-threadsafe-statics -fno-exceptions -fno-rtti -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-enforce-eh-specs -fno-extern-tls-init -fno-access-control -fnon-call-exceptions -fno-use-cxa-atexit -fno-gnu-unique -fno-stack-protector -fno-ident -fno-use-cxa-get-exception-ptr -fPIC -shared main.cpp -o TickrateEnabler64.dll -m64

or just compile in Visual studio or something
