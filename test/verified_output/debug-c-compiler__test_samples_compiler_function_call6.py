// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var1;
PyFloat NP_var2;
PyFloat function_call6_z;

// FUNCTION DECLARATIONS COMPILER SECTION
PyFloat function_call6_my_function(PyInt x, PyFloat y);

// FUNCTION DEFINITIONS COMPILER SECTION
PyFloat function_call6_my_function(PyInt x, PyFloat y) {
PyFloat NP_var0;
NP_var0 = (PyFloat)x/y;
return NP_var0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var1 = 1;
NP_var2 = 2.0;
function_call6_z = function_call6_my_function(NP_var1, NP_var2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0