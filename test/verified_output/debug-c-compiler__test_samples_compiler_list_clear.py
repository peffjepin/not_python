// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT NP_var0;
PYLIST list_clear_l;
PYLIST NP_var2;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
list_clear_l = LIST_INIT(PYINT);
LIST_APPEND(list_clear_l, PYINT, NP_var0);
NP_var0 = 2;
LIST_APPEND(list_clear_l, PYINT, NP_var0);
NP_var0 = 3;
LIST_APPEND(list_clear_l, PYINT, NP_var0);
NP_var2 = list_clear_l;
list_clear(NP_var2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0