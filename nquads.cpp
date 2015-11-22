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

namespace old{

nqparser::nqparser() : t(new wchar_t[4096*4096]) { }
nqparser::~nqparser() { delete[] t; }

/*
We can define the abstract syntax accepted by the code at some point P in the code as
the set of all strings X(P) such that there exists an execution path leading to P such that:
 
  X(P) == the substring of s from the beginning of s (i.e. s[0]) up to the string-pointer s.

The comparison is of course to be taken at the point P. Let's call X(P) the set of acceptable
strings at P.

Parsing will start at some point in the code that we'll call Z, and X(Z) = {}, the empty set
of strings. Parsing will end at some point that we'll call O, and X(O) must equal the abstract
syntax we wish to accept.

For any two points A, B in the code, let's define X(A,B) as the set of possible strings accepted on the 
way from A to B. For example:


pnode nqparser::readcurly() {
	...
	while (iswspace(*s)) ++s;

We'll take A as the point of execution directly before "while (iswspace(*s)) ++s;", and B as
the point directly after. Since the definition of the abstract syntax is the sets of strings
that can be the substring of s from s[0] to the string-pointer s, then abstract syntax is defined
by advancing the string-pointer s. Here we can see that starting at A, each time it encounters a 
white-space character, it advances the string-pointer by 1, and it will stop doing this as soon as it 
encounters a character that's not a white-space character, leaving it at B. Since s is advanced
across any possible string of white-space along the way from A to B, and only these strings, then
X(A,B) is the set of all possible strings containing just white-space.


If we know X(A), and we know X(A,B), then we can partially derive X(B). Calling this partial
derivation X(B)_A.  Using set-builder notation: 

  X(B)_A = {x.first + x.second | x in X(A)*X(A,B)},

where '+' is string concatenation, '*' is cartesian product of sets, and .first and .second is
the first & second elements in the pairs resulting from the cartesian product.

The reason we can only partially derive X(B) from X(A) and X(A,B) is that X(B) may be able to be
reached from other execution paths that don't lie in any of the execution paths from Z to A or
A to B, but do lie in Z to B. One place this would come up is if A was a function call and B was
a function entry, and the function that B is in is called in multiple different places. If B can
be reached along multiple execution paths C1, C2, ..., Cn, then X(B) can be fully defined by
taking the union of each of it's partial definitions, i.e.:

 X(B) = Union of [ X(B)_C1, X(B)_C2, ... X(B)_Cn].

We can see from these definitions that X(Z,P) = X(P), for all execution points P.

So, we have the ability to derive the abstract syntax accepted by the code at some execution point P,
given the knowledge of the abstract syntax accepted by the code at previous execution points 
leading up to P, along with the knowledge of the strings that can be accepted by the code along the
way from those points to P.

Using this, we can derive the actual abstract syntax accepted by our code in terms of transformations 
from our starting syntax X(Z). 


  



Parsing


 */


//should be 'nqparser::readcontext'
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

	//Create a unique blank-node ID for this context
	auto r = gen_bnode_id();

	//If the next character is L'}' then move the string-pointer past L'}',
	//make a bnode for r, and return this.
	if (*s == L'}') { ++s; return mkbnode(r); }

	//Otherwise: nqparser::nqparser().
	auto t = (*this)(s, *r);
	return mkbnode(r);
}

pnode nqparser::readlist() {
	setproc(L"readlist");

	//If the first character is not L'(', then we assume it's not
	//a list and return "(pnode)0".
	if (*s != L'(') return (pnode)0;

	//We made it here so the first character was L'(', moving on.

	//This static int should maintain state across different callings
	//of readlist() so that we can keep track of the number of lists
	//and give each list it's corresponding number as a listid, so that
	//we know we can get a unique id just by incrementing.
	//static int lastid = 0;
	int lpos = 0;

	//?? do we really need to make a lambda out of this?
	//well...i tried to unlambdaize it once and hmmm, i :mean
	//im all for making functions out of things but this random lambda is weird lol
	/*the thing is......dunno what*/

	const string head = list_bnode_name();

	pnode pn;
	++s;

	//Skip over any white-space
	while (iswspace(*s)) ++s;

	//If the next character is L')' then I guess we have
	//an empty list and we return RDF_NIL.	
	if (*s == L')') {
		 ++s; 
		return mkiri(RDF_NIL); 
	}

	do {
		while (iswspace(*s)) ++s;
		if (*s == L')') break;
		if (!(pn = readany(true)))
			throw wruntime_error(string(L"expected iri or bnode or list in list: ") + string(s,0,48));
		
		pnode cons = mkbnode(pstr(list_bnode_name()));

		lists.emplace_back(cons, mkiri(RDF_FIRST), pn);//then we create first triple?

		qlists[head].push_back(pn);//and into qdb.second i presume we add the item too

		++lpos;

		while (iswspace(*s)) ++s;

		if (*s == L')') lists.emplace_back(cons, mkiri(RDF_REST), mkiri(RDF_NIL));//end

		else lists.emplace_back(cons, mkiri(RDF_REST), mkbnode(pstr(list_bnode_name())));

		if (*s == L'.') while (iswspace(*s++));

		if (*s == L'}') throw wruntime_error(string(L"expected { inside list: ") + string(s,0,48));
	}

	while (*s != L')');

	do { ++s; } while (iswspace(*s));

	return mkbnode(pstr(head));
}

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

