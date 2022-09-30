// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt function_call1_x;

// FUNCTION DECLARATIONS COMPILER SECTION
PyInt function_call1_my_func();

// FUNCTION DEFINITIONS COMPILER SECTION
PyInt function_call1_my_func() {
PyInt NP_var0;
NP_var0 = 1;
return NP_var0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
function_call1_x = function_call1_my_func();
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0