// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var1;
PyList* NP_var0;
PyInt NP_var3;
PyList* NP_var2;
PyList* list_addition_l;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var1 = 1;
NP_var0 = LIST_INIT(PyInt, (PySortFunction)pyint_sort_fn, (PySortFunction)pyint_sort_fn_rev, (PyCompareFunction)void_int_eq);
list_append(NP_var0, &NP_var1);
NP_var1 = 2;
list_append(NP_var0, &NP_var1);
NP_var1 = 3;
list_append(NP_var0, &NP_var1);
NP_var3 = 4;
NP_var2 = LIST_INIT(PyInt, (PySortFunction)pyint_sort_fn, (PySortFunction)pyint_sort_fn_rev, (PyCompareFunction)void_int_eq);
list_append(NP_var2, &NP_var3);
NP_var3 = 5;
list_append(NP_var2, &NP_var3);
NP_var3 = 6;
list_append(NP_var2, &NP_var3);
list_addition_l = list_add(NP_var0, NP_var2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0