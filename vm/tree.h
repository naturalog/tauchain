#include <cstring>
#include <deque>
#include <functional>
#include <map>
using namespace std;

template<typename T>
class btree {
protected:
	T v;
	btree<T> *r, *l, *u;
	deque<btree<T>*> rev;
	deque<pair<btree<T>*, btree<T>*>> corev;
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
	return  t && !r == !t->r && !l == !t->l &&
		(!r || r->covisit(v, t->r))	&&
		v(this, t)			&&
		(!l || l->covisit(v, t->r));
}

struct term_base {
	const wchar_t *n;
	void *g; // scope
	bool isvar;
	bool operator==(const term_base&) const;
};

struct var : public term_base {
	unsigned deb; // ruijn
	class term *t;
};

bool term_base::operator==(const term_base& x) const {
	if (isvar) {
		if (!x.isvar) return false;
		return !memcmp(this, &x, sizeof(var));
	}
	if (x.isvar) return false;
	return !memcmp(this, &x, sizeof(term_base));
}

#define newitem(x) new item(x)

template<typename T>
struct uf {
	struct item {
		T* t;
		item *rep;
		set<item*> next;
		item(const T& t) : t(t), rep(0) {}
	};
	map<T, item*> m;
	T rep(const T& t) {
		auto it = m.find(t);
		if (it == m.end()) return m[t] = newitem(t), t;
		return rep(it->second)->t;
	}
private:
	item *rep(const item *i) {
		return  !i->rep		? i	:
			!i->rep->rep	? i->rep:
			(m[i->rep->t] = i->rep = rep(i->rep));
	}
};

template<typename T>
struct uuf : private uf<T> { // unification union find
};

class term : public btree<term_base*> {
	typedef function<bool(term*, term*)> addcond;
	typedef multimap<term*, term*> rawconds;

	inline bool var() const { return v->isvar; }

	bool get_rawconds(term *x, rawconds *r, uuf<int> *u) {
		return covisit([r](btree<T>* x, btree<T>* y) mutable {
			return	(x->var() && y->var())	?
				u.merge(x, y)		:
				(x->var() || y->var())	?
				r->emplace(x, y), true 	:
				(x->v == y->v && 
				!x->l == !y->l && 
				!x->r == !y->r);
		}, x);
	}
public:
}
