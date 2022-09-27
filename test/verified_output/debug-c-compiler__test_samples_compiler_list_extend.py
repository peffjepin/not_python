// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT NP_var0;
PYLIST list_extend_l1;
PYINT NP_var1;
PYLIST list_extend_l2;
PYLIST NP_var3;
PYLIST NP_var4;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
list_extend_l1 = LIST_INIT(PYINT);
LIST_APPEND(list_extend_l1, PYINT, NP_var0);
NP_var0 = 2;
LIST_APPEND(list_extend_l1, PYINT, NP_var0);
NP_var0 = 3;
LIST_APPEND(list_extend_l1, PYINT, NP_var0);
NP_var1 = 4;
list_extend_l2 = LIST_INIT(PYINT);
LIST_APPEND(list_extend_l2, PYINT, NP_var1);
NP_var1 = 5;
LIST_APPEND(list_extend_l2, PYINT, NP_var1);
NP_var1 = 6;
LIST_APPEND(list_extend_l2, PYINT, NP_var1);
NP_var3 = list_extend_l1;
NP_var4 = list_extend_l2;
list_extend(NP_var3, NP_var4);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0