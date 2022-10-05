// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt NP_var0;
NpInt NP_var1;
NpDict* dict_get_item_d;
NpInt NP_var2;
NpInt dict_get_item_x;
NpInt NP_var3;
NpInt dict_get_item_y;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
NP_var1 = 2;
dict_get_item_d = DICT_INIT(NpInt, NpInt, np_void_int_eq);
np_dict_set_item(dict_get_item_d, &NP_var0, &NP_var1);
NP_var0 = 3;
NP_var1 = 4;
np_dict_set_item(dict_get_item_d, &NP_var0, &NP_var1);
NP_var2 = 1;
np_dict_get_val(dict_get_item_d, &NP_var2, &dict_get_item_x);
NP_var3 = 3;
np_dict_get_val(dict_get_item_d, &NP_var3, &dict_get_item_y);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0