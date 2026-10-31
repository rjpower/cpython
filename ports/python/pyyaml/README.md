# PyYAML

Depends on `native/libyaml`, built as a static, position-independent library. The driver puts its include and lib directories on the compiler's search paths, so `yaml._yaml` (the Cython extension behind `yaml.CLoader` and `yaml.CSafeLoader`) builds and links libyaml into the extension. The smoke test requires the C loaders.
