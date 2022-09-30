// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var0;
PyList* list_literal_x;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
list_literal_x = LIST_INIT(PyInt, (PySortFunction)pyint_sort_fn, (PySortFunction)pyint_sort_fn_rev, (PyCompareFunction)void_int_eq);
list_append(list_literal_x, &NP_var0);
NP_var0 = 2;
list_append(list_literal_x, &NP_var0);
NP_var0 = 3;
list_append(list_literal_x, &NP_var0);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0