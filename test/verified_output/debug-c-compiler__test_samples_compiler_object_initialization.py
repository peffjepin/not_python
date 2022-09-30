// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION
typedef struct {PyInt x;PyInt y;} object_initialization_A;

// VARIABLE DECLARATIONS COMPILER SECTION
object_initialization_A* object_initialization_a1;
object_initialization_A* object_initialization_a2;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
object_initialization_a1 = np_alloc(sizeof(object_initialization_A));
object_initialization_a1->x = 1;
object_initialization_a1->y = 1;
object_initialization_a2 = np_alloc(sizeof(object_initialization_A));
object_initialization_a2->y = 2;
object_initialization_a2->x = 2;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0