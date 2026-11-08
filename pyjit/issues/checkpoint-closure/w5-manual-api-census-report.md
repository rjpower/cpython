# Manual value API ownership marking census

This is a declaration/callsite census only. No callers were ported in this
packet. The 15 reviewed object API declarations are marked with their
`Py_VALUE_COMPAT` value counterparts, and six new `[[manual]]` entries were
appended to the isolated `Tools/build/value_ownership.toml` table.

The isolated source tree is `build/checkpoint-bulk-20261105` at frozen
candidate `f65d64a3987` before these census edits. The existing clang-21
build was run with `make -j8 -k all` and then serially with `make -j1 -k
all`, using `-ferror-limit=0`. The serial census exited 2 because marked
object compatibility APIs are intentionally unavailable in value-native
translation units; it found 459 such diagnostics in 13 of the 15 APIs.

| Marked object API | Value API | Calls | Files |
| --- | --- | ---: | ---: |
| `PyObject_GetOptionalAttr` | `_PyValue_GetOptionalAttr` | 124 | 40 |
| `_PyObject_LookupSpecial` | `_PyValue_LookupSpecial` | 14 | 9 |
| `PyMapping_GetOptionalItem` | `_PyValue_GetOptionalItem` | 21 | 6 |
| `PyIter_Next` | `_PyValue_IterNext` | 56 | 25 |
| `PyIter_Send` | `_PyValue_Send` | 1 | 1 |
| `PyObject_Call` | `_PyValue_Call` | 37 | 28 |
| `PyObject_CallOneArg` | `_PyValue_CallOneArg` | 112 | 52 |
| `PyObject_Vectorcall` | `_PyValue_Vectorcall` | 37 | 23 |
| `PyObject_CallNoArgs` | `_PyValue_CallNoArgs` | 29 | 17 |
| `PyMapping_GetOptionalItemString` | `_PyValue_MappingGetOptionalItemString` | 3 | 2 |
| `PyIter_NextItem` | `_PyValue_IterNext` | 4 | 2 |
| `_PyMapping_GetOptionalItem2` | `_PyValue_GetOptionalItem` | 0 | 0 |
| `_PyType_LookupRef` | `_PyType_LookupValue` | 17 | 7 |
| `_PySuper_Lookup` | `_PySuper_LookupValue` | 0 | 0 |
| `_PyObject_Call` | `_PyValue_Call` | 4 | 3 |

## Exact compiler sites by object API

### `PyObject_GetOptionalAttr` → `_PyValue_GetOptionalAttr` (124 sites)

- `Modules/_abc.c`: 477, 493, 748
- `Modules/_asynciomodule.c`: 341, 3416
- `Modules/_csv.c`: 1730
- `Modules/_ctypes/_ctypes.c`: 1138, 1496, 1785, 1818, 2008, 2112, 2287, 2472, 2697, 2870, 2959, 3896
- `Modules/_ctypes/callproc.c`: 810
- `Modules/_ctypes/stgdict.c`: 193
- `Modules/_datetimemodule.c`: 4411
- `Modules/_io/bufferedio.c`: 1579, 1754
- `Modules/_io/fileio.c`: 1271
- `Modules/_io/iobase.c`: 220, 348, 633
- `Modules/_io/textio.c`: 1106, 1370, 3247, 3263, 3518
- `Modules/_pickle.c`: 1282, 1930, 1933, 1936, 1939, 2195, 2245, 4211, 4533, 4619, 5258, 5270, 5406, 7492, 7704
- `Modules/_sqlite/microprotocols.c`: 122, 141
- `Modules/_threadmodule.c`: 2354
- `Modules/arraymodule.c`: 2614
- `Modules/pyexpat.c`: 1073
- `Objects/abstract.c`: 376, 3597, 3696, 3714
- `Objects/call.c`: 176
- `Objects/classobject.c`: 313, 315, 546
- `Objects/descrobject.c`: 2046, 2272
- `Objects/exceptions.c`: 337, 1243
- `Objects/fileobject.c`: 188
- `Objects/funcobject.c`: 1413
- `Objects/genericaliasobject.c`: 247, 318, 395, 417, 507, 604
- `Objects/genobject.c`: 484, 735
- `Objects/lazyimportobject.c`: 107, 341
- `Objects/moduleobject.c`: 1239, 1258, 1270, 1284
- `Objects/object.c`: 1032, 1041, 1313, 1516
- `Objects/odictobject.c`: 2404, 2437
- `Objects/typeobject.c`: 3687, 7681, 7692, 8698, 9107, 9267, 9287, 12476, 12484
- `Objects/typevarobject.c`: 313, 319
- `Python/Python-ast.c`: 1691, 1818, 1833, 1965, 1968, 1971
- `Python/_warnings.c`: 261, 1371, 1400
- `Python/bltinmodule.c`: 58, 211, 1491
- `Python/ceval.c`: 592, 4543, 4549, 4583
- `Python/codecs.c`: 570
- `Python/import.c`: 330, 4386, 4403, 4494
- `Python/intrinsics.c`: 50, 54
- `Python/pythonrun.c`: 243, 867
- `Python/sysmodule.c`: 339, 732

