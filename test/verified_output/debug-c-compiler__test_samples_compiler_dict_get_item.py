// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT NP_var0;
PYINT NP_var1;
PYDICT dict_get_item_d;
PYINT NP_var2;
PYINT dict_get_item_x;
PYINT NP_var3;
PYINT dict_get_item_y;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
NP_var1 = 2;
dict_get_item_d = DICT_INIT(PYINT, PYINT, void_int_eq);
dict_set_item(dict_get_item_d, &NP_var0, &NP_var1);
NP_var0 = 3;
NP_var1 = 4;
dict_set_item(dict_get_item_d, &NP_var0, &NP_var1);
NP_var2 = 1;
dict_get_item_x = *(PYINT*)dict_get_val(dict_get_item_d, &NP_var2);
NP_var3 = 3;
dict_get_item_y = *(PYINT*)dict_get_val(dict_get_item_d, &NP_var3);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0