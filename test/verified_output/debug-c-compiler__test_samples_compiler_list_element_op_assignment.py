// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var0;
PyList* list_element_op_assignment_l;
PyList* NP_var1;
PyInt NP_var2;
PyInt NP_var4;
PyInt NP_var5;
PyInt NP_var3;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
list_element_op_assignment_l = LIST_INIT(PyInt, (PySortFunction)pyint_sort_fn, (PySortFunction)pyint_sort_fn_rev, (PyCompareFunction)void_int_eq);
list_append(list_element_op_assignment_l, &NP_var0);
NP_var0 = 2;
list_append(list_element_op_assignment_l, &NP_var0);
NP_var0 = 3;
list_append(list_element_op_assignment_l, &NP_var0);
NP_var1 = list_element_op_assignment_l;
NP_var2 = 0;
list_get_item(NP_var1, NP_var2, &NP_var4);
NP_var5 = 1;
NP_var3 = NP_var4+NP_var5;
list_set_item(NP_var1, NP_var2, &NP_var3);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0