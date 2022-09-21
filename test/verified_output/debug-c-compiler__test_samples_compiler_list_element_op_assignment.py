// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYLIST list_element_op_assignment_l;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
PYINT NP_var0 = 1;
list_element_op_assignment_l = LIST_INIT(PYINT);
LIST_APPEND(list_element_op_assignment_l, PYINT, NP_var0);
NP_var0 = 2;
LIST_APPEND(list_element_op_assignment_l, PYINT, NP_var0);
NP_var0 = 3;
LIST_APPEND(list_element_op_assignment_l, PYINT, NP_var0);
PYLIST NP_var1 = list_element_op_assignment_l;
PYINT NP_var2 = 0;
PYINT NP_var4;
LIST_GET_ITEM(NP_var1, PYINT, NP_var2, NP_var4);
PYINT NP_var5 = 1;
PYINT NP_var3 = NP_var4+NP_var5;
LIST_SET_ITEM(NP_var1, PYINT, NP_var2, NP_var3);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0