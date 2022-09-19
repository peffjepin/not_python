// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION
typedef struct {PYINT a, PYINT b} object_initialization2_A;

// VARIABLE DECLARATIONS COMPILER SECTION
object_initialization2_A* object_initialization2_a1;
object_initialization2_A* object_initialization2_a2;
object_initialization2_A* object_initialization2_a3;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
object_initialization2_a1 = np_alloc(sizeof(object_initialization2_A));
object_initialization2_a1->a = 1;
object_initialization2_a1->b = 1;
object_initialization2_a2 = np_alloc(sizeof(object_initialization2_A));
object_initialization2_a2->a = 2;
object_initialization2_a2->b = 1;
object_initialization2_a3 = np_alloc(sizeof(object_initialization2_A));
object_initialization2_a3->a = 3;
object_initialization2_a3->b = 3;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0