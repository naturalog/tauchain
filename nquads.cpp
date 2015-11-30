#ifndef NOPARSER
#include <string>
#include <iostream>
#include <set>
#include <stdexcept>
#include "rdf.h"
#include <boost/algorithm/string.hpp>
#include "jsonld.h"
#include "misc.h"
using namespace boost::algorithm;


nqparser::nqparser() : t(new wchar_t[4096*4096]) { }
nqparser::~nqparser() { delete[] t; }


/*
iswspace: 
    space 		0x20, ' '
    form feed 		0x0c, '\f'
    line feed 		0x0a, '\n'
    carriage return 	0x0d, '\r'
    horizontal tab 	0x09, '\t'
    vertical tab 	0x0b, '\v' 
*/

//should be 'nqparser::readcontext'
//ws*,{~'{'},return
//ws*,'{',ws*,'}',return
//ws*,'{',ws*,{~'}'}, << (*this)(s,*r) >>
pnode nqparser::readcurly() {
	setproc(L"readcurly");
	//Skip any white-space at the beginning of the line:
	while (iswspace(*s)) ++s;

	//If the next char is not L'{' then this is not a syntactically
	//correct context, return "(pnode)0".
	if (*s != '{') return (pnode)0;
	++s;

	//Skip any white-space at the beginning of the context:
	while (iswspace(*s)) ++s;

	//**needs comments**
	//Create a unique blank-node ID for this context
	auto r = gen_bnode_id();

	//If the next character is L'}' then move the string-pointer past L'}',
	//make a bnode for r, and return this.
	if (*s == L'}') { ++s; return mkbnode(r); }

	//**needs comments**
	//Otherwise: nqparser::nqparser().
	//This needs to be fixed
	//This says that any string that the parser would accept as valid syntax
	//can be placed inside a curly, and this new string will also be accepted
	//as valid syntax.
	auto t = (*this)(s,*r);//whats going on here?;

	return mkbnode(r);
}


// {~L'('}, return
// L'(',ws*,L')',return

//(ws*,{~L')'},readany_success,ws*,((L'.',ws*)|{~(L'.' | L'}'}),{~L')'})* as X

// L'(',ws*,{~L')'},X,ws*,L')',break,ws*
// L'(',ws*,{~L')'},X,ws*,{~L')'},readany_error,error

// L'(',ws*,{~L')'},X,ws*,{~L')'},readany_success,ws*,{L'}'},error
// L'(',ws*,{~L')'},X,ws*,{~L')'},readany_success,ws*,((L'.',ws*)|{~(L'.' | L'}'}),{L')'},ws*

pnode nqparser::readlist() {
	setproc(L"readlist");

	//We don't skip over any white-space here?

	//If the first character is not L'(', then we assume it's not
	//a list and return "(pnode)0".
	if (*s != L'(') return (pnode)0;
	++s;

	//**needs comments**
	int lpos = 0;
	const string head = list_bnode_name();

	pnode pn;

	//Skip over any white-space
	while (iswspace(*s)) ++s;

	//If the next character is L')' then I guess we have
	//an empty list and we return RDF_NIL.
	if (*s == L')') {
		 ++s;
		return mkiri(RDF_NIL); 
	}

	do {
		//Skip over any white-space
		while (iswspace(*s)) ++s;

		//If the next character is L')' then we've reached the end
		//of the list, break.
		if (*s == L')') {
			break;
		}

		//Maybe not readany().
		//Attempt to readany(true) into pn. If we fail, throw an error.
		if (!(pn = readany(true)))
			//or literal apparently?
			throw wruntime_error(string(L"expected iri or bnode or list in list: ") + string(s,0,48));
		
		//**needs comments**
		pnode cons = mkbnode(pstr(list_bnode_name()));

		//**needs comments**
		lists.emplace_back(cons, mkiri(RDF_FIRST), pn);//then we create first triple?

		//**needs comments**
		_kb.second[head].push_back(pn);
		//qlists[head].push_back(pn);//and into qdb.second i presume we add the item too

		++lpos;

		//Skip over any white-space.
		while (iswspace(*s)) ++s;

		//**needs comments**
		if (*s == L')') lists.emplace_back(cons, mkiri(RDF_REST), mkiri(RDF_NIL));//end

		else lists.emplace_back(cons, mkiri(RDF_REST), mkbnode(pstr(list_bnode_name())));

		//**needs comments**
		//If the next character is L'.', then read that and skip over any white-space
		//following it.
		if (*s == L'.'){
			s++;
			while (iswspace(*s++));
		}

		//If the next character is L'}', then throw a run-time error.
		//**needs comments**
		if (*s == L'}') throw wruntime_error(string(L"expected { inside list: ") + string(s,0,48));

	}
	//Can we encounter an infinite loop here? Let's find out.
	while (*s != L')');//id be more worried about this endless loop :)
		

	//Skip over any white-space.
	do { ++s; } while (iswspace(*s));

	return mkbnode(pstr(head));
}



