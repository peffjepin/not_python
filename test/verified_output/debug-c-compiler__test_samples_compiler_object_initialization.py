// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION
typedef struct {PYINT x, PYINT y} A;

// VARIABLE DECLARATIONS COMPILER SECTION
A* a1;
A* a2;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
a1 = np_alloc(sizeof(A));
a1->x = 1;
a1->y = 1;
a2 = np_alloc(sizeof(A));
a2->y = 2;
a2->x = 2;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0