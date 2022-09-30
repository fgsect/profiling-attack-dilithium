#!/bin/bash
git clone https://github.com/newaetech/chipwhisperer.git
cd chipwhisperer && git checkout 0ae262015ad83f8d04ffb6caeb8e4808145e1e8a && cd ../
mkdir chipwhisperer/victims
cp -r unpack_xof chipwhisperer/victims/unpack_xof