//(iswspace | L',' | L';' | L'.' | L'}' | L'{' | L')') as special

//iswspace*, 0, error
//iswspace*, {!0}, L'<',!(L'>')*,L'>', return
//iswspace*, {!0}, {!L'<'}, L"=>", return
//iswspace*, {!0}, {!L'<'}, !{L"=>"}, (~special)*, {L"true"}, return
//iswspace*, ..., (~special)*, {~(L"true")}, {L"false"}, return
//iswspace*, ..., (~special)*, {~(L"true" | L"false")}, {atoi}, return
//iswspace*, ..., (~special)*, {~(L"true" | L"false" | atoi)}, {atof}, return
//iswspace*, ..., (~special)*, {~(L"true" | L"false" | atoi | atof)}...other tokenization,
//		**the string-pointer has already advanced as far as it will go in
//		**readiri(), the rest is tokenization.
pnode nqparser::readiri() {
	setproc(L"readiri");
	//Skip any white-space at the beginning of the line:
	while (iswspace(*s)) ++s;


	//If *s==0 then fail. I.e. our char pointer has nothing to point to.
	if (*s == 0)
		throw std::runtime_error("iri expected");


	//If our next char is '<', then we're expecting an IRI
	if (*s == L'<') {
		while (*++s != L'>') 
		//until bracket, copy s to t
		//*note: increments 'pos' *after* storing '*s'.
		//**therefore "t[pos] = 0;" is not overwriting anything
			t[pos++] = *s;
		//append a null
		t[pos] = 0; 
		pos = 0;
		//Move pointer past the '>'.
		++s;
		return mkiri(wstrim(t));
	}


	//If the next two chars are '=>', then this is an implication.
	else if (*s == L'=' && *(s+1) == L'>') 
	{ 
		++++s; return mkiri(pimplication); 
	}
	

	//Okay so we didn't hit a '<' bracket, and we didn't get an implication, so
	//copy whatever else we've got from s into t until we hit either white-space or
	//one of these special chars: ",;.}{)". What about "("?	
	while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')') t[pos++] = *s++;
	t[pos] = 0; pos = 0;
	pstring iri = wstrim(t);


	//Check if the string you found represents one of these literal values:
	//Why literals when we're doing readiri?
	if (lower(*iri) == L"true")
		return mkliteral(pstr(L"true"), XSD_BOOLEAN, 0);
	if (lower(*iri) == L"false")
		return mkliteral(pstr(L"false"), XSD_BOOLEAN, 0);
	if (std::atoi(ws(*iri).c_str()))
		return mkliteral(iri, XSD_INTEGER, 0);
	if (std::atof(ws(*iri).c_str()))
		return mkliteral(iri, XSD_DOUBLE, 0);
	//??if (*iri == L"0") return mkliteral(iri, XSD_INTEGER, 0);


	//So we didn't determine it to be a literal. Check if the string contains ':',
	//implying that the substring coming before ':' is a prefix.
	auto i = iri->find(L':');
	
	//If we didn't find ':' then we'll have that "i == string::npos", and so
	//return mkiri() of the whole string.
	if (i == string::npos) return mkiri(iri);

	//Otherwise we did find ':', so we assume that the the substring coming before
	//this represents a prefix, and we'll copy this into 'p'.
	string p = iri->substr(0, ++i);

