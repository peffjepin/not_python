// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYLIST l;
PYINT x;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
PYINT NP_var0 = 1;
l = LIST_INIT(PYINT);
LIST_APPEND(l, PYINT, NP_var0);
NP_var0 = 2;
LIST_APPEND(l, PYINT, NP_var0);
NP_var0 = 3;
LIST_APPEND(l, PYINT, NP_var0);
PYINT NP_var1 = 0;
LIST_GET_ITEM(l, PYINT, NP_var1, x);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0