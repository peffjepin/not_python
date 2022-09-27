// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT NP_var0;
PYLIST list_count_l;
PYLIST NP_var2;
PYINT NP_var3;
PYINT list_count_n;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
list_count_l = LIST_INIT(PYINT);
LIST_APPEND(list_count_l, PYINT, NP_var0);
NP_var0 = 2;
LIST_APPEND(list_count_l, PYINT, NP_var0);
NP_var0 = 2;
LIST_APPEND(list_count_l, PYINT, NP_var0);
NP_var0 = 3;
LIST_APPEND(list_count_l, PYINT, NP_var0);
NP_var2 = list_count_l;
NP_var3 = 2;
LIST_COUNT(NP_var2, PYINT, int_eq, NP_var3, list_count_n);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0