//	TRACE(dout<<"extracted prefix \"" << p <<L'\"'<< endl);

	//Now check if that prefix is contained in nqparser::prefixes.
	auto it = prefixes.find(p);
	if (it != prefixes.end()) {
		//If it is, then prepend the value in nqparser::prefixes to the
		//the remainder of the string after ':', and take this as the value
		//for 'iri'.
//		TRACE(dout<<"prefix: " << p << " subst: " << *it->second->value<<endl);
		iri = pstr(*it->second->value + iri->substr(i));
	}

	//Make the iri and return it.
	return mkiri(iri);
}




//(iswspace | L',' | L';' | L'.' | L'}' | L'{' | L')') as special.

//iswspace*, {~(L"_:")}, return
//iswspace*, L"_:",(~special)*, return.
pnode nqparser::readbnode() {
	setproc(L"readbnode");

	//Skip any white-space at the beginning of the line:
	while (iswspace(*s)) ++s;

	//If the next two characters are not exactly "_:" then apparently it is not
	//a syntactically correct blank node, and we return "pnode(0)".
	if (*s != L'_' || *(s+1) != L':') return pnode(0);
	//We don't need to advance s here because that will be taken care of in the while
	//loop. We can see that neither '_' nor ':' are any of the characters triggering
	//false in that conditional.

	//Copy characters from s into t until reaching either a whitespace character
	//or one of the special characters ",;.}{)".
	while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')') t[pos++] = *s++;

	//Append a null character to t and set pos back to 0.
	t[pos] = 0; pos = 0;

	//Make a blank node from the bnode identifier string parsed into t, and return
	//this node.
	return mkbnode(wstrim(t));
}





//iswspace*, <!(L'@')>, return
//iswspace*, L'@', <!(L"@prefix ")>, error
//iswspace*, L"@prefix ", (~(L':'|0|'\n'))*,(0|\n),error
//iswspace*, L"@prefix ", (~(L':'|0|'\n'))*, L':',(0|\n),error
//iswspace*, L"@prefix ", (~(L':'|0|'\n'))*, L':',~(0|\n), readiri()_error, error
//iswspace*, L"@prefix ", (~(L':'|0|'\n'))*, L':',~(0|\n), readiri()_success,(~('.'|'\n'|0))*,('.'|'\n'|0).
void nqparser::readprefix() {
	setproc(L"readprefix");

	//Skip any white-space at the beginning of the line:
	while (iswspace(*s)) ++s;

	//If the next char is not L'@'. Then it's not a '@prefix' tag, so return.
	if (*s != L'@') return;

	//Otherwise, check to see if the next 8 characters match L"@prefix ". If
	//not, then the appearance of L'@' is apparently syntactically incorrect
	//because we're throwing an error about it.
	if (memcmp(s, L"@prefix ", 8*sizeof(*s)))
			throw wruntime_error(string(L"\"@prefix \" expected: ") + string(s,0,48));

	//We didn't throw a run-time error, so the 8 characters must have matched L"@prefix ".
	//Advance the string-pointer by 8, i.e. past the L"@prefix ".
	s += 8;


	//Copy the string from s into t until reaching ":".		
	while (*s != L':' && *s!= 0 && *s!='\n') t[pos++] = *s++;
	if (!(*s!= 0 && *s!='\n'))
		throw wruntime_error(L"hm");

	t[pos++] = *s++;
	t[pos] = 0; pos = 0;
	
        if (!(*s!= 0 && *s!='\n'))
                throw wruntime_error(L"hm");

	//Trim the resulting string
	pstring tt = wstrim(t);
	TRACE(dout<<"reading prefix: \"" << *tt<<L'\"' << endl);

	//Expecting the next token to represent an IRI, so readiri and add it
	//to prefixes at key *tt.
	prefixes[*tt] = readiri();

	//Now that we've read the prefix, we're expecting to find a '.', so
	//just move our string-pointer past the '.'.
	while (*s != '.' && *s != '\n' && *s != 0) ++s;
	++s;
}




