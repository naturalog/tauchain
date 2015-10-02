
#include "prover.h" // namespace "old"

class yprover
{
public:
old::prover *p;
old::qdb results;
yprover ( old::qdb qkb, bool check_consistency = false);
void query(const old::qdb& goal);
};
