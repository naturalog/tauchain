int main ( int argc, char** argv){
	dict.init();
	int n = 0;
	cpppred_state state;
	do
	{
		query(state);
		if(state.entry == -1) break;
		dout << "result " << n << endl;
		for(Thing t:state.locals)
		{
		        if (is_bound(t))
                		dout << "? = " << str(getValue(&t)) << endl;
		}               
	}
	while(true);
}