//(iswspace | L'?' | L',' | L';' | L'.' | L'}' | L'{' | L')') as special.
//(iswspace | L',' | L';' | L'.' | L'}' | L'{' | L')') as special2.

//iswspace*, {~L'?'},return
//iswspace*, L'?', (~special)*,{L'?'},error
//iswspace*, L'?', (~special)*,{special2},return.
pnode nqparser::readvar() {
	setproc(L"readvar");
	//Skip any white-space at the beginning of the line:
	while (iswspace(*s)) ++s;

	//If the next character is not L'?', then apparently it's not
	//a syntactically correct variable, so we return "pnode(0)"
	if (*s != L'?') return pnode(0);
	s++; 

	//Copy characters from s into t until reaching either a whitespace character
	//or one of the special characters ",;.}{)".
	while (!iswspace(*s) && *s != L'?' && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')') t[pos++] = *s++;
	if(*s == L'?') throw wruntime_error(string(L"bad variable name") + string(s,0,48));

	t[pos] = 0; pos = 0;

	//Make an IRI node for the var identifier string that we parsed into t, and return it.
	return mkiri(wstrim(t));
}



//(!iswspace | L',' | L';' | L'.' | L'}' | L'{' | L')') as special

//iswspace*, {~L'\"'}, return
//iswspace*, L'\"',((~(L'\\' | L'\"')) | (L'\\', $))*, L'\"',L"^^",L'<',(~L'>')*,L'>'
//iswspace*, L'\"',((~(L'\\' | L'\"')) | (L'\\', $))*, L'\"',L'@',(~iswspace)*
//iswspace*, L'\"',((~(L'\\' | L'\"')) | (L'\\', $))*, L'\"',~(special | "^^" | L'@'), error
pnode nqparser::readlit() {
	setproc(L"readlit");

	//Skip any white-space at the beginning of the line:
	while (iswspace(*s)) ++s;

	//If the next char found is not a L'\"', then it's apparently not a syntactically
	//correct literal, because we're returning 'pnode(0)'
	if (*s != L'\"') return pnode(0);

	//Move the string ponter past the L'\"'.
	++s;

	//Does this miss the string "\"\"" ?
	//Copy characters from s into t until you reach a L'\"' that isn't preceded
	//by a L'\\'. (Because \" is a literal quote character and not one of the
	//quotes delimiting the literal value).

	//do { t[pos++] = *s++; } while (!(*(s-1) != L'\\' && *s == L'\"'));

	while (!(*(s-1) != L'\\' && *s == L'\"')) t[pos++] = *s++;
	//Move the string-pointer past the L'\"'. 
	++s;

	string dt, lang;


//what loop?

	//As long as the next character is not whitespace and not one of the special
	//characters ",;.}{)" then:
	if (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')') {
		//If the next two characters are "^^", then:
		if (*s == L'^' && *++s == L'^') {
			//Check if the next character is L'<', and if so then
			//copy characters from s into dt until reaching a L'>' character.
			//Then move the string-pointer past the L'>' and break from
			//the loop.
			if (*++s == L'<')  {
				++s;
				while (*s != L'>') dt += *s++;
				++s;
				break;
			}
		//Otherwise, if the next character is L'@', then copy characters
		//from s into lang until reaching a whitespace character. Then break
		//from the loop.
		} else if (*s == L'@') { 
			while (!iswspace(*s)) lang += *s++;
			break;
		}

		//Otherwise we didn't receive either a langtag or an iri, so throw an error:
		else throw wruntime_error(string(L"expected langtag or iri:") + string(s,0,48));
	}
	//Append a null character to t and set pos back to 0.
	t[pos] = 0; pos = 0;
	//Copy t into t1 and replace all occurrences of L"\\\\" with L"\\".
	string t1 = t;
	boost::replace_all(t1, L"\\\\", L"\\");

	//Make a literal node from the value, the IRI and the langtag, and return this node.
	return mkliteral(wstrim(t1), pstrtrim(dt), pstrtrim(lang));
}


