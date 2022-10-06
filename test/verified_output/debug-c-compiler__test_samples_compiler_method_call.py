// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION
typedef struct {NpInt value;} method_call_A;

// VARIABLE DECLARATIONS COMPILER SECTION
NpFunction method_call_A_fn;
method_call_A* method_call_a;
method_call_A* NP_var3;
NpFunction NP_var2;
NpInt method_call_x;

// FUNCTION DECLARATIONS COMPILER SECTION
NpInt NP_var0(NpContext ctx);

// FUNCTION DEFINITIONS COMPILER SECTION
NpInt NP_var0(NpContext ctx) {
method_call_A* self = ((method_call_A*)ctx.self);
NpInt NP_var1;
NP_var1 = self->value;
return NP_var1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
method_call_A_fn.addr = &NP_var0;
method_call_a = np_alloc(sizeof(method_call_A));
method_call_a->value = 1;
NP_var3 = method_call_a;
NP_var2 = method_call_A_fn;
NP_var2.ctx.self = NP_var3;
method_call_x = ((NpInt (*)(NpContext ctx))NP_var2.addr)(NP_var2.ctx);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0