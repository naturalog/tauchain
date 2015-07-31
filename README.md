# tauchain

Under development. For more information about tauchain visit http://idni.org  

To run example: "./tau < examples/socrates"  

For verbose print: "./tau --level 100 < examples/socrates"  

deps:  
apt-get install libboost-system-dev libboost-filesystem-dev libcurl4-openssl-dev   
todo:not all necessary, update  

C++ Compiler has to support C++11, gcc-4.9 and clang++-3.6 are known to work.  

Building: run make. You can also use cmake, but the hand-written Makefile is the primary method.  

For building with libmarpa:  
sudo apt-get install libboost-regex-dev  

For building libmarpa from scratch:   
this is so that the whole marpa dist with docs can be made..not sure if thats necessary..but it works    
debian and ubuntu now contain "R2", a version of libmamarpa, if we could use it remains to be investigated.  
sudo apt-get install texinfo autoconf libtool cwebx  
then:  
	git submodule init  
	git submodule update  
	cd libmarpa;	make dist;	cd dist;	./configure;	make  
	
