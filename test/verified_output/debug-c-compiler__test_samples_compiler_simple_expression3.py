
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt simple_expression3_x;
NpInt simple_expression3_y;
NpInt simple_expression3_z;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_0;
_np_0 = 1+2;
simple_expression3_x = _np_0+3;
NpInt _np_1;
_np_1 = 9-8;
simple_expression3_y = _np_1-7;
NpInt _np_2;
_np_2 = 2*simple_expression3_x;
NpInt _np_3;
_np_3 = 3*simple_expression3_y;
simple_expression3_z = _np_2+_np_3;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0