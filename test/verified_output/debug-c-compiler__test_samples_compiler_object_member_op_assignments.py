// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION
typedef struct {PYINT value;} object_member_op_assignments_A;

// VARIABLE DECLARATIONS COMPILER SECTION
object_member_op_assignments_A* object_member_op_assignments_a;

// FUNCTION DECLARATIONS COMPILER SECTION
void object_member_op_assignments_A___init__(object_member_op_assignments_A* self);

// FUNCTION DEFINITIONS COMPILER SECTION
void object_member_op_assignments_A___init__(object_member_op_assignments_A* self) {
object_member_op_assignments_A* NP_var0 = self;
PYINT NP_var1 = NP_var0->value;
PYINT NP_var2 = 1;
NP_var0->value = NP_var1+NP_var2;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
object_member_op_assignments_a = np_alloc(sizeof(object_member_op_assignments_A));
object_member_op_assignments_a->value = 1;
object_member_op_assignments_A___init__(object_member_op_assignments_a);
object_member_op_assignments_A* NP_var3 = object_member_op_assignments_a;
PYINT NP_var4 = NP_var3->value;
PYINT NP_var5 = 1;
NP_var3->value = NP_var4+NP_var5;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0