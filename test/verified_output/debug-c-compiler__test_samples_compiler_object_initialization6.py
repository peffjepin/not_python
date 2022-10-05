// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION
typedef struct {NpInt x;NpInt y;} object_initialization6_A;

// VARIABLE DECLARATIONS COMPILER SECTION
NpFunction object_initialization6_A___init__;
object_initialization6_A* object_initialization6_a;

// FUNCTION DECLARATIONS COMPILER SECTION
void* NP_var0(object_initialization6_A* self);

// FUNCTION DEFINITIONS COMPILER SECTION
void* NP_var0(object_initialization6_A* self) {
object_initialization6_A* NP_var1;
NP_var1 = self;
NP_var1->x = 1;
object_initialization6_A* NP_var2;
NP_var2 = self;
NP_var2->y = 1;
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
object_initialization6_A___init__.addr = &NP_var0;
object_initialization6_a = np_alloc(sizeof(object_initialization6_A));
((void* (*)(object_initialization6_A*))object_initialization6_A___init__.addr)(object_initialization6_a);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0