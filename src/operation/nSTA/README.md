# nSTA

`nSTA` follows the operation-tool layout used by hand-written `iRT`.

Current skeleton:

```text
src/operation/nSTA/
  interface/
  source/
    data_manager/
    module/
    toolkit/
      logger/
      monitor/
      utility/
  test/
```

Keep command and external API boundaries in `interface/`. Keep shared nSTA
state in `source/data_manager/`. Add concrete algorithm stages under
`source/module/<stage>/` only when the stage design is known.
