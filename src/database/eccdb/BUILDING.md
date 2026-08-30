# Standalone build

The eccdb directory can be configured without building the complete
ecc-tools application.

## Core EnTTDB

```bash
cmake -S src/database/eccdb -B build/eccdb-core -G Ninja \
  -DECCDB_BUILD_TESTS=ON

# Build only the EnTTDB libraries.
cmake --build build/eccdb-core --target eccdb_database --parallel 128

# Build and run the standalone core tests.
cmake --build build/eccdb-core --parallel 128
ctest --test-dir build/eccdb-core --output-on-failure --parallel 128
```

This builds the EnTTDB storage, binary persistence, exporters, and core unit
tests. It does not build a LEF or DEF parser.

## Direct LEF/DEF and OpenDB differential tests

```bash
cmake -S src/database/eccdb -B build/eccdb-differential -G Ninja \
  -DECCDB_BUILD_TESTS=ON \
  -DECCDB_STANDALONE_LEF_DEF=ON \
  -DOPENROAD_SOURCE_DIR=/path/to/OpenROAD \
  -DOPENDB_PYTHON_MODULE_DIR=/path/to/OpenROAD/bazel-bin/src/odb
cmake --build build/eccdb-differential \
  --target eccdb_differential_tests --parallel 128
ctest --test-dir build/eccdb-differential --output-on-failure \
  --parallel 128 --label-regex eccdb_differential
```

This configuration adds only the checked-in SI2 LEF/DEF parsers. OpenDB is a
runtime differential oracle and is not linked into EnTTDB; its paths are
optional, and tests that require unavailable external corpora are skipped.

Every standalone configure emits `compile_commands.json` in its build
directory for clangd.

## Legacy iDB LEF adapter differential tests

```bash
cmake -S src/database/eccdb -B build/eccdb-legacy-differential -G Ninja \
  -DECCDB_BUILD_TESTS=ON \
  -DECCDB_STANDALONE_LEGACY_IDB=ON
cmake --build build/eccdb-legacy-differential \
  --target eccdb_differential_tests --parallel 128
ctest --test-dir build/eccdb-legacy-differential --output-on-failure \
  --parallel 128 --label-regex eccdb_differential
```

This option implies `ECCDB_STANDALONE_LEF_DEF`. It adds only the legacy
layout objects, geometry, LEF service, and LEF builder required to compare the
direct EnTT importer with the legacy iDB materialization path. It does not add
the complete `IdbBuilder` or the DEF, Verilog, GDS, JSON, GUI, Python, iRT, and
iDRC target trees.
