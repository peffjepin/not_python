
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="inner", .length=5},
{.data="outer", .length=5}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__);
NpNone _np_1(NpContext __ctx__);
NpFunction closure_outer;

// FUNCTION DEFINITIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__) {
__ctx__.closure = np_alloc(8);
(*(NpInt*)(__ctx__.closure + 0)) = 1;
NpFunction closure_inner;
closure_inner.__addr__ = _np_1;
closure_inner.__name__ = NOT_PYTHON_STRING_CONSTANTS[1];
closure_inner.__ctx__ = __ctx__;
NpNone _np_5;
_np_5 = ((NpNone (*)(NpContext))closure_inner.__addr__)(closure_inner.__ctx__);
if (global_exception) {
np_free(__ctx__.closure);
return 0;
}
np_free(__ctx__.closure);
return 0;
}
NpNone _np_1(NpContext __ctx__) {
NpInt _np_3;
_np_3 = (*(NpInt*)(__ctx__.closure + 0));
NpString _np_4;
_np_4 = np_int_to_str(_np_3);
if (global_exception) {
return 0;
}
NpNone _np_2;
_np_2 = builtin_print(1, _np_4);
if (global_exception) {
return 0;
}
return 0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
closure_outer.__addr__ = _np_0;
closure_outer.__name__ = NOT_PYTHON_STRING_CONSTANTS[2];
NpNone _np_6;
_np_6 = ((NpNone (*)(NpContext))closure_outer.__addr__)(closure_outer.__ctx__);
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