pnode nqparser::readany(bool lit){
	pnode pn;
	//Attempt to read a prefix tag.
	readprefix();

	//Try to read the next token using: readbnode(), readvar(), readlit(), readlist(), readcurly(),
	//and readiri(). If it fails, return "(pnode)0". Otherwise, return the node received into pn.
	if (!(pn = readbnode()) && !(pn = readvar()) && (!lit || !(pn = readlit())) && !(pn = readlist()) && !(pn = readcurly()) && !(pn = readiri()) )
		return (pnode)0;
	return pn;
}


void nqparser::preprocess(std::wistream& is, std::wstringstream& ss){
        string s; //get lines from is into s.

        //Make a function out of this
        //Iterate over lines coming from is, placing them into s, and
	//accumulating these strings into ss.
        while (getline(is, s)) {
                //dout << "line:\"" << s << "\"" << std::endl;

                //Trim white-space on the line.
                //(in what specific manner?)
                trim(s);

                //If it's an empty line or a comment then continue.
                if (!s.size() || s[0] == '#') continue;

                //Check to see if the line is "fin." or some variation where
                //white-space separates "fin" from "." (...?), and if so
                //break.
                if (startsWith(s, L"fin") && *wstrim(s.c_str() + 3) == L"."){
                        break;
                }
		//dout << s << endl;
                //Otherwise, add this line into ss. 
                //(more trailing white-space).
                ss << ' ' << s<< ' ';

	}
}


