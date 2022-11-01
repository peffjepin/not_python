
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="fn", .length=2}};

// TYPEDEFS COMPILER SECTION
typedef struct { NpInt value; } method_call_A;

// DECLARATIONS COMPILER SECTION
method_call_A* method_call_a;
NpInt method_call_x;
NpInt _np_0(NpContext __ctx__);
NpFunction method_call_A_fn;
NpFunction _np_5;

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt _np_0(NpContext __ctx__) {
method_call_A* _np_1;
_np_1 = (method_call_A*)__ctx__.self;
NpInt _np_2;
_np_2 = _np_1->value;
return _np_2;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
method_call_A_fn.__addr__ = _np_0;
method_call_A_fn.__name__ = NOT_PYTHON_STRING_CONSTANTS[1];
method_call_a = np_alloc(8);
if (global_exception) {
return 1;
}
NpInt _np_3;
_np_3 = 1;
method_call_a->value = _np_3;
method_call_A* _np_4;
_np_4 = method_call_a;
_np_5 = method_call_A_fn;
NpContext _np_6;
_np_6 = _np_5.__ctx__;
_np_6.self = _np_4;
_np_5.__ctx__ = _np_6;
method_call_x = ((NpInt (*)(NpContext))_np_5.__addr__)(_np_5.__ctx__);
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