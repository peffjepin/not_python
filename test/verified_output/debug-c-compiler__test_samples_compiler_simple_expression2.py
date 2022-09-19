// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYFLOAT simple_expression2_x;
PYFLOAT simple_expression2_y;
PYFLOAT simple_expression2_z;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
simple_expression2_x = 1.0;
simple_expression2_y = 2.0;
PYFLOAT NP_var0 = simple_expression2_x+simple_expression2_y;
simple_expression2_z = NP_var0+3.0;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0