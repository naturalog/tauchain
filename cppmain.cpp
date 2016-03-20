int main ( int argc, char** argv){
	for (int x = 0; x < argc - 1; x++)
		if (0==strcmp(argv[x+1], "silent"))
			silent = true;
	(void)argv;
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
	
	
#ifdef getValue_profile
dout << "getValue_BOUNDS = " << getValue_BOUNDS << "  getValue_OFFSETS = " 
<< getValue_OFFSETS << "  getValue_OTHERS = " << getValue_OTHERS << endl;
#endif

	
}

