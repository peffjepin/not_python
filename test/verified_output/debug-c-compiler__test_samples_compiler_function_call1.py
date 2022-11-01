
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt function_call1_x;
NpInt _np_0(NpContext __ctx__);
NpFunction function_call1_my_func;

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt _np_0(NpContext __ctx__) {
NpInt _np_1;
_np_1 = 1;
return _np_1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
function_call1_my_func.addr = _np_0;
if (global_exception) {
return 1;
}
function_call1_x = ((NpInt (*)(NpContext))function_call1_my_func.addr)(function_call1_my_func.ctx);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0