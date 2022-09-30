// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var0;
PyInt NP_var1;
PyDict* dict_pop_d;
PyDict* NP_var3;
PyInt dict_pop_v;
PyInt NP_var4;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
NP_var1 = 2;
dict_pop_d = DICT_INIT(PyInt, PyInt, void_int_eq);
dict_set_item(dict_pop_d, &NP_var0, &NP_var1);
NP_var3 = dict_pop_d;
NP_var4 = 1;
dict_pop_val(NP_var3, &NP_var4, &dict_pop_v);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0