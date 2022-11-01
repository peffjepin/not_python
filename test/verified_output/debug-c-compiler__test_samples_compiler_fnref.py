
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="my_function", .length=11},
{.data="hello", .length=5}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFunction fnref_fnref;
NpNone _np_0(NpContext __ctx__, NpString value);
NpFunction fnref_my_function;

// FUNCTION DEFINITIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__, NpString value) {
NpString _np_2;
_np_2 = value;
NpNone _np_1;
_np_1 = builtin_print(1, _np_2);
if (global_exception) {
return 0;
}
return 0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
fnref_my_function.__addr__ = _np_0;
fnref_my_function.__name__ = NOT_PYTHON_STRING_CONSTANTS[1];
fnref_fnref = fnref_my_function;
NpString _np_4;
_np_4 = NOT_PYTHON_STRING_CONSTANTS[2];
NpNone _np_3;
_np_3 = ((NpNone (*)(NpContext, NpString))fnref_fnref.__addr__)(fnref_fnref.__ctx__, _np_4);
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