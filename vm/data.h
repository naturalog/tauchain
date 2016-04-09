#ifndef __DATA_H__
#define __DATA_H__
template<typename T>
struct parr { // persistent array
	const T& get(unsigned);
	parr<T>* set(unsigned, const T&);
	void reroot();
};
// persistent dsds of ints
struct puf : private pair<parr<int>, parr<unsigned>> {
	int find(int);
	puf* unio(int, int); // returns 0 for const1=const2
};
#endif
