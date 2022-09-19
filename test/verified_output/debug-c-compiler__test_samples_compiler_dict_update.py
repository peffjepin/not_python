// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYDICT dict_update_d1;
PYDICT dict_update_d2;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
PYINT NP_var0 = 1;
PYINT NP_var1 = 2;
dict_update_d1 = DICT_INIT(PYINT, PYINT, void_int_eq);
dict_set_item(dict_update_d1, &NP_var0, &NP_var1);
PYINT NP_var2 = 2;
PYINT NP_var3 = 3;
dict_update_d2 = DICT_INIT(PYINT, PYINT, void_int_eq);
dict_set_item(dict_update_d2, &NP_var2, &NP_var3);
PYDICT NP_var5 = dict_update_d1;
PYDICT NP_var6 = dict_update_d2;
dict_update(NP_var5, NP_var6);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0