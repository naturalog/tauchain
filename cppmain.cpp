int main ( int argc, char** argv){
	(void)argv;
	assert(argc == 1);
	dict.init();
	cppdict_init();
	cpppred_state state;
	int entry=0;
	do
	{
		entry=cppout_query(state,entry);
		if(entry == -1){ dout << "done" << endl; break;}
	}
	while(true);
}

