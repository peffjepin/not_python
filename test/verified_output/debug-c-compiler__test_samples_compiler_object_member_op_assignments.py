// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION
typedef struct {NpInt value;} object_member_op_assignments_A;

// VARIABLE DECLARATIONS COMPILER SECTION
NpFunction object_member_op_assignments_A___init__;
object_member_op_assignments_A* object_member_op_assignments_a;
object_member_op_assignments_A* NP_var5;
NpInt NP_var6;
NpInt NP_var7;

// FUNCTION DECLARATIONS COMPILER SECTION
void* NP_var0(NpContext ctx);

// FUNCTION DEFINITIONS COMPILER SECTION
void* NP_var0(NpContext ctx) {
object_member_op_assignments_A* self = ((object_member_op_assignments_A*)ctx.self);
object_member_op_assignments_A* NP_var1;
NP_var1 = self;
NpInt NP_var2;
NP_var2 = 1;
NpInt NP_var3;
NP_var3 = NP_var1->value;
NP_var1->value = NP_var3+NP_var2;
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
object_member_op_assignments_A___init__.addr = &NP_var0;
object_member_op_assignments_a = np_alloc(sizeof(object_member_op_assignments_A));
object_member_op_assignments_a->value = 1;
NpContext NP_var4 = object_member_op_assignments_A___init__.ctx;
NP_var4.self = object_member_op_assignments_a;
((void* (*)(NpContext ctx))object_member_op_assignments_A___init__.addr)(NP_var4);
NP_var5 = object_member_op_assignments_a;
NP_var6 = 1;
NP_var7 = NP_var5->value;
NP_var5->value = NP_var7+NP_var6;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0