
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION
typedef struct { NpInt value; } object_op_assignment_A;

// DECLARATIONS COMPILER SECTION
object_op_assignment_A* object_op_assignment_a;

// FUNCTION DEFINITIONS COMPILER SECTION
object_op_assignment_A* _np_0(NpContext __ctx__, NpInt other) {
object_op_assignment_A* self;
self = __ctx__.self;
object_op_assignment_A* _np_1;
_np_1 = self;
NpInt _np_2;
_np_2 = other;
NpInt _np_3;
_np_3 = _np_1->value;
NpInt _np_4;
_np_4 = _np_3 + _np_2;
_np_1->value = _np_4;
object_op_assignment_A* _np_5;
_np_5 = self;
return _np_5;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpFunction object_op_assignment_A___iadd__;
object_op_assignment_A___iadd__.addr = _np_0;
object_op_assignment_a = np_alloc(8);
NpInt _np_6;
_np_6 = 1;
object_op_assignment_a->value = _np_6;
NpInt _np_7;
_np_7 = 1;
NpFunction _np_8;
_np_8 = object_op_assignment_A___iadd__;
NpContext _np_9;
_np_9 = _np_8.ctx;
_np_9.self = object_op_assignment_a;
_np_8.ctx = _np_9;
object_op_assignment_a = ((object_op_assignment_A* (*)(NpContext, NpInt))_np_8.addr)(_np_8.ctx, _np_7);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0