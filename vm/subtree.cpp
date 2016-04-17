#include <iostream>
#include <map>
using namespace std;
struct term { int p, sz; term* args; };
typedef pair<term*, term*> ppterm;
template<typename T>
struct plist { // persistent list
	plist() {}
	struct element {
		element(const T& t = T(), element *next = 0) : t(t), next(next) {}
		T t;
		element *next;
	} head;
	plist(element h) : head(h) {}
	plist* push_front(const T& tt) {
		plist *res = new plist(element(tt));
		res->head.next = &head;
		return res;
	};
};
map<ppterm, plist<ppterm*>> ir;

void testplist() {
	auto *l = new plist<int>;
	auto p = [](plist<int>& l) {
		auto pp = [](plist<int>::element* e) {
			cout << e->t << ' ';
		};
		auto e = &l.head;
		do { pp(e); } while (e = e->next);
		cout << endl;
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
