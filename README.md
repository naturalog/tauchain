# tauchain

Under development. For more information about tauchain visit http://idni.org

To run example: "./tau < examples/socrates"

For verbose print: "./tau --level 100 < examples/socrates"

deps:
apt-get install libboost-system-dev libboost-filesystem-dev libcurl4-openssl-dev

C++ Compiler has to support C++11.

Building: run make. You can also use cmake, but the hand-written Makefile is the primary method.

For building with marpa:
sudo apt-get install libboost-regex-dev

For building marpa from scratch:
sudo apt-get install texinfo autoconf libtool cwebx 
then:
	git submodule init
	git submodule update
	cd libmarpa;	make dist;	cd dist;	./configure;	make
