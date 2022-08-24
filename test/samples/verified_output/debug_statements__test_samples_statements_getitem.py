EXPR: (dictionary __getitem__ some_key)
EXPR: (dictionary __getitem__ (some_key + some_postfix))
EXPR: (arr __getitem__ slice())
EXPR: (arr __getitem__ slice())
EXPR: (arr __getitem__ slice(start=1))
EXPR: (arr __getitem__ slice(start=1))
EXPR: (arr __getitem__ slice(start=1, stop=2))
EXPR: (arr __getitem__ slice(start=1, stop=2))
EXPR: (arr __getitem__ slice(start=1, stop=2, step=3))
EXPR: (arr __getitem__ slice(stop=1))
EXPR: (arr __getitem__ slice(stop=1))
EXPR: (arr __getitem__ slice(stop=1, step=2))
EXPR: (arr __getitem__ slice(step=1))
EXPR: (arr __getitem__ slice(start=1, step=2))
EXPR: (arr __getitem__ slice(start=(1 + 2), stop=(3 + 4), step=(5 + 6)))
EOF

exitcode=0