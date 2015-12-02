kb
@prefix : <http://foo#>.
@prefix list: <http://www.w3.org/2000/10/swap/list#>.
@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>.
@prefix rdfs: <http://www.w3.org/1999/02/22-rdf-schema#>.

(1 2 3) a :local_list.

{?l a :local_list. ?l <http://www.w3.org/1999/02/22-rdf-syntax-ns#first> ?f.}  => { :yes a :result. }.

{?l a :local_list. ?l rdf:first ?f.}  => { :yes2 a :result. }.

fin.

query
@prefix : <http://foo#>.
@prefix list: <http://www.w3.org/2000/10/swap/list#>.
@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>.
@prefix rdfs: <http://www.w3.org/1999/02/22-rdf-schema#>.

?x a :result.

fin.