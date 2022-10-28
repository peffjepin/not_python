
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION
typedef struct { NpInt value; } method_call_A;

// DECLARATIONS COMPILER SECTION
method_call_A* method_call_a;
NpInt method_call_x;

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt _np_0(NpContext __ctx__) {
method_call_A* self;
self = __ctx__.self;
method_call_A* _np_1;
_np_1 = self;
NpInt _np_2;
_np_2 = _np_1->value;
return _np_2;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpFunction method_call_A_fn;
method_call_A_fn.addr = _np_0;
method_call_a = np_alloc(8);
NpInt _np_3;
_np_3 = 1;
method_call_a->value = _np_3;
method_call_A* _np_4;
_np_4 = method_call_a;
NpFunction _np_5;
_np_5 = method_call_A_fn;
NpContext _np_6;
_np_6 = _np_5.ctx;
_np_6.self = _np_4;
_np_5.ctx = _np_6;
method_call_x = ((NpInt (*)(NpContext))_np_5.addr)(_np_5.ctx);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0