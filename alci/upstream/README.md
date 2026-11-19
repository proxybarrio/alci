# Patches prepared for ELKS

Changes that also apply to upstream ELKS (`ghaerr/elks`), kept here as
plain patches against ELKS master with the measurements that support them.
They are not sent by the agent: the maintainer decides and sends them.

| File | Against | What | Measured |
| --- | --- | --- | --- |
| `exec-round-dseg.patch` | `df2a61c5` | `exec`: data segment rounded up to 256 bytes, so a program's freed segment fits the next exec of it | 86Box IBM AT 512K: 6 of 8 background `sh -c` failed with "not enough memory", now none; +17 bytes of code, about 128 bytes per process (task S3, `alci/RESULTS.md`) |