void nqparser::nq_to_qdb(qdb& kb, std::wistream& is){
	_kb = kb;

//std::pair<std::list<quad>, std::map<string, std::list<pnode>>> nqparser::operator()(const wchar_t* _s, string ctx/* = L"@default"*/) {

	//Read an nq file in is into the wstringstream ss.
	std::wstringstream ss;

	preprocess(is,ss);

	const wchar_t* _s = (wchar_t*)ss.str().c_str();
	string ctx;
	std::list<std::pair<pnode, plist>> preds;
	//std::pair<std::list<quad>,std::map<string, std::list<pnode>>> rr;
	//std::pair<std::map<string, pqlist>, std::map<string, std::list<pnode>>> qdb
	//qdb rr_qdb;	
	s = _s;
	if (!s || !*s) kb = {{},{}};
	//if (!s || !*s) rr = {{},{}};

	string graph;
	pnode subject, pn;
	pos = 0;

	//Get a reverse_iterator pointing to the last element in preds into pos1.
	auto pos1 = preds.rbegin();

	if (s){
	while(*s) {
		//This is expecting to read a subject node immediately by using readany(false).
		//Try to read any node except a literal into subject. RDF subjects can only
		//be IRIs or blank-nodes. If we fail to read a node, then throw an error.
		if (!(subject = readany(false)))
			throw wruntime_error(string(L"expected iri or bnode subject:") + string(s,0,48));

		//This is expecting to be looping over the predicate-object pairs given for this subject,
		//separated by L';' i.e. like "subject pred1 obj1; pred2 obj2;" or
		// "subject pred1 obj1a, obj1b, obj1c; pred2 obj2a, obj2b, obj2c." etc.
		//This loop is over the preds, i.e. "subject pred1 ...; pred2 ...; ...."
		//and you'll see in the do-condition that reading L';' is what iterates the loop.	
		do {
			while (iswspace(*s) || *s == L';') ++s;

			//On the first time around this might just be subject-nothing-nothing	
			//If we reach either the end, a L'.', or a L'}', then break.
			if (!*s || *s == L'.' || *s == L'}') break; 
	
			//readcurly() here?
			//Otherwise, try to readiri() and then try to readcurly().
			if ((pn = readiri()) || (pn = readcurly())) {
				//If one of them succeeds, we'll have a node in 'pn', and we'll
				//stick this node along with an empty list on the back of preds.
				//This list will hold all the object nodes captured in the
				//following do-loop.
				preds.emplace_back(pn, plist());

				//Set pos1 to the back of the list
				pos1 = preds.rbegin();
			}
			//If both readiri() and readcurly() fail, then throw an error.
			else throw wruntime_error(string(L"expected iri predicate:") + string(s,0,48));

			//This loop is over the objects for each predicate, and you can see
			//in the do-condition that it iterates on ending up at a L','.
			do {
				//Skip the following characters until reading something besides
				//whitespace and L','.
				while (iswspace(*s) || *s == L',') ++s;
				
				//*On the first time around this would be subject-predicate-nothing
				//If that character is L'.' or L'}' then break.
				if (*s == L'.' || *s == L'}') break;

				//Otherwise, it's some other character, try to read a node
				//into 'pn' using readany(true). (Can be either IRI, blank node,
				//or literal).
				if ((pn = readany(true))) {
					//If we successfully read a node, then we place it on
					//the back of the plist() we made for our predicate.
					pos1->second.push_back(pn);
				}

				//If we can't read a node using readany(true), then throw an error.
				else throw wruntime_error(string(L"expected iri or bnode or literal object:") + string(s,0,48));
				//Skip any more white-space following this.
				while (iswspace(*s)) ++s;

			//If the next character is L',', then repeat.
			//i.e. expecting to capture another object.
			} while (*s == L',');

			//Okay, we did that do-sequence and finally exited because when we
			//checked the conditional, the character was something other than L','.
			//Skip any more white-space following this.
			while (iswspace(*s)) ++s;

		//If the next character is L';', then repeat.
		//i.e. expecting to capture another predicate and object list.
		} while (*s == L';');

		//Okay, we did that do-sequence and finally exited because when we checked
		//the conditional, the character was something other than L';'. Check to see
		//if the next character is either L'.' or L'}' or if we hit the end of the file.

		if (*s != L'.' && *s != L'}' && *s) {

			//We did not get any of those, and so we'll try to read either a blank node
			//or an IRI node into pn. We're attempting to read a context label. If that 
			//fails, we throw an error, otherwise we copy the value of this node into 'graph'.
			if (!(pn = readbnode()) && !(pn = readiri()))
				throw wruntime_error(string(L"expected iri or bnode graph:") + string(s,0,48));

			graph = *pn->value;
		} else
			//The next character was either L'.' or L'}', and so we copy the
			//value of 'ctx' into 'graph'.
			graph = ctx;

		if (kb.first.find(graph) == kb.first.end()) kb.first[graph] = make_shared<qlist>();
		for (auto d : lists){
			quad dquad = quad(std::get<0>(d), std::get<1>(d), std::get<2>(d), graph);
			kb.first[graph]->push_back(make_shared<quad>(dquad));
		}
				//r.emplace_back(std::get<0>(d), std::get<1>(d), std::get<2>(d), graph);

		for (auto x : preds){
			for (pnode object : x.second){
				quad xquad = quad(subject, x.first, object, graph);
				kb.first[graph]->push_back(make_shared<quad>(xquad));
			}
		}
				//r.emplace_back(subject, x.first, object, graph);
		lists.clear();
		preds.clear();

		
		//Skip all following white-space, the character '.' if it's there, and 
		//then skip all white-space following that.
		while (iswspace(*s)) ++s;

		if(*s == '.') ++s; 
//what about white-space between the '.' how many are you expecting? well, not sure if this parser is attempting to follow any spec, but personally im not expecting more than one
		//according to this as many as i want
		while (iswspace(*s)) ++s;

		//Okay we're past the white-space and the '.', 
		//return here?
		if (*s == L'}') { ++s; /*rr = { r, qlists };*/ }
		if (*s == L')') throw wruntime_error(string(L"expected ) outside list: ") + string(s,0,48));
	}
	}
}
#endif
