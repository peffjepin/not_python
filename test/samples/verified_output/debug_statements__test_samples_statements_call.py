EXPR: (my_function __call__ (my_arg))
EXPR: (multiple_args __call__ (1, 2, 3, 4))
EXPR: (kwargs __call__ (arg1=1, arg2=2))
EXPR: (mixed __call__ (1, 2, arg3=3, arg4=4))
EXPR: (expression_args __call__ (((1 + 2) + 3), (3 ** 9)))
EXPR: (expression_kwargs __call__ (x=(1 + 2), y=(3 + 4)))
EXPR: (expression_mixed __call__ ((1 + 2), y=(3 + 4)))
EOF

exitcode=0