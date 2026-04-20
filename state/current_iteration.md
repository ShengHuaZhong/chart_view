# Current Iteration

- Task: `113-e400-official-static-asset-wave-1`
- Status: `112-e400-compiler-loader-integration` completed. The local official
  `PresLib_e4.0.0.dai` file now feeds the runtime-owned preferred
  compiler/loader path, `S52LookupModel` recognizes the official table names
  used by the e4.0.0 catalog, and the pinned OpenCPN path remains available as
  an explicit fallback.
- Blocker: none
- Previous task: `112-e400-compiler-loader-integration` added the official
  DAI-based preferred catalog path alongside the OpenCPN fallback path and
  proved that the official compiler/lookup path is usable without host changes
  or runtime public-ABI expansion.
