
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpDict* dict_get_item_d;
NpInt dict_get_item_x;
NpInt dict_get_item_y;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
NpInt _np_0;
_np_0 = 1;
NpInt _np_1;
_np_1 = 2;
dict_get_item_d = np_dict_init(8, 8, np_void_int_eq);
if (global_exception) {
return 1;
}
np_dict_set_item(dict_get_item_d, &_np_0, &_np_1);
if (global_exception) {
return 1;
}
_np_0 = 3;
_np_1 = 4;
np_dict_set_item(dict_get_item_d, &_np_0, &_np_1);
if (global_exception) {
return 1;
}
NpDict* _np_2;
_np_2 = dict_get_item_d;
NpInt _np_3;
_np_3 = 1;
np_dict_get_val(_np_2, &_np_3, &dict_get_item_x);
if (global_exception) {
return 1;
}
NpDict* _np_4;
_np_4 = dict_get_item_d;
NpInt _np_5;
_np_5 = 3;
np_dict_get_val(_np_4, &_np_5, &dict_get_item_y);
if (global_exception) {
return 1;
}
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0