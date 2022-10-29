
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFloat function_call3_z;
NpFunction function_call3_my_func;
NpFloat _np_0(NpContext __ctx__, NpFloat x, NpFloat y);

// FUNCTION DEFINITIONS COMPILER SECTION
NpFloat _np_0(NpContext __ctx__, NpFloat x, NpFloat y) {
NpFloat _np_1;
_np_1 = x;
NpFloat _np_2;
_np_2 = y;
NpFloat _np_3;
_np_3 = _np_1 + _np_2;
return _np_3;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
function_call3_my_func.addr = _np_0;
NpFloat _np_5;
_np_5 = 1.000000;
NpFloat _np_6;
_np_6 = 3.000000;
NpFloat _np_4;
_np_4 = ((NpFloat (*)(NpContext, NpFloat, NpFloat))function_call3_my_func.addr)(function_call3_my_func.ctx, _np_5, _np_6);
NpFloat _np_8;
_np_8 = 2.000000;
NpFloat _np_9;
_np_9 = 3.000000;
NpFloat _np_7;
_np_7 = ((NpFloat (*)(NpContext, NpFloat, NpFloat))function_call3_my_func.addr)(function_call3_my_func.ctx, _np_8, _np_9);
NpInt _np_10;
_np_10 = 2;
NpFloat _np_11;
_np_11 = _np_10 * _np_4;
NpInt _np_12;
_np_12 = 3;
NpFloat _np_13;
_np_13 = ((NpFloat)_np_12) / ((NpFloat)_np_7);
function_call3_z = _np_4 + _np_13;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0