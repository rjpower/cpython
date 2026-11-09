# Complete closing benchmark tables

Frozen candidate `4d666e1b482`, GCC 15.2, JIT off, CPU 15/hash0. Lower ratios use less elapsed time or fewer instructions. See [report](closing-report.md) and [independent raw audit](final-performance-audit.md).

## Application kernels

Two balanced pairs per control, seven samples per process. Geometric means weight each kernel once. These are workload results, not component attribution.

| Kernel | Time / golden | Time / upstream |
| --- | ---: | ---: |
| richards | 0.916751 | 1.240218 |
| go | 0.928377 | 1.227241 |
| nbody | 0.971235 | 0.841104 |
| float | 0.991747 | 0.807037 |
| spectral_norm | 0.763540 | 0.663817 |
| raytrace | 0.908463 | 0.962438 |
| deltablue | 0.886394 | 1.619651 |
| chaos | 0.974863 | 0.974569 |
| nqueens | 0.969904 | 1.209259 |
| fannkuch | 1.000587 | 1.283878 |
| generators | 0.985246 | 1.415179 |
| comprehensions | 0.935004 | 1.330819 |
| coroutines | 0.876219 | 1.249122 |
| unpack_sequence | 0.818137 | 1.151186 |
| deepcopy | 0.963418 | 1.356861 |
| deepcopy_reduce | 0.959787 | 1.361174 |
| deepcopy_memo | 1.001685 | 1.340406 |
| json_dumps | 0.961494 | 1.315698 |
| json_loads | 1.008427 | 1.199214 |
| pickle | 0.997085 | 1.090692 |
| unpickle | 0.982192 | 1.193002 |
| regex_compile | 0.966375 | 1.187182 |
| logging_format | 0.984156 | 1.293271 |
| logging_silent | 0.586521 | 0.885649 |
| logging_simple | 0.968555 | 1.330307 |
| xml_etree_generate | 0.991782 | 1.274024 |
| xml_etree_iterparse | 0.970813 | 1.274189 |
| xml_etree_parse | 0.980863 | 1.157075 |
| xml_etree_process | 0.982730 | 1.419427 |
| **Geometric mean** | **0.934192** | **1.174985** |

## Logging confirmation

Six balanced pairs in each independent campaign. The gate is mean at most 1.0, every pair at most 1.02.

| Campaign | Mean / upstream | Pair range | Pass |
| --- | ---: | --- | --- |
| 1 | 0.906735 | 0.895782–0.923285 | yes |
| 2 | 0.897500 | 0.866234–0.916993 | yes |

## Micro instructions

All 65 registered rows, four low/high pairs each (200/600 outer loops, 1,000 inner iterations). Net values subtract the registered skeleton; raw values keep it. Ratios of nonpositive net values are omitted. These ratios are instruction diagnostics, not elapsed speedups.

