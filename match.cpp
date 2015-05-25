#define BOOST_COMPUTE_DEBUG_KERNEL_COMPILATION
#define BOOST_COMPUTE_HAVE_THREAD_LOCAL
#define BOOST_COMPUTE_THREAD_SAFE

#include <boost/compute/core.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/types/builtin.hpp>
#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/program.hpp>
#include <string>
#include "prover.h"
#include "match.h"

namespace compute = boost::compute;
using namespace prover;
using compute::int4_;

std::string prog = 
	"#define get(x) terms[(((int)x)-((int)head))/sizeof(int4)]"
	"bool maybe_unify(local const int4& s, local const int4& d, global const int4* head) {"
	"	if (s.x < 0 || d.x < 0) return true;"
	"	if (!(s.x == d.x && !s.y == !d.y && !s.z == !d.z)) return false;"
	"	if (!s.y) return true;"
	"	local const term* ss = get(s.y), so = get(s.z), ds = get(d.y), dob = get(d.z);"
	"	return maybe_unify(ss, ds) && maybe_unify(so, dob);"
	"}"
	"kernel void match(global int4* terms, local int4 s, constant int head, char* out) {"
	"	int gid = get_global_id(0);"
	"	const int4 d = terms[gid];"
	"	if (maybe_unify(s, d, terms, head))"
	"		out[gid/8] |= 1 << (gid % 8);"
	"}";

compute::device device = compute::system::default_device();
compute::context context(device);
compute::program p = boost::compute::program::create_with_source(prog, context);
compute::kernel kmatch;

void init_cl() {
}
/*
int4_ t2v(const term& t) {
	return int4_((int)t.p, (int)t.s, (int)t.o, (int)t.node);
}

compute::command_queue& get_clq() {
	std::cout << "default cl device: " << device.name() << std::endl;
	return compute::command_queue(context, device);
}

compute::vector<int4_> clterms(const ruleset& rs, compute::command_queue& q) {
	compute::vector<cl_int4> res;
	compute::forward_list<int4_> host;
	for (auto r : rs) {
		const term* t = r->p;
		host.emplace_front(t->p, t->s, t->o);
	}
	compute::copy(host.begin(), host.end(), res, q);
}

match(const term& t, compute::vector<int4_> v, compute::command_queue& q, char* out) {
	memset(out, 0, v.size() / 8);
	int4_ tt = { t.p, t.s, t.p, p };
	kmatch.set_args(v, tt, 
}*/
