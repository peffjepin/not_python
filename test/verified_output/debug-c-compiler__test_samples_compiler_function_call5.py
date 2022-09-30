// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var1;
PyInt NP_var2;
PyInt function_call5_z;

// FUNCTION DECLARATIONS COMPILER SECTION
PyInt function_call5_my_function(PyInt x, PyInt y);

// FUNCTION DEFINITIONS COMPILER SECTION
PyInt function_call5_my_function(PyInt x, PyInt y) {
PyInt NP_var0;
NP_var0 = x+y;
return NP_var0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var1 = 2;
NP_var2 = 3;
function_call5_z = function_call5_my_function(NP_var1, NP_var2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0