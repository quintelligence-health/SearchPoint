#!/bin/bash

echo 'should use node 0.12.7'

pushd . > /dev/null
cd node/SearchPointJs
node main.js config.json
popd
