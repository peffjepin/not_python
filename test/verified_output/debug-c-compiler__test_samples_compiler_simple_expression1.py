// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt simple_expression1_x;
NpInt simple_expression1_y;
NpInt NP_var0;
NpInt simple_expression1_z;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
simple_expression1_x = 1;
simple_expression1_y = 2;
NP_var0 = simple_expression1_x+simple_expression1_y;
simple_expression1_z = NP_var0+3;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0