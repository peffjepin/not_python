// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYDICT d;
PYINT x;
PYINT y;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
PYINT NP_var0 = 1;
PYINT NP_var1 = 2;
d = DICT_INIT(PYINT, PYINT, void_int_eq);
dict_set_item(d, &NP_var0, &NP_var1);
NP_var0 = 3;
NP_var1 = 4;
dict_set_item(d, &NP_var0, &NP_var1);
PYINT NP_var2 = 1;
x = *(PYINT*)dict_get_val(d, &NP_var2);
PYINT NP_var3 = 3;
y = *(PYINT*)dict_get_val(d, &NP_var3);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0