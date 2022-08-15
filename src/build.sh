#!/bin/sh

set -xe

gcc -Wall -Wextra -g -o main lexer.c keywords.c operators.c test_lexer.c
