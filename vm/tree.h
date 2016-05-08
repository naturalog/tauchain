#include <deque>
#include <functional>
using namespace std;
template<typename T>
class btree {
	T v;
	btree<T> *r, *l, *u;
	deque<btree<T>*> rev;
	deque<pair<btree<T>*, btree<T>*>> corev;
//	map<btree<T>*, unsigned> pos;
	btree<T>* root();
public:
	btree(T v, btree<T> *r = 0, btree<T> *l = 0, btree<T> *u = 0)
		: v(v), r(r), l(l), u(u) { }
	typedef function<bool(btree<T>*)> visitor;
	typedef function<bool(btree<T>*, btree<T>*)> covisitor;

	bool visit(visitor);
	bool covisit(covisitor, btree<T>*);

	bool revisit(visitor);
	bool corevisit(covisitor);

	void revisit(btree<T>*);
	void corevisit(btree<T>*, btree<T>*);
};

template<typename T>
btree<T>* btree<T>::root() {
	btree<T> *x = u;
	while (x->u) x = x->u;
	return x;
}

template<typename T>
bool btree<T>::visit(visitor v) { 
	return (!r || r->visit()) && v(this) && (!l || l->visit());
}

template<typename T>
bool btree<T>::revisit(visitor v) {
	while (!rev.empty()) {
		btree<T>* t = rev.front();
		rev.pop_front();
		if (!v(t)) break;
	}
	return rev.empty();
}

template<typename T>
bool btree<T>::corevisit(covisitor v) {
	while (!corev.empty()) {
		auto t = corev.front();
		corev.pop_front();
		if (!v(t.first, t.second)) break;
	}
	return corev.empty();
}

template<typename T>
void btree<T>::revisit(btree<T>* t) {
	root()->rev.push_back(t);
}

template<typename T>
void btree<T>::corevisit(btree<T>* x, btree<T>* y) {
	root()->corev.emplace_back(x, y);
}

template<typename T>
bool btree<T>::covisit(covisitor v, btree<T> *t) {
	return  !r == !t->r && !l == !t->l 	&&
		(!r || r->covisit(v, t->r))	&&
		v(this, t)			&&
		(!l || l->covisit(v, t->r));
}






