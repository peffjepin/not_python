// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt NP_var0;
NpList* list_extend_l1;
NpInt NP_var1;
NpList* list_extend_l2;
NpList* NP_var3;
NpList* NP_var4;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
list_extend_l1 = LIST_INIT(NpInt, (NpSortFunction)np_int_sort_fn, (NpSortFunction)np_int_sort_fn_rev, (NpCompareFunction)np_void_int_eq);
np_list_append(list_extend_l1, &NP_var0);
NP_var0 = 2;
np_list_append(list_extend_l1, &NP_var0);
NP_var0 = 3;
np_list_append(list_extend_l1, &NP_var0);
NP_var1 = 4;
list_extend_l2 = LIST_INIT(NpInt, (NpSortFunction)np_int_sort_fn, (NpSortFunction)np_int_sort_fn_rev, (NpCompareFunction)np_void_int_eq);
np_list_append(list_extend_l2, &NP_var1);
NP_var1 = 5;
np_list_append(list_extend_l2, &NP_var1);
NP_var1 = 6;
np_list_append(list_extend_l2, &NP_var1);
NP_var3 = list_extend_l1;
NP_var4 = list_extend_l2;
np_list_extend(NP_var3, NP_var4);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0