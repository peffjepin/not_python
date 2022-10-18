
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpDict* dict_update_d1;
NpDict* dict_update_d2;

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
dict_update_d1 = _np_2;
NpInt _np_3;
NpInt _np_4;
_np_3 = 2;
_np_4 = 3;
NpDict* _np_5;
_np_5 = DICT_INIT(NpInt, NpInt, np_void_int_eq);
np_dict_set_item(_np_5, &_np_3, &_np_4);
dict_update_d2 = _np_5;
NpDict* _np_8;
_np_8 = dict_update_d1;
NpDict* _np_9;
_np_9 = dict_update_d2;
void* _np_6;
_np_6 = np_dict_update(_np_8, _np_9);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0