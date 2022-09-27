// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT NP_var0;
PYINT NP_var1;
PYDICT dict_for_each_d;
PYDICT NP_var2;
PYINT NP_var3;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
NP_var1 = 2;
dict_for_each_d = DICT_INIT(PYINT, PYINT, void_int_eq);
dict_set_item(dict_for_each_d, &NP_var0, &NP_var1);
NP_var2 = dict_for_each_d;
DICT_ITER_KEYS(NP_var2, PYINT, NP_var3, NP_var4)
{
}
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0