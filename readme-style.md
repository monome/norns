# norns
## c programming style guide

this is a work in progress! please add to it, request clarification, point out discrepancies, dispute, &c.

## formatting

use `uncrustify` with the included `uncrustify.cfg` to enforce formatting rules. in a nutshell, they are:

- 2-space indentation
- always spaces, never tabs
- 80 columns; reflow long comments
- no enforcement of LF / CR (native line breaks)
- no enforcement of line break before opening brace (i'm open to changing this.)
- no indent on opening brace
- pointer star(s) aligned with variable: `char *p`, not `char* p`
- spaces around operators
- always use full braces: (`if(foo) { bar(); }`, not `if(foo) bar; `)
- match spaces in nested parentheses
- (others i'm forgetting...)

## file structure

order of things in files:

### headers
- opening comment naming the header and describing its function
- includes
- defines
- types
- function declarations

### source
- includes
- defines
- types
- static variables
- static function declarations (very short inline functions can be defined here, skipping separate declaration)
- extern function definitions
- static function definitions

## functions
	
* new function names should be all-lowercase, with underscores for spaces: `foo_bar_baz(){}`. 
	
* static function names are otherwise unrestricted.
	
* functions with external linkage should be prefixed with a unique string indicating the code module: `extern void foo_init(); extern void foo_deinit(); ` by default, this prefix is the module name. for long module names used exceptionally often, it's ok to abbreviate: `foobarbaz_init()` to `fbb_init()` or even `f_init()`. 

* prefer short functions (no more than one screenful of text.)

## variables

* variable names are unrestricted. `lower_case_underscore` and `lowerCamelCase` are both acceptable. 

* *all* global variables should have static linkage; use accessor functions to manipulate a global variable from outside the module that declared it. thus, it is not really necessary to use the keyword `static` for variable declarations.

* prefer variable declaration at the lowest possible scope. if this starts to look funny, consider breaking up the function.

* prefer using `static const` variables instead of preprocessor defines whenever possible.  

## `typedef` and `struct`

* new types should suffixed with `_t`, and created only from basic data types and and enums. e.g.: `typedef uint8_t foo_t;`, `typedef enum { BAR_BAZ, BAR_ZAP } bar_t;`
	
* the `struct` keyword should always be used with structs, and struct names do not need a `_t`: 
```
struct foo { int bar; };
void foo_inc(struct foo *f) { f->bar++; }
```

if program design calls for a truly opaque datatype, it _might_ be acceptable to typedef a struct; in this case use `UpperCamelCase` with no `_t` suffix. this should be an exceptional occurence; better to simply expose the data structure; best to confine data structure knowledge to a single code module if possible.
	
## `goto`

only to be used within a function to implement early exit / state cleanup. don't hestitate to use this whenever it simplifies exit logic and makes the function code more concise.

## code modules

the general consideration for separating code into modules is to compartmentalize access to memory, knowledge of data structures, and references to external libraries. for example, only the `timers.c` module requires use of `nanosleep`, knowledge of timer struct layout, or direct access to memory associated with timers.

## include guards

use `#pragma once` instead of `#indef FOO_H #define FOO_H ... #endif`. it's cleaner, less error-prone, and close enough to standard as to make no difference. also has the dubious benefit of correctly handling multiple headers with the same name (but try not to do that anyway.)
