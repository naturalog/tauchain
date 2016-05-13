#include <cstring>
#include <deque>
#include <functional>
#include <map>
#include <forward_list>
using namespace std;

template<typename T>
class btree						{
protected						:
	T v						;
	btree<T> *r, *l, *u				;
	deque<btree<T>*> rev				;
	deque<pair<btree<T>*, btree<T>*>> corev		;

	btree<T>* root()				{
		return	u				?
			u->u				?
			root(u)				:
			u				:
			this				;}

public							:
	btree(T v,btree<T> *r=0,btree<T> *l=0,btree<T> *u=0)
		: v(v), r(r), l(l), u(u)		{}

	typedef function<bool(btree<T>*)> visitor	;
	typedef function<bool(btree<T>*, btree<T>*)> covisitor;

	bool visit(visitor v)				{
		return	(!r||r->visit()	)		&&
			v(this)				&&
			(!l||l->visit())		;}

	bool covisit(covisitor, btree<T>*)		{
		return  t && !r == !t->r && !l == !t->l	&&
			(!r || r->covisit(v, t->r))	&&
			v(this, t)			&&
			(!l || l->covisit(v, t->r))	;}

	bool revisit(visitor v)				{
		return  rev.empty()			||
			v(rev.front())			&&
			rev.pop_front()			,
			revisit(v)			;}

	bool revisit(covisitor v)			{
		return  cor.empty()			||
			v(cor.front().first		,
			  cor.front().second)		&&
			cor.pop_front()			,
			revisit(v)			;}

	void revisit(btree<T> *x, btree<T> *y = 0)	{
		root()->(
			y				?
			cor.emplace_back(x, y)		:
			rev.emplace_back(x))		;}
							};


struct term_						{
	const wchar_t *n				;
	void *g						; // scope
	bool isvar					;
	bool operator==(const term_&) const		{
		return	isvar				?
			x.isvar 			&&
			!memcmp(this, &x, sizeof(var)	:
			!x.isvar			&&
			!memcmp(this, &x, sizeof(term_));}
							};


struct var						: 
	public term_					{
	unsigned deb					; // ruijn
	class term *t					;
							};


class term						:
	public btree<term_base*>			{
	typedef function<bool(term*, term*)> addcond	;
	typedef multimap<term*, term*> rawconds		;

	inline bool var() const { return v->isvar; }
	bool get_rawconds(term *x, rawconds *r, uuf<int> *u);
public							:
}

bool term::get_rawconds(term *x, rawconds *r, uuf<int> *u) {
	return	covisit([r](btree<T>* x, btree<T>* y) mutable {
		return	x->var() && y->var()		?
			u.merge(x, y)			:
			x->var() || y->var()		?
			r->emplace(x, y), true 		:
			x->v == y->v			&&
			!x->l == !y->l			&&
			!x->r == !y->r			;
							},
		x					);
};
