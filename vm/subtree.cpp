#include <iostream>
#include <map>
using namespace std;
struct term { int p, sz; term* args; };
typedef pair<term*, term*> ppterm;
template<typename T>
struct plist { // persistent list
	struct element {
		element(const T& t = T(), const element *next = 0)
			: t(t), next(next) {}
		element(const element& e, const element *next = 0)
			: element(e.t, next) {}
		const T t;
		const element *next;
	};
	const element head;
	const unsigned len;
	plist() : len(0) {}
	const plist* push_front(const T& tt) const {
		return new plist(element(tt), &head, len);
	}
private:	
	plist(element h, const element *next = 0, unsigned len = 0)
		: len(len + 1), head(h, next) { }
};
map<ppterm, plist<ppterm*>> ir;

void testplist() {
	const auto *l = new plist<int>;
	auto p = [](const plist<int>& l) {
		auto pp = [](const plist<int>::element* e) {
			cout << e->t << ' ';
		};
		auto e = &l.head;
		do { pp(e); } while (e = e->next);
		cout << endl;
	};
	p(*l); l = l->push_front(1);
	p(*l); l = l->push_front(2);
	auto q = l;
	p(*l); l = l->push_front(3);
	p(*l); l = l->push_front(4);
	p(*l); l = l->push_front(5);
	p(*q);

};

int main() {
	testplist();
	return 0;
}
