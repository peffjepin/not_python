
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt simple_expression1_x;
NpInt simple_expression1_y;
NpInt simple_expression1_z;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
simple_expression1_x = 1;
simple_expression1_y = 2;
NpInt _np_0;
_np_0 = simple_expression1_x;
NpInt _np_1;
_np_1 = simple_expression1_y;
NpInt _np_2;
_np_2 = _np_0 + _np_1;
NpInt _np_3;
_np_3 = 3;
simple_expression1_z = _np_2 + _np_3;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0