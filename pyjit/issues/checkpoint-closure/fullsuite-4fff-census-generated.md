## Boxes by class

| site | suite |
|---|---|
| unattributed | 655995 |
| number | 1832890 |
| compare | 72399 |
| repr | 280390 |
| sequence | 3600707 |
| iteration | 10985845 |
| attribute | 6439256 |
| call | 2695281 |
| list | 6956698 |
| tuple | 5364662 |
| dict | 981718 |
| set | 61106 |
| cell/structseq | 187 |
| call adapter | 3571324 |
| parser | 7222817 |
| interpreter | 0 |
| field | 771472 |
| frame | 13475 |
| compiler | 48545 |
| test | 619 |

## Boxes by kind

| kind | suite |
|---|---|
| core | 23917323 |
| thunk | 17282245 |
| slot adapter | 6915747 |
| call adapter | 3440071 |

## Owner split

| kind | core | core-pending | test | compat-test | external | foreign | unresolved |
|---|---|---|---|---|---|---|---|
| core | 23915214 | 0 | 654 | 0 | 0 | 0 | 1455 |
| thunk | 16212584 | 191096 | 226548 | 651312 | 0 | 0 | 705 |
| slot adapter | 6615584 | 93013 | 200000 | 7145 | 0 | 0 | 5 |
| call adapter | 3422833 | 8729 | 884 | 4978 | 0 | 0 | 2647 |

## Debt total

50463865

## Top debt

| function | unit | class | kind | count |
|---|---|---|---|---|
| _PyBytes_FromIterator | Objects/bytesobject.c | core | thunk | 6389976 |
| Py_DECREF_MORTAL | Modules/itertoolsmodule.c | core | core | 6010360 |
| PyValue_AsKnownObjectSteal | Modules/_collectionsmodule.c | core | core | 3786971 |
| property_descr_set | Objects/descrobject.c | core | slot adapter | 2459596 |
| _ctypes_Simple_value_set | Modules/_ctypes/_ctypes.c | core | slot adapter | 2367946 |
| PyValue_AsObject | Modules/_struct.c | core | core | 1812714 |
| math_fsum_impl | Modules/mathmodule.c | core | thunk | 1665186 |
| PyValue_AsObject | Python/bltinmodule.c | core | core | 1568522 |
| internal_bisect_right | Modules/_bisectmodule.c | core | thunk | 1276610 |
| Py_DECREF | Modules/itertoolsmodule.c | core | thunk | 1247626 |
| _PyObject_VectorcallTstate | Objects/call.c | core | thunk | 1144244 |
| _PyObject_VectorcallTstate | Objects/call.c | core | thunk | 1095202 |
| _PyDict_Subscript | Objects/dictobject.c | core | call adapter | 1061103 |
| PyValue_AsObject | Objects/longobject.c | core | core | 957533 |
| partial_call | Modules/_functoolsmodule.c | core | core | 693815 |
| PyValue_AsObject | Python/bltinmodule.c | core | core | 674709 |
| PyValue_AsObject | Objects/bytesobject.c | core | core | 673556 |
| tp_new_wrapper | Objects/typeobject.c | core | call adapter | 672774 |
| Py_INCREF | Modules/_operator.c | core | core | 628402 |
| frozendict_vectorcall | Objects/dictobject.c | core | call adapter | 618833 |

## Accounting

| item | value |
|---|---|
| dumps folded | 6326 |
| leftover .run | 598 |
| uncounted launches by tag | : 40, test.test_asyncio.test_base_events: 4, test.test_asyncio.test_eager_task_factory: 2, test.test_asyncio.test_tasks: 6, test.test_concurrent_futures.test_interpreter_pool: 2, test.test_concurrent_futures.test_shutdown: 2, test.test_future_stmt.test_future: 2, test.test_inspect.test_inspect: 2, test.test_io.test_fileio: 2, test.test_io.test_general: 2, test.test_io.test_textio: 2, test.test_multiprocessing_fork.test_misc: 6, test.test_multiprocessing_fork.test_processes: 4, test.test_multiprocessing_forkserver.test_misc: 6, test.test_multiprocessing_forkserver.test_processes: 4, test.test_multiprocessing_spawn.test_misc: 6, test.test_multiprocessing_spawn.test_processes: 4, test.test_os.test_posix: 22, test.test_pydoc.test_pydoc: 2, test___all__: 2, test__colorize: 2, test_annotationlib: 2, test_argparse: 2, test_array: 2, test_ast: 2, test_atexit: 2, test_base64: 2, test_bdb: 2, test_builtin: 2, test_bytes: 2, test_c_locale_coercion: 2, test_calendar: 2, test_capi: 2, test_cmd: 2, test_cmd_line: 22, test_cmd_line_script: 4, test_code: 2, test_code_module: 2, test_compile: 2, test_compileall: 2, test_coroutines: 2, test_csv: 2, test_ctypes: 2, test_dataclasses: 2, test_datetime: 2, test_descr: 2, test_dict: 2, test_difflib: 2, test_doctest: 2, test_email: 2, test_embed: 72, test_enum: 2, test_eof: 2, test_exceptions: 2, test_faulthandler: 4, test_frame: 2, test_fstring: 2, test_functools: 2, test_gc: 2, test_gc_verify: 2, test_gettext: 2, test_graphlib: 2, test_gzip: 2, test_hash: 2, test_httpservers: 2, test_idle: 2, test_import: 2, test_importlib: 2, test_interpreters: 2, test_ir_contract: 4, test_itertools: 2, test_json: 2, test_lazy_import: 2, test_linecache: 2, test_list: 2, test_locale: 2, test_logging: 2, test_marshal: 2, test_mimetypes: 2, test_module: 2, test_monitoring: 2, test_multiprocessing_main_handling: 2, test_ntpath: 2, test_optparse: 2, test_pathlib: 2, test_pdb: 2, test_perf_profiler: 2, test_pickle: 2, test_posixpath: 2, test_pprint: 2, test_profile: 2, test_profiling: 2, test_pstats: 2, test_py_compile: 2, test_pystats: 2, test_pyvalue_api: 2, test_pyvalue_release: 2, test_pyvalue_sysflags: 2, test_readline: 2, test_regrtest: 174, test_repl: 2, test_runpy: 8, test_sax: 2, test_script_helper: 2, test_select: 2, test_shlex: 2, test_shutil: 2, test_signal: 2, test_site: 12, test_socket: 2, test_source_encoding: 2, test_sqlite3: 2, test_str: 2, test_string: 2, test_struct: 2, test_structseq: 2, test_subprocess: 12, test_support: 10, test_sys: 4, test_tabnanny: 2, test_tagged_values: 2, test_tarfile: 2, test_tempfile: 2, test_threading: 2, test_tokenize: 2, test_tomllib: 2, test_tools: 2, test_trace: 2, test_traceback: 2, test_tracemalloc: 4, test_tstring: 2, test_turtle: 2, test_types: 2, test_typing: 2, test_unicodedata: 2, test_unittest: 9, test_utf8_mode: 6, test_uuid: 2, test_warnings: 2, test_weakref: 2, test_zipfile: 2, test_zipimport_support: 2, test_zoneinfo: 2 |
| resets | 2 |
| drops | 0 |
| box_cached | 34026890 (66.0% of boxes) |
