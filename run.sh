#!/bin/bash

echo 'should use node 6.10.0'

pushd . > /dev/null
cd node/SearchPointJs
node main.js config.json
popd
