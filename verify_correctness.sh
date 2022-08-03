#!/bin/bash

set -eu
set -o pipefail

numdiff vectorized_output.txt \
    scalar_output.txt