// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt NP_var0;
NpInt NP_var1;
NpDict* dict_update_d1;
NpInt NP_var2;
NpInt NP_var3;
NpDict* dict_update_d2;
NpDict* NP_var5;
NpDict* NP_var6;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
NP_var1 = 2;
dict_update_d1 = DICT_INIT(NpInt, NpInt, np_void_int_eq);
np_dict_set_item(dict_update_d1, &NP_var0, &NP_var1);
NP_var2 = 2;
NP_var3 = 3;
dict_update_d2 = DICT_INIT(NpInt, NpInt, np_void_int_eq);
np_dict_set_item(dict_update_d2, &NP_var2, &NP_var3);
NP_var5 = dict_update_d1;
NP_var6 = dict_update_d2;
np_dict_update(NP_var5, NP_var6);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0