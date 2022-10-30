
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="hello", .length=5}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFunction fnref_fnref;
NpFunction fnref_my_function;
NpNone _np_0(NpContext __ctx__, NpString value);

// FUNCTION DEFINITIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__, NpString value) {
NpString _np_2;
_np_2 = value;
NpNone _np_1;
_np_1 = builtin_print(1, _np_2);
return 0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
fnref_my_function.addr = _np_0;
fnref_fnref = fnref_my_function;
NpString _np_4;
_np_4 = NOT_PYTHON_STRING_CONSTANTS[1];
NpNone _np_3;
_np_3 = ((NpNone (*)(NpContext, NpString))fnref_fnref.addr)(fnref_fnref.ctx, _np_4);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0