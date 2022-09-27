// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT NP_var0;
PYLIST list_setitem_l;
PYLIST NP_var1;
PYINT NP_var2;
PYINT NP_var3;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
list_setitem_l = LIST_INIT(PYINT);
LIST_APPEND(list_setitem_l, PYINT, NP_var0);
NP_var0 = 2;
LIST_APPEND(list_setitem_l, PYINT, NP_var0);
NP_var0 = 3;
LIST_APPEND(list_setitem_l, PYINT, NP_var0);
NP_var1 = list_setitem_l;
NP_var2 = 0;
NP_var3 = 4;
LIST_SET_ITEM(NP_var1, PYINT, NP_var2, NP_var3);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0