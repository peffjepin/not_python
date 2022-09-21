// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION
typedef struct {PYINT value;} object_op_assignment_A;

// VARIABLE DECLARATIONS COMPILER SECTION
object_op_assignment_A* object_op_assignment_a;

// FUNCTION DECLARATIONS COMPILER SECTION
object_op_assignment_A* object_op_assignment_A___iadd__(object_op_assignment_A* self, PYINT other);

// FUNCTION DEFINITIONS COMPILER SECTION
object_op_assignment_A* object_op_assignment_A___iadd__(object_op_assignment_A* self, PYINT other) {
object_op_assignment_A* NP_var0 = self;
PYINT NP_var1 = NP_var0->value;
PYINT NP_var2 = other;
NP_var0->value = NP_var1+NP_var2;
object_op_assignment_A* NP_var3 = self;
return NP_var3;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
object_op_assignment_a = np_alloc(sizeof(object_op_assignment_A));
object_op_assignment_a->value = 1;
PYINT NP_var4 = 1;
object_op_assignment_a = object_op_assignment_A___iadd__(object_op_assignment_a, NP_var4);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0