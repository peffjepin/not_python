// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var1;
PyInt function_call2_y;

// FUNCTION DECLARATIONS COMPILER SECTION
PyInt function_call2_my_func(PyInt x);

// FUNCTION DEFINITIONS COMPILER SECTION
PyInt function_call2_my_func(PyInt x) {
PyInt NP_var0;
NP_var0 = x;
return NP_var0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var1 = 3;
function_call2_y = function_call2_my_func(NP_var1);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0