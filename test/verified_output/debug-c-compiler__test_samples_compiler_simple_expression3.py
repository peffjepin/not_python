// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var0;
PyInt simple_expression3_x;
PyInt NP_var1;
PyInt simple_expression3_y;
PyInt NP_var2;
PyInt NP_var3;
PyInt simple_expression3_z;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1+2;
simple_expression3_x = NP_var0+3;
NP_var1 = 9-8;
simple_expression3_y = NP_var1-7;
NP_var2 = 2*simple_expression3_x;
NP_var3 = 3*simple_expression3_y;
simple_expression3_z = NP_var2+NP_var3;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0