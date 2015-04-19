# bson-ext

[![linux build status](https://secure.travis-ci.org/imlucas/bson-ext.png)](http://travis-ci.org/imlucas/bson-ext)
[![windows build status](https://ci.appveyor.com/api/projects/status/github/imlucas/bson-ext)](https://ci.appveyor.com/project/imlucas/bson-ext)

This module contains the C++ BSON parser only and is not meant to be used in
isolation from the [bson](http://npm.im/bson) NPM module. It lives in
it's own module so it can be an optional dependency for the
 [bson](http://npm.im/bson) module.


## Testing

```
npm test
```

## Prebuilt Binaries

bson-ext uses [node-pre-gyp](http://npm.im/node-pre-gyp) to publish and install
prebuilt binaries.  This means you don't need the full toolchain installed
and configured correctly to use this module.

## License

Apache 2
