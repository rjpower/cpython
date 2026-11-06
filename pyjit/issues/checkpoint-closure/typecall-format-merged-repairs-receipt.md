# Constructor and formatting merged-repairs receipt

The merged repair cohort passed three builds, with 117 modules checked in each, and lint. The affected release and debug test runs each passed 410 tests. These checks precede application of the advanced integer formatting patch.

Fresh statistics for that cohort are in `build/pyjit-link-wave1/checkpoint-closure/typecall-format-reviewed-repairs-stats`, with its matching executable and source identity. All 12 constructor and float-format probes recorded zero startup boxes, zero hot-loop boxes, zero drops, and complete identity accounting. This is new evidence for the repaired cohort; it does not alter the earlier [attribution diagnosis](typecall-format-boxes-diagnosis.md), which describes a different frozen executable.

The integer formatting implementation and its tests have since been applied to the integration candidate. They have not yet been built or tested in that candidate. The two integer format/field probes, each covering 100 calls, remain pending the writer build and fresh statistics; no integer formatting boxing result is claimed here.
