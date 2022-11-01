
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="__init__", .length=8}};

// TYPEDEFS COMPILER SECTION
typedef struct { NpInt value; } object_member_op_assignments_A;

// DECLARATIONS COMPILER SECTION
object_member_op_assignments_A* object_member_op_assignments_a;
NpNone _np_0(NpContext __ctx__);
NpFunction object_member_op_assignments_A___init__;
NpFunction _np_6;

// FUNCTION DEFINITIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__) {
object_member_op_assignments_A* _np_1;
_np_1 = (object_member_op_assignments_A*)__ctx__.self;
NpInt _np_2;
_np_2 = 1;
NpInt _np_3;
_np_3 = _np_1->value;
NpInt _np_4;
_np_4 = _np_3 + _np_2;
_np_1->value = _np_4;
return 0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
object_member_op_assignments_A___init__.__addr__ = _np_0;
object_member_op_assignments_A___init__.__name__ = NOT_PYTHON_STRING_CONSTANTS[1];
object_member_op_assignments_a = np_alloc(8);
if (global_exception) {
return 1;
}
NpInt _np_5;
_np_5 = 1;
object_member_op_assignments_a->value = _np_5;
_np_6 = object_member_op_assignments_A___init__;
NpContext _np_7;
_np_7 = _np_6.__ctx__;
_np_7.self = object_member_op_assignments_a;
_np_6.__ctx__ = _np_7;
NpNone _np_8;
_np_8 = ((NpNone (*)(NpContext))_np_6.__addr__)(_np_6.__ctx__);
if (global_exception) {
return 1;
}
object_member_op_assignments_A* _np_9;
_np_9 = object_member_op_assignments_a;
NpInt _np_10;
_np_10 = 1;
NpInt _np_11;
_np_11 = _np_9->value;
NpInt _np_12;
_np_12 = _np_11 + _np_10;
_np_9->value = _np_12;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0