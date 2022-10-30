
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt function_call2_y;
NpFunction function_call2_my_func;
NpInt _np_0(NpContext __ctx__, NpInt x);

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt _np_0(NpContext __ctx__, NpInt x) {
NpInt _np_1;
_np_1 = x;
return _np_1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
function_call2_my_func.addr = _np_0;
NpInt _np_2;
_np_2 = 3;
if (global_exception) {
return 1;
}
function_call2_y = ((NpInt (*)(NpContext, NpInt))function_call2_my_func.addr)(function_call2_my_func.ctx, _np_2);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0