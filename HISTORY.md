<a name="2.0.2"></a>
## [2.0.2](https://github.com/christkv/bson-ext/compare/v2.0.0...v2.0.2) (2019-04-08)


### Bug Fixes

* **checkKeys:** allow through certain keys beginning with $ ([8382dde](https://github.com/christkv/bson-ext/commit/8382dde))



<a name="2.0.0"></a>
# 2.0.0 (2018-03-02)


### Bug Fixes

* **db-ref:** correctly avoid parsing DBRef for invalid shapes ([fb58633](https://github.com/christkv/bson-ext/commit/fb58633))
* **db-ref:** support additional fields on DBRef type ([ff13e82](https://github.com/christkv/bson-ext/commit/ff13e82))
* **object-id:** move length assertion into ReadObjectId ([f3e7812](https://github.com/christkv/bson-ext/commit/f3e7812))
* **symbol:** upgrade deprecated symbol type to string ([5189f85](https://github.com/christkv/bson-ext/commit/5189f85))


### Features

* **mocha:** switch to using mocha for testing the module ([4d4a3a7](https://github.com/christkv/bson-ext/commit/4d4a3a7))
* **utf8-parsing:** add utf8 string validation ([6f01469](https://github.com/christkv/bson-ext/commit/6f01469))



1.0.5 2016-01-20
----------------
- Better identify Map instances, avoid throwing on anything containing an entries field, #37.

1.0.4 2016-01-11
----------------
- #204 remove Buffer.from as it's partially broken in early 4.x.x. series of node releases.

1.0.3 2016-01-03
----------------
- Fixed toString for ObjectId so it will work with inspect.

1.0.2 2016-01-02
----------------
- Minor optimizations for ObjectID to use Buffer.from where available.

1.0.1 2016-12-06
----------------
- Reverse behavior for undefined to be serialized as NULL. MongoDB 3.4 does not allow for undefined comparisons.

1.0.0 2016-12-06
----------------
- Introduced new BSON API and documentation.

0.1.13 2015-10-05
-----------------
- Upgrade to nan 2.0.9 (Issue #27, https://github.com/guymguym)
- Removed pre-gyp code as no longer needed.

0.1.12 2015-08-06
-----------------
- Undefined fields serialized as null values in arrays.

0.1.11 2015-08-06
-----------------
- Undefined fields are omitted from serialization.

0.1.10 2015-06-17
-----------------
- No longer print to console.error if driver did not load correctly.

0.1.9 2015-06-17
----------------
- Removed deprecated Node C++ method for Nan.h based ones.

0.1.8 2015-06-12
----------------
- Refactored to use single 16MB buffer for all serialization.

0.1.7 2015-05-15
----------------
- Attempt node-pre-gyp otherwise fallback to node-gyp or just fail.

0.1.6 2015-05-07
----------------
- Updated to use bundled node-pre-gyp as install method.

0.1.5 2015-05-07
----------------
- Updated npmignore to remove any non-needed artifacts.

0.1.4 2015-05-05
----------------
- Updated nan.h dependency to 1.8.x.

0.1.3 2015-04-23
----------------
- Windows only prebuilt support (Issue #6, https://github.com/imlucas)

0.1.2 2015-04-20
----------------
- Removed pre-packaged binaries from build.

0.1.1 2015-04-20
----------------
- Merged node-pre-gyp support (Issue #1, https://github.com/imlucas)

0.1.0 2015-03-26
----------------
- First pusht to npm, cleanup of project and left only c++ and test harnesses.