pnode nqparser::readbnode() {
	setproc(L"readbnode");

	//Skip any white-space at the beginning of the line:
	while (iswspace(*s)) ++s;

	//If the next two characters are not exactly "_:" then apparently it is not
	//a syntactically correct blank node, and we return "pnode(0)".
	if (*s != L'_' || *(s+1) != L':') return pnode(0);

	//Copy characters from s into t until reaching either a whitespace character
	//or one of the special characters ",;.}{)".
	while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')') t[pos++] = *s++;

	//Append a null character to t and set pos back to 0.
	t[pos] = 0; pos = 0;

	//Make a blank node from the bnode identifier string parsed into t, and return
	//this node.
	return mkbnode(wstrim(t));
}

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
	//Move our string-pointer forward by 8, i.e. past the L"@prefix ".
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

pnode nqparser::readvar() {
	setproc(L"readvar");
	//Skip any white-space at the beginning of the line:
	while (iswspace(*s)) ++s;

	//If the next character is not L'?', then apparently it's not
	//a syntactically correct variable, so we return "pnode(0)"
	if (*s != L'?') return pnode(0);

	//Copy characters from s into t until reaching either a whitespace character
	//or one of the special characters ",;.}{)".
	while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')') t[pos++] = *s++;
	t[pos] = 0; pos = 0;

	//Make an IRI node for the var identifier string that we parsed into t, and return it.
	return mkiri(wstrim(t));
}

pnode nqparser::readlit() {
	setproc(L"readlit");

	//Skip any white-space at the beginning of the line:
	while (iswspace(*s)) ++s;

	//If the next char found is not a L'\"', then it's apparently not a syntactically
	//correct literal, because we're returning 'pnode(0)'
	if (*s != L'\"') return pnode(0);

	//Move the string ponter past the L'\"'.
	++s;

	//Copy characters from s into t until you reach a L'\"' that isn't preceded
	//by a L'\\'. (Because \" is a literal quote character and not one of the
	//quotes delimiting the literal value).
	do { t[pos++] = *s++; } while (!(*(s-1) != L'\\' && *s == L'\"'));
	
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


int nqparser::preprocess(std::wistream& is, std::wstringstream& ss){
	int fins=0;
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
                        fins++;
                        break;
                }
//              dout << s << endl;
//it does some minor preprocessing
                //Otherwise, add this line into ss. 
                //(more trailing white-space).
                ss << ' ' << s<< ' ';
	}

	return fins;	
}

int nq_to_qdb(qdb& kb, std::wistream& is){

	std::wstringstream ss;
	//Read an nq file in is into the wstringstream ss.
	int fins = preprocess(is,ss);
	//so what does this do, sounds like shuffle bytes from one stream into another
	//yep 

//std::pair<std::list<quad>, std::map<string, std::list<pnode>>> nqparser::operator()(const wchar_t* _s, string ctx/* = L"@default"*/) {
	
	const wchar_t* _s = (wchar_t*)ss.str().c_str();
	string ctx;
	std::list<std::pair<pnode, plist>> preds;
	std::pair<std::list<quad>,std::map<string, std::list<pnode>>> rr;
	//std::pair<std::map<string, pqlist>, std::map<string, std::list<pnode>>> qdb
	//qdb rr_qdb;	
	s = _s;
	//if (!s || !*s) rr_qdb = {{},{}};
	if (!s || !*s) rr = {{},{}};

	string graph;
	pnode subject, pn;
	pos = 0;

	//Get a reverse_iterator pointing to the last element in preds into pos1.
	auto pos1 = preds.rbegin();

	//If there's a stringified nq-file to read then
	if (s){
	//While there's a stringified nq-file to read:
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
			//Skip the following characters until reading something besides  whitespace and ";"
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

		for (auto d : lists)
				r.emplace_back(std::get<0>(d), std::get<1>(d), std::get<2>(d), graph);

		for (auto x : preds)
			for (pnode object : x.second)
				r.emplace_back(subject, x.first, object, graph);
		lists.clear();
		preds.clear();

		
		//Skip all following white-space, the character '.' if it's there, and 
		//then skip all white-space following that.
		while (iswspace(*s)) ++s;

		//Should this actually be L'.'? L denotes a wide STRING, this is a ['c','h','a','r']
		while (*s == '.') ++s;
		while (iswspace(*s)) ++s;

		//Okay we're past the white-space and the '.', 
		if (*s == L'}') { ++s; rr = { r, qlists }; }
		if (*s == L')') throw wruntime_error(string(L"expected ) outside list: ") + string(s,0,48));
	}
	}
	rr = { r, qlists };


        kb.second = rr.second;
        for (quad q : r.first) {
                string c = *q.graph->value;
                if (kb.first.find(c) == kb.first.end()) kb.first[c] = make_shared<qlist>();
                kb.first[c]->push_back(make_shared<quad>(q));
        }
        return fins;
}
}
#endif