| Workload | Candidate net instructions/iteration | Raw / upstream | Net / upstream | Net / golden |
| --- | ---: | ---: | ---: | ---: |
| empty_loop | 91.999 | 1.139103 | 1.139103 | 1.023060 |
| empty_loop_large_i | 92.094 | 0.635408 | 0.635408 | 1.023059 |
| empty_list_loop | 118.313 | 1.735865 | 1.735865 | 0.922505 |
| int_add | 76.001 | 0.686089 | 0.463134 | 1.027049 |
| int_add_small | 75.998 | 0.929115 | 0.759602 | 1.027051 |
| float_add | 76.999 | 0.801480 | 0.591876 | 0.974602 |
| int_compare | 92.998 | 1.064315 | 0.999404 | 1.021917 |
| float_compare | 92.994 | 1.157572 | 1.176442 | 1.010758 |
| int_chain | 370.009 | 0.541768 | 0.479277 | 1.060175 |
| float_chain | 379.006 | 0.693866 | 0.633738 | 0.984427 |
| compare_branch | 121.504 | 0.936922 | 0.825925 | 1.016768 |
| store_fast_two | 91.999 | 1.335382 | 1.613384 | 0.938747 |
| attr_load | 84.998 | 1.165744 | 1.196020 | 0.999925 |
| attr_load_poly | 363.697 | 1.745095 | 1.748118 | 1.013053 |
| attr_load_poly5 | 527.991 | 1.815711 | 1.834621 | 1.013221 |
| attr_store | 73.999 | 1.115831 | 1.088190 | 0.999968 |
| attr_store_boxed | 74.002 | 1.115698 | 1.087908 | 0.999958 |
| attr_store_poly | 429.946 | 1.697509 | 1.687250 | 1.002534 |
| attr_load_after_dict | 82.997 | 0.428018 | 0.252972 | 0.999921 |
| attr_store_after_dict | 112.004 | 0.417342 | 0.274485 | 0.999999 |
| attr_load_dataclass | 85.001 | 1.165278 | 1.194998 | 0.999907 |
| attr_store_dataclass | 727.164 | 1.655401 | 1.756103 | 1.000000 |
| slots_load | 79.998 | 1.155415 | 1.174760 | 0.999925 |
| property_load | 356.006 | 1.208117 | 1.227333 | 0.894467 |
| class_attr_load | 565.189 | 4.539366 | 8.829597 | 1.012527 |
| method_call | 449.003 | 1.387843 | 1.452846 | 0.963507 |
| new_instance | 1949.268 | 1.302405 | 1.311278 | 0.989344 |
| new_instance_empty | 1428.226 | 1.280960 | 1.291319 | 0.974749 |
| isinstance | 363.007 | 1.614527 | 1.805506 | 0.945303 |
| global_load | 82.001 | 1.253465 | 1.412576 | 0.999950 |
| builtin_load | 96.999 | 1.278791 | 1.447102 | 0.999975 |
| call_len | 237.005 | 1.413155 | 1.558723 | 0.936781 |
| call_abs | 353.001 | 1.573431 | 1.747037 | 0.986031 |
| call_math_sin | 515.001 | 1.292088 | 1.323850 | 1.001932 |
| call_py_1 | 399.009 | 1.198113 | 1.212597 | 0.947758 |
| call_py_3 | 493.002 | 1.289024 | 1.321479 | 0.953571 |
| call_py_kw | 1337.002 | 1.985451 | 2.092427 | 0.997764 |
| call_closure | 586.007 | 1.294301 | 1.322591 | 0.959082 |
| generator_resume | 464.680 | 2.256941 | 2.801176 | 0.949404 |
| list_subscript | 92.002 | 1.102805 | 1.068749 | 0.919981 |
| tuple_subscript | 85.999 | 1.080330 | 1.023819 | 0.966275 |
| dict_subscript | 500.004 | 2.042866 | 2.392067 | 1.004010 |
| float_list_subscript | 92.002 | 1.090197 | 1.045319 | 0.919999 |
| str_join | 446.615 | 1.791363 | 2.030916 | 1.008922 |
| str_concat | 544.838 | 1.119145 | 1.115844 | 1.007647 |
| str_concat_loop | 543.424 | 1.096492 | 1.089591 | 1.007419 |
| try_noraise | 46.000 | 1.123284 | 1.092929 | 0.821449 |
| try_raise | 1939.708 | 1.354625 | 1.366891 | 1.019980 |
| getattr_property | 1377.003 | 1.355378 | 1.372792 | 0.973146 |
| getattr_property_float | 1376.997 | 1.355409 | 1.372826 | 0.973140 |
| getattribute_hook | 1621.999 | 1.669233 | 1.714490 | 0.993262 |
| getattr_hook | 1942.980 | 1.703494 | 1.744419 | 1.004114 |
| setattr_hook | 1211.105 | 1.425893 | 1.453695 | 0.992617 |
| descr_get_py | 1500.189 | 1.423063 | 1.445155 | 1.004687 |
| descr_set_py | 1430.161 | 1.425528 | 1.448966 | 0.993747 |
| property_store | 1265.164 | 1.372592 | 1.393360 | 0.993715 |
| property_store_float | 1265.161 | 1.362810 | 1.382554 | 0.993714 |
| call_instance | 1603.998 | 1.359141 | 1.374368 | 0.990123 |
| namedtuple_field | 531.195 | 1.703375 | 1.863228 | 1.013356 |
| member_double | 581.194 | 1.420671 | 1.478522 | 1.012177 |
| object_setattr | 1487.003 | 1.212061 | 1.216883 | 0.986728 |
| class_attr_store | 1280.119 | 1.719869 | 1.785285 | 0.999218 |
| method_lazy_dict | 449.003 | 1.388021 | 1.453082 | 0.963521 |
| getset_number | 686.207 | 1.638768 | 1.741164 | 1.010301 |
| cell_contents | 727.197 | 2.028186 | 2.250400 | 1.009728 |

## Allocation and collector time

All six timed rows; one warm-up plus five timed calls. One serial sweep per build, so ratios are unpaired diagnostics. Collector units are seconds per fixed nominal allocated GB, not measured allocation bytes. Startup/import RSS-only rows are excluded.

| Workload | Unit | Upstream | Golden | Candidate | / upstream | / golden |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| tuples_10m | s/loop | 1.069035 | 1.069539 | 0.963854 | 0.901611 | 0.901186 |
| instances_1m | s/loop | 0.428416 | 0.418654 | 0.415512 | 0.969879 | 0.992495 |
| dicts_200k | s/loop | 0.080904 | 0.068988 | 0.068711 | 0.849294 | 0.995987 |
| collector_live16 | s/fixed allocated GB | 0.568879 | 0.690244 | 0.721062 | 1.267513 | 1.044647 |
| collector_live128 | s/fixed allocated GB | 0.544709 | 0.698371 | 0.728061 | 1.336604 | 1.042513 |
| collector_live512 | s/fixed allocated GB | 0.546502 | 0.691598 | 0.718710 | 1.315108 | 1.039201 |

The collector rows are about 4% slower than golden in this unpaired sweep. Repeat in balanced order before calling that a new regression; this round makes no collector-redesign claim.

Raw evidence: E11/closing39-kernels-{golden,upstream}/, closing39-logging-{1,2}/ and closing39-supplemental-v2/. All final campaign/sweep identities and input hashes are stable.
