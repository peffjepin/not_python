
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFloat function_call8_z;
NpFunction function_call8_my_function;
NpFloat _np_0(NpContext __ctx__, NpInt x, NpFloat y);

// FUNCTION DEFINITIONS COMPILER SECTION
NpFloat _np_0(NpContext __ctx__, NpInt x, NpFloat y) {
NpInt _np_1;
_np_1 = x;
NpFloat _np_2;
_np_2 = y;
NpFloat _np_3;
_np_3 = ((NpFloat)_np_1) / ((NpFloat)_np_2);
return _np_3;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
function_call8_my_function.addr = _np_0;
NpFloat _np_4;
_np_4 = 2.000000;
NpInt _np_5;
_np_5 = 1;
if (global_exception) {
return 1;
}
function_call8_z = ((NpFloat (*)(NpContext, NpInt, NpFloat))function_call8_my_function.addr)(function_call8_my_function.ctx, _np_5, _np_4);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0