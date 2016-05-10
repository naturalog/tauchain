#include <stdbool.h>
#include <wchar.h>

typedef unsigned uint;
typedef const char* pcch;

typedef uint term;
typedef void* set;
typedef void* scope;

unit term_create_lit	(pcch);
unit term_create_list	(unit*);
unit term_create_var	(pcch, scope);

set  set_create		(void *key);
bool set_require	(set, term, term, scope); // term=term
bool set_merge		(set s, set d, set *res, scope);
unit set_compact	(set); // returns set's size
