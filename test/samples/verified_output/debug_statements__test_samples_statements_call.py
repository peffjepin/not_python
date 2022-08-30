(my_function __call__ (my_arg))
(multiple_args __call__ (1, 2, 3, 4))
(kwargs __call__ (arg1=1, arg2=2))
(mixed __call__ (1, 2, arg3=3, arg4=4))
(expression_args __call__ (((1 + 2) + 3), (3 ** 9)))
(expression_kwargs __call__ (x=(1 + 2), y=(3 + 4)))
(expression_mixed __call__ ((1 + 2), y=(3 + 4)))
EOF

exitcode=0