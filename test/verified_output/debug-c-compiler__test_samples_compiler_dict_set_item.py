// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="ten", .length=3},
{.data="twenty", .length=6}};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyDict* dict_set_item_d;
PyDict* NP_var0;
PyInt NP_var1;
PyString NP_var2;
PyDict* NP_var3;
PyInt NP_var4;
PyString NP_var5;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
dict_set_item_d = DICT_INIT(PyInt, PyString, void_int_eq);
NP_var0 = dict_set_item_d;
NP_var1 = 10;
NP_var2 = NOT_PYTHON_STRING_CONSTANTS[0];
dict_set_item(NP_var0, &NP_var1, &NP_var2);
NP_var3 = dict_set_item_d;
NP_var4 = 20;
NP_var5 = NOT_PYTHON_STRING_CONSTANTS[1];
dict_set_item(NP_var3, &NP_var4, &NP_var5);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0