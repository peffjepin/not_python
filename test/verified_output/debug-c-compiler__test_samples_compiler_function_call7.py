
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt function_call7_a;
NpFunction function_call7_my_function;

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt _np_0(NpContext __ctx__, NpInt x, NpInt y, NpInt z, NpInt w) {
NpInt _np_2;
_np_2 = x+y;
NpInt _np_3;
_np_3 = _np_2+z;
NpInt _np_1;
_np_1 = _np_3+w;
return _np_1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
function_call7_my_function.addr = &_np_0;
NpInt _np_4;
_np_4 = 1;
NpInt _np_5;
_np_5 = 2;
NpInt _np_7;
_np_7 = 5*4;
NpInt _np_6;
_np_6 = _np_7+1;
NpInt _np_8;
_np_8 = 3;
function_call7_a = ((NpInt (*)(NpContext ctx, NpInt, NpInt, NpInt, NpInt))function_call7_my_function.addr)(function_call7_my_function.ctx, _np_4, _np_5, _np_8, _np_6);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0