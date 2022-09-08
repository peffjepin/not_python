(dictionary __getitem__ some_key)
(dictionary __getitem__ (some_key + some_postfix))
(arr __getitem__ slice())
(arr __getitem__ slice())
(arr __getitem__ slice(start=1))
(arr __getitem__ slice(start=1))
(arr __getitem__ slice(start=1, stop=2))
(arr __getitem__ slice(start=1, stop=2))
(arr __getitem__ slice(start=1, stop=2, step=3))
(arr __getitem__ slice(stop=1))
(arr __getitem__ slice(stop=1))
(arr __getitem__ slice(stop=1, step=2))
(arr __getitem__ slice(step=1))
(arr __getitem__ slice(start=1, step=2))
(arr __getitem__ slice(start=(1 + 2), stop=(3 + 4), step=(5 + 6)))
EOF

exitcode=0