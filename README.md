# tauchain

Under development. For more information about tauchain visit http://idni.org  

To run example: "./tau < examples/socrates"  

For verbose print: "./tau --level 100 < examples/socrates"  

deps:  
apt-get install libboost-system-dev libboost-filesystem-dev libcurl4-openssl-dev   
todo:not all necessary, update  

C++ Compiler has to support C++11, gcc-4.9 and clang++-3.6 are known to work.  

Building: run make. You can also use cmake, but the hand-written Makefile is the primary method.  

For building with libmarpa / with n3 parser:  
sudo apt-get install libboost-regex-dev  

For building libmarpa dist, run build-libmarpa.sh
