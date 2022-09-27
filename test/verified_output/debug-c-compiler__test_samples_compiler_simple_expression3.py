// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYINT NP_var0;
PYINT simple_expression3_x;
PYINT NP_var1;
PYINT simple_expression3_y;
PYINT NP_var2;
PYINT NP_var3;
PYINT simple_expression3_z;

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