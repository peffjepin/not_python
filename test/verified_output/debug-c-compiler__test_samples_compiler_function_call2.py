
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="my_func", .length=7}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt function_call2_y;
NpInt _np_0(NpContext __ctx__, NpInt x);
NpFunction function_call2_my_func;

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt _np_0(NpContext __ctx__, NpInt x) {
NpInt _np_1;
_np_1 = x;
return _np_1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
function_call2_my_func.__addr__ = _np_0;
function_call2_my_func.__name__ = NOT_PYTHON_STRING_CONSTANTS[1];
NpInt _np_2;
_np_2 = 3;
function_call2_y = ((NpInt (*)(NpContext, NpInt))function_call2_my_func.__addr__)(function_call2_my_func.__ctx__, _np_2);
if (global_exception) {
return 1;
}
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0