
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFloat simple_expression2_x;
NpFloat simple_expression2_y;
NpFloat simple_expression2_z;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
simple_expression2_x = 1.0;
simple_expression2_y = 2.0;
NpFloat _np_0;
_np_0 = simple_expression2_x+simple_expression2_y;
simple_expression2_z = _np_0+3.0;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0