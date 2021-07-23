
# SearchPoint

## Prerequisites

Make sure the following are installed:

- Git
- Node 8
- `node-gyp` which can be installed with `npm install -g node-gyp`
- `lapacke` which can be installed with `apt-get install liblapacke-dev`
- `uuid` which can be installed with `apt-get install uuid-dev`

## Installation Instructions

To setup SearchPoint first clone all the Git modules and sub-modules:
``` 
git clone git@github.com:lstopar/SearchPoint.git
cd SearchPoint
git submodule update --init --recursive
```

Then compile SearchPoint with the following commands:
```
cd SearchPoint
npm install
node-gyp configure
node-gyp build --jobs 20
```

Install the dependencies:
```
cd SearchPointSrv
npm install
```

Add SearchPoint to SearchPointSrv as a library:
```
cd SearchPointSrv/node_modules
ln -s ../../SearchPoint searchpoint
```

## Running SearchPoint

To run SearchPoint go into the `bin` directory and run:
```
   ./run-demo
```

## Creating API Keys

SearchPoint uses the Bing API to obtain the search results. To create an API key,
login to the Azure marketplace `https://portal.azure.com/#home` with account
`mateja.skraba@ijs.si`. Then create a Cognitive Services service and finally the
Bing Service.

Finally click on the created resource, where you will find the API key.
