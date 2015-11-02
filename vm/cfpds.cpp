#include <functional>
#include <utility>
#include <iostream>
using namespace std;
// Conchon&Filliatre persistent data structures from "A Persistent Union-Find
// Data Structure" published on "Proceeding ML '07 Proceedings of the 2007
// workshop on Workshop on ML" pp. 37-46 
// https://www.lri.fr/~filliatr/ftp/publis/puf-wml07.pdf
struct cfpds {
	template<typename alpha>
	class arr {
		int n;
		function<alpha(int)> f;
	public:
		arr(int _n, function<alpha(int)> _f) : n(_n), f(_f) {}
		alpha		get(int k) { return f(k); }
		arr<alpha>*	set(int k, alpha aa) {
			return new arr<alpha>(n, [this, k, aa](int n) {
					return n == k ? aa : f(n);
				});
		}

		pair<arr<int>*, int> find_aux(int i) {
			int fi = get(i);
			if (fi == i) return make_pair(this, i);
			pair<arr<int>*, int> fr = find_aux(fi);
			return make_pair(fr.first->set(i, fr.second), fr.second);
		}
		void print() {
			for (int k = 0; k < n; ++k)
				cout << k << '\t' << f(k) << endl;
		}
	};
	struct uf {
		arr<int> *rank, *rep;
		uf(int n): rank(new arr<int>(n,[](int){return 0;})), rep(new arr<int>(n,[](int n){return n;})){}
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
