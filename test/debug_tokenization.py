for i in range(3):
    print(i)

my_list = [
    x for x in range(10) if x == 5
]

list_with_literals = [1, 2, 3]

x = .111
x = 1.111
x = 0000

# comment on start of line
y = 1  # comment at end of line

string = "hello, world"
string = '"hello, world"'
empty = ""
string_with_escaped_quotes = "\"hello!\""

no_newlines_between_parens = (
    1,
    2,
    3,
)
no_newlines_between_square = [
    1,
    2,
    3,
]
no_newlines_between_curly = {
    1: 1,
    2: 2,
    3: 3,
}
no_newlines_nested = (
    [
        1,
        2,
        3
    ],
    {
        1: 1,
        2: 2,
    },
)
