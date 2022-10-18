
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt function_call4_z;
NpFunction function_call4_my_function;

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt _np_0(NpContext __ctx__, NpInt x, NpInt y) {
NpInt _np_1;
_np_1 = x+y;
return _np_1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
function_call4_my_function.addr = &_np_0;
NpInt _np_3;
_np_3 = 1+2;
NpInt _np_2;
_np_2 = _np_3+3;
NpInt _np_5;
_np_5 = 4*5;
NpInt _np_4;
_np_4 = _np_5*6;
function_call4_z = ((NpInt (*)(NpContext ctx, NpInt, NpInt))function_call4_my_function.addr)(function_call4_my_function.ctx, _np_2, _np_4);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0