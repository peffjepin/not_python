def my_function(value: str):
    print(value)


fnref: Function[[str], None] = my_function
fnref("hello")
