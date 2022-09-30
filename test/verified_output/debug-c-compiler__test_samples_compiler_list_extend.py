// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var0;
PyList* list_extend_l1;
PyInt NP_var1;
PyList* list_extend_l2;
PyList* NP_var3;
PyList* NP_var4;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
list_extend_l1 = LIST_INIT(PyInt, (PySortFunction)pyint_sort_fn, (PySortFunction)pyint_sort_fn_rev, (PyCompareFunction)void_int_eq);
list_append(list_extend_l1, &NP_var0);
NP_var0 = 2;
list_append(list_extend_l1, &NP_var0);
NP_var0 = 3;
list_append(list_extend_l1, &NP_var0);
NP_var1 = 4;
list_extend_l2 = LIST_INIT(PyInt, (PySortFunction)pyint_sort_fn, (PySortFunction)pyint_sort_fn_rev, (PyCompareFunction)void_int_eq);
list_append(list_extend_l2, &NP_var1);
NP_var1 = 5;
list_append(list_extend_l2, &NP_var1);
NP_var1 = 6;
list_append(list_extend_l2, &NP_var1);
NP_var3 = list_extend_l1;
NP_var4 = list_extend_l2;
list_extend(NP_var3, NP_var4);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0