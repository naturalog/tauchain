extern "C" {
#include "strings.h"
}
#include "containers.h"
#include <map>
#include <forward_list>
#include <list>
#include <set>
using std::ostream;
using std::string;
using std::endl;
using std::runtime_error;
using std::set;
using ostm = std::ostream;
extern ostream &dout;

typedef const char cch;
typedef cch* pcch;

// IR
struct crd;
struct ccmp { int operator()(const crd& x, const crd& y); };
typedef std::list<crd> word;
typedef const crd ccrd;
typedef const word cword;

struct crd {
	lary<int> c;
	pcch str;
	crd(pcch str = 0, uint len = 0) : c(len), str(str) {}
	crd(const crd& t) : c(t.c), str(t.str) {}
	crd(crd&& t) : c(t.c), str(t.str) {}
	crd& operator=(const crd& t) { return c = t.c, str = t.str, *this; }
};
//ostream& operator<<(ostream& os, const struct wrd& w);
//

struct ast {
	class term {
		pcch p;
		term **args;
	public:
		term(pcch v);
		term(term** _args, uint sz);
		term(const vector<term*>& a) : term(a.begin(), a.size()) {}
		~term();
		const uint sz;
		ostream& operator>>(ostream &os) const;
		void crds(word&, crd c = crd(), int k = -1) const;
	};
	struct rule {
		struct premise {
		        const term *t;
		        premise(const term *t);
			ostream &operator>>(ostream &os) const;
			};
		typedef vector<premise*> body_t;
	        const term *head;
		const body_t body;
	        rule(const term *h, const body_t &b = body_t());
	        rule(const term *h, const term *b);
	        rule(const rule &r);
		ostream &operator>>(ostream &os) const;
		word crds(int rl);
	};
	vector<rule*> rules;
	vector<term*> terms;
};
typedef vector<ast::rule::premise*> body_t;
typedef const ast::term cterm;
void readdoc(bool query, ast *st);
//ostream& operator<<(ostream& os, const crd& c);
//ostream& operator<<(ostream& os, const word& w);
void push_front(word& t, int i);
