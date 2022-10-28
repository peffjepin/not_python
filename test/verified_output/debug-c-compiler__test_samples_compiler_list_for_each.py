
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpList* list_for_each_my_list;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_0;
_np_0 = 1;
list_for_each_my_list = np_list_init(8, np_int_sort_fn, np_int_sort_fn_rev, np_void_int_eq);
np_list_append(list_for_each_my_list, &_np_0);
_np_0 = 2;
np_list_append(list_for_each_my_list, &_np_0);
_np_0 = 3;
np_list_append(list_for_each_my_list, &_np_0);
NpList* _np_2;
_np_2 = list_for_each_my_list;
NpIter _np_3;
_np_3 = np_list_iter(_np_2);
NpInt _np_4;
while(1) {
_np_3.next_data = _np_3.next(_np_3.iter);
if (_np_3.next_data) _np_4 = *((NpInt*)_np_3.next_data);
NpPointer _np_5;
_np_5 = _np_3.next_data;
if (!_np_5) {
break;
}
NpInt _np_7;
_np_7 = _np_4;
NpString _np_8;
_np_8 = np_int_to_str(_np_7);
NpNone _np_6;
_np_6 = builtin_print(1, _np_8);
_np_1:
}
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0