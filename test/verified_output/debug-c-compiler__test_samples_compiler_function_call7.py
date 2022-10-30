
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt function_call7_a;
NpFunction function_call7_my_function;
NpInt _np_0(NpContext __ctx__, NpInt x, NpInt y, NpInt z, NpInt w);

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt _np_0(NpContext __ctx__, NpInt x, NpInt y, NpInt z, NpInt w) {
NpInt _np_1;
_np_1 = x;
NpInt _np_2;
_np_2 = y;
NpInt _np_3;
_np_3 = _np_1 + _np_2;
NpInt _np_4;
_np_4 = z;
NpInt _np_5;
_np_5 = _np_3 + _np_4;
NpInt _np_6;
_np_6 = w;
NpInt _np_7;
_np_7 = _np_5 + _np_6;
return _np_7;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
function_call7_my_function.addr = _np_0;
NpInt _np_8;
_np_8 = 1;
NpInt _np_9;
_np_9 = 2;
NpInt _np_10;
_np_10 = 5;
NpInt _np_11;
_np_11 = 4;
NpInt _np_12;
_np_12 = _np_10 * _np_11;
NpInt _np_13;
_np_13 = 1;
NpInt _np_14;
_np_14 = _np_12 + _np_13;
NpInt _np_15;
_np_15 = 3;
if (global_exception) {
return 1;
}
function_call7_a = ((NpInt (*)(NpContext, NpInt, NpInt, NpInt, NpInt))function_call7_my_function.addr)(function_call7_my_function.ctx, _np_8, _np_9, _np_15, _np_14);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0