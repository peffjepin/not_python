
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="__iadd__", .length=8}};

// TYPEDEFS COMPILER SECTION
typedef struct { NpInt value; } object_op_assignment_A;

// DECLARATIONS COMPILER SECTION
object_op_assignment_A* object_op_assignment_a;
object_op_assignment_A* _np_0(NpContext __ctx__, NpInt other);
NpFunction object_op_assignment_A___iadd__;
NpFunction _np_8;

// FUNCTION DEFINITIONS COMPILER SECTION
object_op_assignment_A* _np_0(NpContext __ctx__, NpInt other) {
object_op_assignment_A* _np_1;
_np_1 = (object_op_assignment_A*)__ctx__.self;
NpInt _np_2;
_np_2 = other;
NpInt _np_3;
_np_3 = _np_1->value;
NpInt _np_4;
_np_4 = _np_3 + _np_2;
_np_1->value = _np_4;
object_op_assignment_A* _np_5;
_np_5 = (object_op_assignment_A*)__ctx__.self;
return _np_5;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
object_op_assignment_A___iadd__.__addr__ = _np_0;
object_op_assignment_A___iadd__.__name__ = NOT_PYTHON_STRING_CONSTANTS[1];
if (global_exception) {
return 1;
}
object_op_assignment_a = np_alloc(8);
NpInt _np_6;
_np_6 = 1;
object_op_assignment_a->value = _np_6;
NpInt _np_7;
_np_7 = 1;
_np_8 = object_op_assignment_A___iadd__;
NpContext _np_9;
_np_9 = _np_8.__ctx__;
_np_9.self = object_op_assignment_a;
_np_8.__ctx__ = _np_9;
if (global_exception) {
return 1;
}
object_op_assignment_a = ((object_op_assignment_A* (*)(NpContext, NpInt))_np_8.__addr__)(_np_8.__ctx__, _np_7);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0