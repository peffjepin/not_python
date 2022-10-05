// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION
typedef struct {NpInt value;} object_op_assignment_A;

// VARIABLE DECLARATIONS COMPILER SECTION
NpFunction object_op_assignment_A___iadd__;
object_op_assignment_A* object_op_assignment_a;
NpInt NP_var5;

// FUNCTION DECLARATIONS COMPILER SECTION
object_op_assignment_A* NP_var0(object_op_assignment_A* self, NpInt other);

// FUNCTION DEFINITIONS COMPILER SECTION
object_op_assignment_A* NP_var0(object_op_assignment_A* self, NpInt other) {
object_op_assignment_A* NP_var1;
NP_var1 = self;
NpInt NP_var2;
NP_var2 = other;
NpInt NP_var3;
NP_var3 = NP_var1->value;
NP_var1->value = NP_var3+NP_var2;
object_op_assignment_A* NP_var4;
NP_var4 = self;
return NP_var4;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
object_op_assignment_A___iadd__.addr = &NP_var0;
object_op_assignment_a = np_alloc(sizeof(object_op_assignment_A));
object_op_assignment_a->value = 1;
NP_var5 = 1;
object_op_assignment_a = ((object_op_assignment_A* (*)(object_op_assignment_A*, NpInt))object_op_assignment_A___iadd__.addr)(object_op_assignment_a, NP_var5);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0