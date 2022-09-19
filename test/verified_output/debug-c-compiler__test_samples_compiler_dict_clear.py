// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYDICT dict_clear_d;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
PYINT NP_var0 = 1;
PYINT NP_var1 = 2;
dict_clear_d = DICT_INIT(PYINT, PYINT, void_int_eq);
dict_set_item(dict_clear_d, &NP_var0, &NP_var1);
NP_var0 = 3;
NP_var1 = 4;
dict_set_item(dict_clear_d, &NP_var0, &NP_var1);
PYDICT NP_var3 = dict_clear_d;
dict_clear(NP_var3);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0