# PyYAML

The libyaml headers are not installed here, so the build skips the `yaml._yaml` extension and `yaml.CLoader`; the smoke test exercises the pure-Python loader and checks the C loader only when the extension was built.
