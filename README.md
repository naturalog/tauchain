# tauchain

Under development. For more information about tauchain visit http://idni.org

To run example: "./tau < examples/socrates"

For verbose print: "./tau --level 100 < examples/socrates"

deps:
apt-get install libboost-system-dev libboost-filesystem-dev libcurl4-openssl-dev

you will need a fairly new gcc, probably 4.9

building: just run make. you can also use cmake, but the hand-written Makefile is the primary method.

for building with marpa:
sudo apt-get install libboost-regex-dev

for building marpa from scratch:
this is so the whole marpa dist with docs can be made..not sure if thats necessary..but it works
sudo apt-get install texinfo autoconf libtool cwebx 
then:
	git submodule init
	git submodule update
	cd libmarpa;	make dist;	cd dist;	./configure;	make
