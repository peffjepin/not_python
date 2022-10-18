
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION
typedef struct { NpInt value; } method_call_A;

// DECLARATIONS COMPILER SECTION
method_call_A* method_call_a;
NpInt method_call_x;
NpFunction method_call_A_fn;

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt _np_0(NpContext __ctx__) {
method_call_A* self;
self = ((method_call_A*)__ctx__.self);
NpInt _np_1;
_np_1 = self->value;
return _np_1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
method_call_A_fn.addr = &_np_0;
method_call_A* _np_2;
_np_2 = np_alloc(sizeof(method_call_A));
NpInt _np_3;
_np_3 = 1;
_np_2->value = _np_3;
method_call_a = _np_2;
method_call_A* _np_5;
_np_5 = method_call_a;
NpFunction _np_6;
_np_6 = method_call_A_fn;
_np_6.ctx.self = _np_5;
NpFunction _np_4;
_np_4 = _np_6;
method_call_x = ((NpInt (*)(NpContext ctx))_np_4.addr)(_np_4.ctx);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0