
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION
typedef struct { NpInt value; } object_member_op_assignments_A;

// DECLARATIONS COMPILER SECTION
object_member_op_assignments_A* object_member_op_assignments_a;
NpFunction object_member_op_assignments_A___init__;

// FUNCTION DEFINITIONS COMPILER SECTION
void* _np_0(NpContext __ctx__) {
object_member_op_assignments_A* self;
self = ((object_member_op_assignments_A*)__ctx__.self);
object_member_op_assignments_A* _np_1;
_np_1 = self;
NpInt _np_2;
_np_2 = 1;
NpInt _np_3;
_np_3 = _np_1->value;
_np_1->value = _np_3+_np_2;
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
object_member_op_assignments_A___init__.addr = &_np_0;
object_member_op_assignments_A* _np_4;
_np_4 = np_alloc(sizeof(object_member_op_assignments_A));
_np_4->value = 1;
NpContext _np_6;
_np_6 = object_member_op_assignments_A___init__.ctx;
_np_6.self = _np_4;
void* _np_5;
_np_5 = ((void* (*)(NpContext ctx))object_member_op_assignments_A___init__.addr)(_np_6);
object_member_op_assignments_a = _np_4;
object_member_op_assignments_A* _np_7;
_np_7 = object_member_op_assignments_a;
NpInt _np_8;
_np_8 = 1;
NpInt _np_9;
_np_9 = _np_7->value;
_np_7->value = _np_9+_np_8;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0