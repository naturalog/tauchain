int main ( int argc, char** argv){
	(void)argv;
	assert(argc == 1);
	dict.init();
	cppdict_init();
	cpppred_state state;
	do
	{
		query(state);
		if(state.entry == -1){ dout << "done" << endl; break;}
	}
	while(true);
}

