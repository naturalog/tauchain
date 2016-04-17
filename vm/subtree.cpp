#include <iostream>
#include <map>
using namespace std;
struct term { int p, sz; term* args; };
typedef pair<term*, term*> ppterm;
template<typename T>
struct plist { // persistent list
	plist() {}
	struct element {
		element() {}
		element(const T& t) : t(t), isdiff(false) {}
		bool isdiff;
		T t;
		union {
			element *n; // next in list
			element *p; // prev version
		};
		element* next() { return isdiff ? p->n : n; }
	} head;
	plist(element h) : head(h) {}
	struct iterator {
		element* e;
		iterator(element* e = 0) : e(e) {}
		operator const T&() { return e->t; }
		operator bool() { return e; }
		iterator next() { return iterator(e->next()); }
	};
	iterator begin() { return iterator(&head); }
	iterator end() { return iterator(0); }
	plist* push_front(const T& tt) {
		plist *res = new plist(element(tt));
		res->head.n = &head;
	};
};
map<ppterm, plist<ppterm*>> ir;

void testplist() {
	auto *l = new plist<int>;
	auto p = [](plist<int>& l) {
		for (auto it = l.begin(); it; it = it.next())
			cout << (int)it << endl; cout << endl;
	};
	p(*l); l = l->push_front(1);
	p(*l); l = l->push_front(2);
	p(*l); l = l->push_front(3);
	p(*l); l = l->push_front(4);
	p(*l); l = l->push_front(5);

};

int main() {
	testplist();
	return 0;
}
