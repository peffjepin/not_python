
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt simple_expression3_x;
NpInt simple_expression3_y;
NpInt simple_expression3_z;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
NpInt _np_0;
_np_0 = 1;
NpInt _np_1;
_np_1 = 2;
NpInt _np_2;
_np_2 = _np_0 + _np_1;
NpInt _np_3;
_np_3 = 3;
simple_expression3_x = _np_2 + _np_3;
NpInt _np_4;
_np_4 = 9;
NpInt _np_5;
_np_5 = 8;
NpInt _np_6;
_np_6 = _np_4 - _np_5;
NpInt _np_7;
_np_7 = 7;
simple_expression3_y = _np_6 - _np_7;
NpInt _np_8;
_np_8 = 2;
NpInt _np_9;
_np_9 = simple_expression3_x;
NpInt _np_10;
_np_10 = _np_8 * _np_9;
NpInt _np_11;
_np_11 = 3;
NpInt _np_12;
_np_12 = simple_expression3_y;
NpInt _np_13;
_np_13 = _np_11 * _np_12;
simple_expression3_z = _np_10 + _np_13;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0