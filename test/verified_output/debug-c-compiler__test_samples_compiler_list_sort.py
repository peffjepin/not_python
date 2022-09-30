// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var0;
PyList* list_sort_l;
PyList* NP_var2;
PyList* NP_var4;
PyBool NP_var5;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
list_sort_l = LIST_INIT(PyInt, (PySortFunction)pyint_sort_fn, (PySortFunction)pyint_sort_fn_rev, (PyCompareFunction)void_int_eq);
list_append(list_sort_l, &NP_var0);
NP_var0 = 2;
list_append(list_sort_l, &NP_var0);
NP_var0 = 3;
list_append(list_sort_l, &NP_var0);
NP_var2 = list_sort_l;
list_sort(NP_var2, false);
NP_var4 = list_sort_l;
NP_var5 = true;
list_sort(NP_var4, NP_var5);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0