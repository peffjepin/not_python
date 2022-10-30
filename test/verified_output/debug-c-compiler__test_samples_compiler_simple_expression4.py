
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFloat simple_expression4_a;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
NpInt _np_0;
_np_0 = 2;
NpInt _np_1;
_np_1 = 3;
NpInt _np_2;
_np_2 = _np_0 * _np_1;
NpInt _np_3;
_np_3 = 1;
NpInt _np_4;
_np_4 = _np_3 + _np_2;
NpInt _np_5;
_np_5 = 3;
NpInt _np_6;
_np_6 = 2;
NpInt _np_7;
_np_7 = _np_5 * _np_6;
NpInt _np_8;
_np_8 = 4;
NpFloat _np_9;
_np_9 = ((NpFloat)_np_7) / ((NpFloat)_np_8);
simple_expression4_a = _np_4 - _np_9;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0