### `_PyObject_LookupSpecial` → `_PyValue_LookupSpecial` (14 sites)

- `Modules/posixmodule.c`: 1518, 17900
- `Objects/abstract.c`: 311
- `Objects/bytesobject.c`: 656, 3102
- `Objects/complexobject.c`: 532
- `Objects/dictobject.c`: 3698
- `Objects/object.c`: 934, 2324
- `Objects/typeobject.c`: 8825, 8879, 14879
- `Objects/weakrefobject.c`: 233
- `Python/sysmodule.c`: 2045

### `PyMapping_GetOptionalItem` → `_PyValue_GetOptionalItem` (21 sites)

- `Modules/_pickle.c`: 5235
- `Objects/lazyimportobject.c`: 102, 113, 449, 670, 711
- `Objects/unicodeobject.c`: 8914, 9230, 9712
- `Python/ceval.c`: 3830, 3973, 4382, 4437, 4503, 4988, 4995, 5060, 5081
- `Python/import.c`: 274, 415
- `Python/pystate.c`: 1132

### `PyIter_Next` → `_PyValue_IterNext` (56 sites)

- `Modules/_abc.c`: 492
- `Modules/_collectionsmodule.c`: 1757, 1760, 2677, 2723
- `Modules/_csv.c`: 1078, 1480, 1596
- `Modules/_functoolsmodule.c`: 1260
- `Modules/_io/bytesio.c`: 936
- `Modules/_io/iobase.c`: 801, 858
- `Modules/_pickle.c`: 2303, 3377, 3398, 3408, 3451, 3666, 3691, 3706, 3746, 4061, 6575
- `Modules/_sqlite/cursor.c`: 932
- `Modules/_zoneinfo.c`: 612
- `Modules/itertoolsmodule.c`: 1364, 2084
- `Modules/mathmodule.c`: 1513, 2997, 3042, 3086
- `Modules/posixmodule.c`: 1815, 9328
- `Objects/bytearrayobject.c`: 2379
- `Objects/bytesobject.c`: 3246
- `Objects/dictobject.c`: 3962, 4159, 6035, 6264, 6474
- `Objects/frameobject.c`: 400
- `Objects/namespaceobject.c`: 143
- `Objects/odictobject.c`: 2339, 2346, 2354, 2362, 2419
- `Objects/setobject.c`: 1946
- `Objects/templateobject.c`: 27, 33, 38
- `Objects/weakrefobject.c`: 888
- `Python/ast.c`: 180
- `Python/pylifecycle.c`: 1853
- `Python/symtable.c`: 1152
- `Python/sysmodule.c`: 337

### `PyIter_Send` → `_PyValue_Send` (1 sites)

- `Modules/_asynciomodule.c`: 3237

### `PyObject_Call` → `_PyValue_Call` (37 sites)

