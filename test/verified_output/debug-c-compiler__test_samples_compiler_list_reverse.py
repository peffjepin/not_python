// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYLIST list_reverse_l;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
PYINT NP_var0 = 1;
list_reverse_l = LIST_INIT(PYINT);
LIST_APPEND(list_reverse_l, PYINT, NP_var0);
NP_var0 = 2;
LIST_APPEND(list_reverse_l, PYINT, NP_var0);
NP_var0 = 3;
LIST_APPEND(list_reverse_l, PYINT, NP_var0);
PYLIST NP_var2 = list_reverse_l;
list_reverse(NP_var2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0