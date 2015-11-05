#include <functional>
#include <utility>
#include <iostream>
#include <cassert>
using namespace std;
// Conchon&Filliatre persistent data structures from "A Persistent Union-Find
// Data Structure" published on "Proceeding ML '07 Proceedings of the 2007
// workshop on Workshop on ML" pp. 37-46 
// https://www.lri.fr/~filliatr/ftp/publis/puf-wml07.pdf
struct cfpds {
	template<typename alpha>
	class arr {
		struct arr_t {
			int n;
			function<alpha(int)> f;
			void set(int m, int k) {
				auto ff = f;
				f = [ff, m, k](int n) { return n == m ? k : ff(n); };
			}
			alpha operator()(int k) { return f(k); }
		};
		struct diff_t {
			int i;
			alpha v;
			arr<alpha> *t;
		};

		bool isdiff;
//		union {
			arr_t a;
			diff_t d;
//		};	
	public:
		arr(int n, function<alpha(int)> f) : isdiff(false) { a.n = n; a.f = f; }
		arr(int i, alpha v, arr<alpha> *t) : isdiff(true) { d.i = i; d.v = v; d.t = t; }
/*		
		alpha		get(int k) { return a.f(k); }
		arr<alpha>*	set(int k, alpha aa) {
			auto f = a.f;
			int n = a.n;
			return new arr<alpha>(a.n, [n, f, k, aa](int n) {
					return n == k ? aa : f(n);
				});
		}
*/
		alpha get(int i) {
			if (!isdiff) return assert(i < a.n), a(i);
			return d.i == i ? d.v : d.t->get(i);
		}
		static void set(arr<alpha> **res, int i, alpha v) {
			arr<alpha>& t = **res;
			if (t.isdiff) {
				*res = new arr<alpha>(i, v, &t);
				return;
			}
			arr_t &a = t.a, &n = *res;
			alpha old = a(i);
			a.set(i, v);

//			return (isdiff = true, d.i = i, d.v = v), (d.t = new arr<alpha>(i, v, this));
		}


		pair<arr<int>*, int> find_aux(int i) {
			int fi = get(i);
			if (fi == i) return make_pair(this, i);
			pair<arr<int>*, int> fr = find_aux(fi);
			return make_pair(fr.first->set(i, fr.second), fr.second);
		}
		void print() { for (int k = 0; k < a.n; ++k) cout << k << '\t' << get(k) << endl; }
	};
	struct uf {
		arr<int> *rank, *rep;
		uf(int n)
			: rank(new arr<int>(n,[](int  ){return 0;}))
			, rep (new arr<int>(n,[](int n){return n;})) {}
		uf(uf* u, arr<int>* reps) : rank(u->rank), rep(reps) {}
		uf(arr<int>* ranks, arr<int>* reps) : rank(ranks), rep(reps) {}
		int find(int x) {
			pair<arr<int>*, int> fcx = rep->find_aux(x);
			return (rep = fcx.first), fcx.second;
		}
		uf* unio(int x, int y) {
			int cx = find(x), cy = find(y);
			if (cx == cy) return this;
			int rx = rank->get(cx), ry = rank->get(cy);
			if (rx > ry) return new uf(this, rep->set(cy, cx));
			if (rx < ry) return new uf(this, rep->set(cx, cy));
			return new uf(rank->set(cx, rx+1), rep->set(cy, cx));
		}
		void print() { cout << "ranks:"<<endl; rank->print(); cout << "reps: "<<endl; rep->print(); }
	};
};

int main() {
	cfpds::uf *uf = new cfpds::uf(4);
	uf->print();
	cout << endl;
	uf = uf->unio(1, 2);
	uf = uf->unio(2, 3);
	uf = uf->unio(3, 1);
	uf->print();
}
