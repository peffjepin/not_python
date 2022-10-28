
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpDict* dict_update_d1;
NpDict* dict_update_d2;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_0;
_np_0 = 1;
NpInt _np_1;
_np_1 = 2;
dict_update_d1 = np_dict_init(8, 8, np_void_int_eq);
np_dict_set_item(dict_update_d1, &_np_0, &_np_1);
NpInt _np_2;
_np_2 = 2;
NpInt _np_3;
_np_3 = 3;
dict_update_d2 = np_dict_init(8, 8, np_void_int_eq);
np_dict_set_item(dict_update_d2, &_np_2, &_np_3);
NpDict* _np_4;
_np_4 = dict_update_d1;
NpDict* _np_6;
_np_6 = dict_update_d2;
NpNone _np_5;
_np_5 = np_dict_update(_np_4, _np_6);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0