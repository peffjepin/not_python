
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFloat function_call3_z;
NpFunction function_call3_my_func;

// FUNCTION DEFINITIONS COMPILER SECTION
NpFloat _np_0(NpContext __ctx__, NpFloat x, NpFloat y) {
NpFloat _np_1;
_np_1 = x+y;
return _np_1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
function_call3_my_func.addr = &_np_0;
NpFloat _np_3;
_np_3 = 1.0;
NpFloat _np_4;
_np_4 = 3.0;
NpFloat _np_2;
_np_2 = ((NpFloat (*)(NpContext ctx, NpFloat, NpFloat))function_call3_my_func.addr)(function_call3_my_func.ctx, _np_3, _np_4);
NpFloat _np_6;
_np_6 = 2.0;
NpFloat _np_7;
_np_7 = 3.0;
NpFloat _np_5;
_np_5 = ((NpFloat (*)(NpContext ctx, NpFloat, NpFloat))function_call3_my_func.addr)(function_call3_my_func.ctx, _np_6, _np_7);
NpFloat _np_8;
_np_8 = 2*_np_2;
NpFloat _np_9;
_np_9 = (NpFloat)3/_np_5;
function_call3_z = _np_2+_np_9;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0