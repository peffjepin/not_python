// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION
typedef struct {NpInt value;} object_member_op_assignments_A;

// VARIABLE DECLARATIONS COMPILER SECTION
object_member_op_assignments_A* object_member_op_assignments_a;
object_member_op_assignments_A* NP_var3;
NpInt NP_var4;
NpInt NP_var5;

// FUNCTION DECLARATIONS COMPILER SECTION
void* object_member_op_assignments_A___init__(object_member_op_assignments_A* self);

// FUNCTION DEFINITIONS COMPILER SECTION
void* object_member_op_assignments_A___init__(object_member_op_assignments_A* self) {
object_member_op_assignments_A* NP_var0;
NP_var0 = self;
NpInt NP_var1;
NP_var1 = 1;
NpInt NP_var2;
NP_var2 = NP_var0->value;
NP_var0->value = NP_var2+NP_var1;
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
object_member_op_assignments_a = np_alloc(sizeof(object_member_op_assignments_A));
object_member_op_assignments_a->value = 1;
object_member_op_assignments_A___init__(object_member_op_assignments_a);
NP_var3 = object_member_op_assignments_a;
NP_var4 = 1;
NP_var5 = NP_var3->value;
NP_var3->value = NP_var5+NP_var4;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0