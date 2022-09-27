// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT NP_var0;
PYLIST list_getitem_l;
PYINT NP_var1;
PYINT list_getitem_x;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
list_getitem_l = LIST_INIT(PYINT);
LIST_APPEND(list_getitem_l, PYINT, NP_var0);
NP_var0 = 2;
LIST_APPEND(list_getitem_l, PYINT, NP_var0);
NP_var0 = 3;
LIST_APPEND(list_getitem_l, PYINT, NP_var0);
NP_var1 = 0;
LIST_GET_ITEM(list_getitem_l, PYINT, NP_var1, list_getitem_x);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0