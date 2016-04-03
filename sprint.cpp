//region sprint

//Thing::Serializer::Decorated
//Var::Serializer::Decorated
string sprintVar(string label, Thing *v){
	stringstream wss;
	wss << label << ": (" << v << ")" << str(v);
	return wss.str();
}

//Pred::Serializer::Decorated
string sprintPred(string label, nodeid pred){
	stringstream wss;
	wss << label << ": (" << pred << ")" << dict[pred];
	return wss.str();
}

//Thing::Serializer::Decorated
string sprintThing(string label, Thing *t){
	stringstream wss;
	wss << label << ": [" << t << "]" << str(t);
	return wss.str();
}


string sprintSrcDst(Thing *Ds, Thing *s, Thing *Do, Thing *o){
	stringstream wss;
	wss << sprintThing("Ds", Ds) << ", " << sprintThing("s",s) << endl;
	wss << sprintThing("Do", Do) << ", " << sprintThing("o",o);
	return wss.str();
}


//endregion
