
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFloat simple_expression2_x;
NpFloat simple_expression2_y;
NpFloat simple_expression2_z;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
simple_expression2_x = 1.000000;
simple_expression2_y = 2.000000;
NpFloat _np_0;
_np_0 = simple_expression2_x;
NpFloat _np_1;
_np_1 = simple_expression2_y;
NpFloat _np_2;
_np_2 = _np_0 + _np_1;
NpFloat _np_3;
_np_3 = 3.000000;
simple_expression2_z = _np_2 + _np_3;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0