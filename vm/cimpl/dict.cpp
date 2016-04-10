#include <wchar.h>
#include <set>
using namespace std;

struct wccmp {
	bool operator()(const wchar_t* x, const wchar_t* y) const { return x ? wcscmp(x, y) < 0 : y ? true : false; }
};
extern "C"{
const wchar_t* getstr(const wchar_t *s);
int _main(/*int argc, char** argv*/);
}

const wchar_t* getstr(const wchar_t *s) {
	static set<const wchar_t*, wccmp> ss;
	static set<const wchar_t*, wccmp>::iterator it;
	it = ss.find(s);
	return (it == ss.end()) ? ss.insert(s = wcsdup(s)), s : *it;
}
int main(int, char**) { return _main(/*argc, argv*/); }
