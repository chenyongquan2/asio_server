# 1)Decription
a cpp server and client using asio

# 2)Perpare 
you should install the xmake firstly, and make sure to add the xmake into your env path

# 3)How to bulild
## a)Config
xmake f -p windows -a x64 -m debug --toolchain=msvc -vyD
## b)Build
### case 1:build all target
xmake or xmake -ba
### case 2:build specialized target
xmake -b client  or xmake -b server

# 4)How to run
start these executor in order
## Step1:Start the server
cd and run the server by cmd: ${workspaceFolder}/build/windows/x64/debug/client.exe
## Step2:Start the server
cd and run the server by cmd: ${workspaceFolder}/build/windows/x64/debug/server.exe