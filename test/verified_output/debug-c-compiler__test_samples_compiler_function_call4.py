
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="my_function", .length=11}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt function_call4_z;
NpInt _np_0(NpContext __ctx__, NpInt x, NpInt y);
NpFunction function_call4_my_function;

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt _np_0(NpContext __ctx__, NpInt x, NpInt y) {
NpInt _np_1;
_np_1 = x;
NpInt _np_2;
_np_2 = y;
NpInt _np_3;
_np_3 = _np_1 + _np_2;
return _np_3;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
function_call4_my_function.__addr__ = _np_0;
function_call4_my_function.__name__ = NOT_PYTHON_STRING_CONSTANTS[1];
NpInt _np_4;
_np_4 = 1;
NpInt _np_5;
_np_5 = 2;
NpInt _np_6;
_np_6 = _np_4 + _np_5;
NpInt _np_7;
_np_7 = 3;
NpInt _np_8;
_np_8 = _np_6 + _np_7;
NpInt _np_9;
_np_9 = 4;
NpInt _np_10;
_np_10 = 5;
NpInt _np_11;
_np_11 = _np_9 * _np_10;
NpInt _np_12;
_np_12 = 6;
NpInt _np_13;
_np_13 = _np_11 * _np_12;
if (global_exception) {
return 1;
}
function_call4_z = ((NpInt (*)(NpContext, NpInt, NpInt))function_call4_my_function.__addr__)(function_call4_my_function.__ctx__, _np_8, _np_13);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0