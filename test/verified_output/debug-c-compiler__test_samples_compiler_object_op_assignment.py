
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION
typedef struct { NpInt value; } object_op_assignment_A;

// DECLARATIONS COMPILER SECTION
object_op_assignment_A* object_op_assignment_a;
NpFunction object_op_assignment_A___iadd__;

// FUNCTION DEFINITIONS COMPILER SECTION
object_op_assignment_A* _np_0(NpContext __ctx__, NpInt other) {
object_op_assignment_A* self;
self = ((object_op_assignment_A*)__ctx__.self);
object_op_assignment_A* _np_1;
_np_1 = self;
NpInt _np_2;
_np_2 = other;
NpInt _np_3;
_np_3 = _np_1->value;
_np_1->value = _np_3+_np_2;
object_op_assignment_A* _np_4;
_np_4 = self;
return _np_4;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
object_op_assignment_A___iadd__.addr = &_np_0;
object_op_assignment_A* _np_5;
_np_5 = np_alloc(sizeof(object_op_assignment_A));
NpInt _np_6;
_np_6 = 1;
_np_5->value = _np_6;
object_op_assignment_a = _np_5;
NpInt _np_7;
_np_7 = 1;
NpContext _np_8;
_np_8 = object_op_assignment_A___iadd__.ctx;
_np_8.self = object_op_assignment_a;
object_op_assignment_a = ((object_op_assignment_A* (*)(NpContext ctx, NpInt))object_op_assignment_A___iadd__.addr)(_np_8, _np_7);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0