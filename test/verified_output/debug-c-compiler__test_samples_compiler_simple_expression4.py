// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var1;
PyInt NP_var0;
PyInt NP_var3;
PyFloat NP_var2;
PyFloat simple_expression4_a;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var1 = 2*3;
NP_var0 = 1+NP_var1;
NP_var3 = 3*2;
NP_var2 = (PyFloat)NP_var3/(PyFloat)4;
simple_expression4_a = NP_var0-NP_var2;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0