// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION
typedef struct {PYINT x;PYINT y;} object_initialization6_A;

// VARIABLE DECLARATIONS COMPILER SECTION
object_initialization6_A* object_initialization6_a;

// FUNCTION DECLARATIONS COMPILER SECTION
void object_initialization6_A___init__(object_initialization6_A* self);

// FUNCTION DEFINITIONS COMPILER SECTION
void object_initialization6_A___init__(object_initialization6_A* self) {
object_initialization6_A* NP_var0 = self;
NP_var0->x = 1;
object_initialization6_A* NP_var1 = self;
NP_var1->y = 1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
object_initialization6_a = np_alloc(sizeof(object_initialization6_A));
object_initialization6_A___init__(object_initialization6_a);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0