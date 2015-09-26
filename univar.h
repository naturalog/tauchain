class prover
{
public:
old::prover p;
prover ( old::qdb qkb, bool check_consistency = false);
void query(const qdb& goal, subs * s = 0);
};
