int main ( int argc, char** argv){
	dict.init();
	cppdict_init();
	int n = 0;
	cpppred_state state;
	do
	{
		query(state);
		if(state.entry == -1){ dout << "done" << endl; break;}
		dout << "result " << n << endl;
		for(Thing t:state.locals)
		{
		        if (is_bound(t))
		        {
		        	dout << "?";
		        	Thing *v = getValue(&t);
		        	if (is_node(*v))
                			dout << " = " << cppdict[v->node];
                		dout << endl;
                	}
		}               
	}
	while(true);
}

