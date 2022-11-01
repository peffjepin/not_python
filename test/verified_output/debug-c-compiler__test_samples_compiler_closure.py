
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

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
closure_inner.addr = _np_1;
closure_inner.ctx = __ctx__;
NpNone _np_5;
_np_5 = ((NpNone (*)(NpContext))closure_inner.addr)(closure_inner.ctx);
np_free(__ctx__.closure);
return 0;
}
NpNone _np_1(NpContext __ctx__) {
NpInt _np_3;
_np_3 = (*(NpInt*)(__ctx__.closure + 0));
NpString _np_4;
_np_4 = np_int_to_str(_np_3);
NpNone _np_2;
_np_2 = builtin_print(1, _np_4);
return 0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
closure_outer.addr = _np_0;
NpNone _np_6;
_np_6 = ((NpNone (*)(NpContext))closure_outer.addr)(closure_outer.ctx);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0