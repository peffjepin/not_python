
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpDict* dict_get_item_d;
NpInt dict_get_item_x;
NpInt dict_get_item_y;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_0;
NpInt _np_1;
_np_0 = 1;
_np_1 = 2;
NpDict* _np_2;
_np_2 = DICT_INIT(NpInt, NpInt, np_void_int_eq);
np_dict_set_item(_np_2, &_np_0, &_np_1);
_np_0 = 3;
_np_1 = 4;
np_dict_set_item(_np_2, &_np_0, &_np_1);
dict_get_item_d = _np_2;
NpInt _np_4;
_np_4 = 1;
NpInt _np_3;
_np_3 = _np_4;
NpInt _np_5;
np_dict_get_val(dict_get_item_d, &_np_3, &_np_5);
dict_get_item_x = _np_5;
NpInt _np_7;
_np_7 = 3;
NpInt _np_6;
_np_6 = _np_7;
NpInt _np_8;
np_dict_get_val(dict_get_item_d, &_np_6, &_np_8);
dict_get_item_y = _np_8;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0