- `Modules/_ctypes/callproc.c`: 2218
- `Modules/_cursesmodule.c`: 5169
- `Modules/_datetimemodule.c`: 1321
- `Modules/_functoolsmodule.c`: 763, 1278, 1467, 1498, 1794
- `Modules/_interpretersmodule.c`: 646
- `Modules/_operator.c`: 1890
- `Modules/_pickle.c`: 4713
- `Modules/_posixsubprocess.c`: 829
- `Modules/_threadmodule.c`: 423
- `Modules/_tkinter.c`: 2706
- `Modules/_zoneinfo.c`: 827
- `Modules/atexitmodule.c`: 165
- `Modules/itertoolsmodule.c`: 1953
- `Modules/main.c`: 387, 440
- `Modules/pyexpat.c`: 334
- `Modules/timemodule.c`: 1136
- `Objects/call.c`: 325
- `Objects/classobject.c`: 496
- `Objects/funcobject.c`: 1903
- `Objects/genericaliasobject.c`: 760
- `Objects/genobject.c`: 841
- `Objects/typevarobject.c`: 2500
- `Objects/weakrefobject.c`: 706
- `Python/Python-ast.c`: 1995
- `Python/ceval.c`: 3680
- `Python/codecs.c`: 449, 497
- `Python/crossinterp.c`: 53, 1167
- `Python/errors.c`: 47, 280, 935

### `PyObject_CallOneArg` → `_PyValue_CallOneArg` (112 sites)

- `Modules/_asynciomodule.c`: 275, 717, 761, 1698, 1762, 2622, 3014, 3149
- `Modules/_bisectmodule.c`: 114, 254, 322, 464
- `Modules/_collectionsmodule.c`: 685
- `Modules/_csv.c`: 1558
- `Modules/_ctypes/callproc.c`: 1106, 1331
- `Modules/_elementtree.c`: 3194, 3557, 3635, 3666, 3776, 3805, 3881, 3925
- `Modules/_interpchannelsmodule.c`: 2807
- `Modules/_interpqueuesmodule.c`: 394, 1387
- `Modules/_io/iobase.c`: 648
- `Modules/_json.c`: 873, 880, 962, 991, 1087, 1648
- `Modules/_pickle.c`: 416, 2812, 3019, 5197, 7101, 7123
- `Modules/_randommodule.c`: 376
- `Modules/_sqlite/connection.c`: 595, 1615
- `Modules/_sqlite/cursor.c`: 422
- `Modules/_sqlite/microprotocols.c`: 116, 126, 145
- `Modules/_sre/sre.c`: 1524
- `Modules/_struct.c`: 2782
- `Modules/_uuidmodule.c`: 325, 1747
- `Modules/cjkcodecs/cjkcodecs.h`: 347, 347, 347, 347, 347, 347
- `Modules/cjkcodecs/multibytecodec.c`: 157
- `Modules/itertoolsmodule.c`: 1510, 1636, 3569
- `Modules/pyexpat.c`: 204
- `Objects/abstract.c`: 381
- `Objects/bytearrayobject.c`: 160, 2822
- `Objects/bytesobject.c`: 2769
- `Objects/complexobject.c`: 1409
- `Objects/descrobject.c`: 2082, 2134
- `Objects/dictobject.c`: 3546, 3705, 7810
- `Objects/exceptions.c`: 1348
- `Objects/fileobject.c`: 133
- `Objects/floatobject.c`: 1647, 1848
- `Objects/funcobject.c`: 592
- `Objects/genericaliasobject.c`: 631
- `Objects/genobject.c`: 130, 1791
- `Objects/lazyimportobject.c`: 356
- `Objects/longobject.c`: 6518
- `Objects/memoryobject.c`: 2381, 2420
- `Objects/moduleobject.c`: 1454, 1817
- `Objects/namespaceobject.c`: 79
- `Objects/odictobject.c`: 913
- `Objects/typeobject.c`: 2446, 3386, 13088
- `Objects/unicodeobject.c`: 4580, 7339, 9665
- `Objects/weakrefobject.c`: 1100
- `Python/_warnings.c`: 843, 892, 1407, 1809
- `Python/bltinmodule.c`: 69
- `Python/codecs.c`: 212, 366
- `Python/crossinterp.c`: 639, 708, 732
- `Python/errors.c`: 50, 1789
- `Python/gc.c`: 927
- `Python/import.c`: 3620
- `Python/intrinsics.c`: 37, 181
- `Python/pythonrun.c`: 1198

