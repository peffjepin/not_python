// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var0;
PyInt NP_var1;
PyDict* dict_get_item_d;
PyInt NP_var2;
PyInt dict_get_item_x;
PyInt NP_var3;
PyInt dict_get_item_y;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
NP_var1 = 2;
dict_get_item_d = DICT_INIT(PyInt, PyInt, void_int_eq);
dict_set_item(dict_get_item_d, &NP_var0, &NP_var1);
NP_var0 = 3;
NP_var1 = 4;
dict_set_item(dict_get_item_d, &NP_var0, &NP_var1);
NP_var2 = 1;
dict_get_val(dict_get_item_d, &NP_var2, &dict_get_item_x);
NP_var3 = 3;
dict_get_val(dict_get_item_d, &NP_var3, &dict_get_item_y);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0