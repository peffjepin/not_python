
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFloat function_call6_z;
NpFunction function_call6_my_function;

// FUNCTION DEFINITIONS COMPILER SECTION
NpFloat _np_0(NpContext __ctx__, NpInt x, NpFloat y) {
NpFloat _np_1;
_np_1 = (NpFloat)x/y;
return _np_1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
function_call6_my_function.addr = &_np_0;
NpInt _np_2;
_np_2 = 1;
NpFloat _np_3;
_np_3 = 2.0;
function_call6_z = ((NpFloat (*)(NpContext ctx, NpInt, NpFloat))function_call6_my_function.addr)(function_call6_my_function.ctx, _np_2, _np_3);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0