### `PyObject_Vectorcall` → `_PyValue_Vectorcall` (37 sites)

- `Modules/_asynciomodule.c`: 2813, 2853, 3477
- `Modules/_ctypes/callbacks.c`: 231
- `Modules/_ctypes/stgdict.c`: 313
- `Modules/_elementtree.c`: 1046, 3228, 3848, 4034
- `Modules/_operator.c`: 966
- `Modules/_sqlite/connection.c`: 202, 210, 2100, 2195
- `Modules/_sqlite/cursor.c`: 565, 1232
- `Modules/_sqlite/module.c`: 113
- `Modules/_sqlite/util.c`: 85
- `Modules/_sre/sre.c`: 1368, 1388
- `Objects/descrobject.c`: 2139
- `Objects/genericaliasobject.c`: 769
- `Objects/typeobject.c`: 13075, 14934
- `Objects/typevarobject.c`: 365
- `Objects/unionobject.c`: 492
- `Parser/pegen.c`: 666
- `Python/bltinmodule.c`: 643, 3055
- `Python/ceval.c`: 1043, 4421, 4530
- `Python/errors.c`: 1542
- `Python/gc.c`: 1314
- `Python/import.c`: 4574
- `Python/pythonrun.c`: 772
- `Python/sysmodule.c`: 673

### `PyObject_CallNoArgs` → `_PyValue_CallNoArgs` (29 sites)

- `Modules/_asynciomodule.c`: 345, 367, 568, 690, 759, 4567
- `Modules/_datetimemodule.c`: 4415
- `Modules/_elementtree.c`: 4392
- `Modules/_functoolsmodule.c`: 2155
- `Modules/_pickle.c`: 1472, 5274
- `Modules/_sqlite/connection.c`: 1056, 1556
- `Modules/_ssl.c`: 4566
- `Modules/_tkinter.c`: 3176
- `Modules/_uuidmodule.c`: 264
- `Modules/_zoneinfo.c`: 2735
- `Modules/faulthandler.c`: 1494
- `Modules/readline.c`: 1349
- `Objects/genobject.c`: 832
- `Objects/namespaceobject.c`: 269
- `Objects/typevarobject.c`: 584, 599, 614, 1341, 1879, 2041
- `Parser/tokenizer/reader.c`: 400
- `Python/pylifecycle.c`: 1379

### `PyMapping_GetOptionalItemString` → `_PyValue_MappingGetOptionalItemString` (3 sites)

- `Modules/_lzmamodule.c`: 277, 377
- `Modules/_sqlite/cursor.c`: 783

### `PyIter_NextItem` → `_PyValue_IterNext` (4 sites)

- `Objects/dictobject.c`: 3475, 3495, 3513
- `Objects/memoryobject.c`: 3164

### `_PyMapping_GetOptionalItem2` → `_PyValue_GetOptionalItem` (0 sites)

No callsites were found in this compiled Linux unit set.

### `_PyType_LookupRef` → `_PyType_LookupValue` (17 sites)

- `Modules/_collectionsmodule.c`: 2654, 2655, 2656, 2657
- `Modules/_lsprof.c`: 204
- `Objects/classobject.c`: 208, 460
- `Objects/moduleobject.c`: 1421
- `Objects/typeobject.c`: 4423, 7370, 13107, 13123, 13284
- `Python/ceval.c`: 2514, 5155
- `Python/pylifecycle.c`: 939, 946

### `_PySuper_Lookup` → `_PySuper_LookupValue` (0 sites)

No callsites were found in this compiled Linux unit set.

### `_PyObject_Call` → `_PyValue_Call` (4 sites)

- `Modules/signalmodule.c`: 1936
- `Objects/call.c`: 517, 535
- `Objects/typeobject.c`: 3434
