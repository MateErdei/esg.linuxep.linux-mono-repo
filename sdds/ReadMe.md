Warehouse Generation
====================

This repo can build the following artifacts:

* For dev branches (`develop` or `feature/*`)
  * Standalone SDDS3 repo for automation or manual installation
  * LaunchDarkly flags
  * Mock SUS responses
* For release branches (`release/*` or `hotfix/*`)
  * Partial SDDS3 repo for SDDS3 publishing
  * LaunchDarkly flags for SDDS3 publishing


# Running tests locally
    ./tap run warehouse.test.ubuntu2004 --build-backend=local --test-backend=local

To run a specific suite, add to the environment e.g. `TAP_PARAMETER_ROBOT_TEST="Sul Downloader fails update if expected product missing from SUS"`

To run a specific test, add to the environment e.g. `TAP_PARAMETER_ROBOT_SUITE="InstallAndUpgrade"`

# Build Pipeline

The warehouse build pipeline looks like this:

    +--------------+         +------------+
    | winep_suites |-------> | prod-sdds3 |
    +--------------+ \       +------------+
                      \----> | dev-sdds3 |
                       \     +---------------+
                        `--> | dev-warehouse |
                             +---------------+

That is, all builds depend on `winep_suites`, but otherwise run in parallel.

## TAP Parameters

There are several TAP parameters you can use to choose which stage(s) to run:

    PROD_SDDS3
    [ ] Build production SDDS3 repo? (builds if checked OR on release branches)

    DEV
    [X] Build dev pipeline: SDDS2 warehouse and SDDS3 repo? (builds if checked AND NOT on release branches)	

    OMIT_SDDS2
    [ ] Omit dev SDDS2 warehouse? (only applicable for DEV pipeline)

To use these parameters on the commandline, use environment variables, like so:

    TAP_PARAMETER_DEV=true \
      tap fetch warehouse


## winep_suites

The `winep_suites` pipeline stage uses the `build/winep_suites.xml` package.

The `winep_suites/` folder in the repo contains the content of the suites themselves.

## prod-sdds3

The `prod-sdds3` pipeline stage uses the `build/prod.xml` package.

The repo is build from the following source files:

* `sdds3/...`
* `specs/...`

By default, this pipeline stage builds on `release/*` and `hotfix/*` branches.

## dev-sdds3

The `dev-sdds3` pipeline stage uses the `build/dev.xml` package with `mode="sdds3"`.

The repo is build from the following source files:

* `def/sdds3-suites-def.yaml`
* `def/sdds3-supplements-def.yaml`
* `def/common/components.yaml`
* `def/common/supplements.yaml`
* `sdds3/...`

This pipeline stage *never* builds on `release/*` or `hotfix/*` branches.

## dev-warehouse

The `dev-warehouse` pipeline stage uses the `build/dev.xml` package with `mode="sdds2"`.

The warehouse is build from the following source files:

* `def/cloud-warehouse-def.yaml`
* `def/opm-warehouse-def.yaml`
* `def/server-warehouse-def.yaml`
* `def/supplements-warehouse-def.yaml`
* `def/common/components.yaml`
* `def/common/supplements.yaml`
* `external/...`
* `src/...`

This pipeline stage *never* builds on `release/*` or `hotfix/*` branches.


# Local Build Instructions

**Prerequisite**: you've set up your build machine for Bazel and TAP. See [instructions here](https://stash.sophos.net/projects/EM/repos/esg/browse).

What do you want to build?

## Building a Production SDDS3 Repo Locally

The Production SDDS3 repo builds the software defined in the `specs/` folder:

    specs
    ├── winep
    │   ├── cepng
    │   │   ├── importspec.xml
    │   │   └── pubspec.xml
    │   ├── cix
    │   │   ├── importspec.xml
    │   │   └── pubspec.xml
    │   ├── encryption
    │   │   ├── importspec.xml
    │   │   └── pubspec.xml
    │   └── mtr
    │       ├── importspec.xml
    │       └── pubspec.xml
    └── winsrv
        ├── cepngsrv
        │   ├── importspec.xml
        │   └── pubspec.xml
        ├── cixsrv
        │   ├── importspec.xml
        │   └── pubspec.xml
        └── mtr
            └── link

**NOTE**: the `prep_prod.py` script downloads the software mentioned in the `importspec.xml` files needed
to build the repo. This requires lots of disk space: at least ~500MB.

Instructions:

1. Fetch inputs:

   ```bash
   $ TAP_PARAMETER_PROD_SDDS3=true \
       tap fetch warehouse
   ```

2. Generate the Bazel build rules:

   ```bash
   $ py -3 -u sdds3/tools/prep_prod.py
   ```

3. Build the repo:

   ```bash
   $ bazel build //sdds3/...
   ```

The following directories contain the output:

* `bazel-bin/sdds3/` contains the SDDS3 repo
* `output/launchdarkly/` contains a LaunchDarkly flag for each suite
* `output/specs/` contains Ballista-compatible specs based on the committed specs

## Building a Standalone Dev SDDS3 Repo Locally

The Standalone Dev SDDS3 repo contains the latest version of the Central Windows Agent, plus SDDS3 replicas
of the required data supplements.

**NOTE**: fetching build inputs downloads the latest build of all software for all platforms -- it takes ~500MB.

**NOTE**: downloading the data supplements adds another ~200MB.

Instructions:

1. Fetch inputs:

   ```bash
   $ TAP_PARAMETER_DEV=true \
       tap fetch warehouse
   ```

2. Generate the Bazel build rules:

   ```bash
   $ py -3 -u sdds3/tools/prep_dev.py
   ```

3. Build the repo:

   ```bash
   $ bazel build //sdds3/...
   ```

The following directories contain the output:

* `bazel-bin/sdds3/` contains the SDDS3 repo
* `output/launchdarkly/` contains a LaunchDarkly flag for each suite
* `output/sus/` contains a mock SUS response for each of the `winep` and `winsrv` products

## Building a Dev SDDS2 Warehouse Locally

This needs to be run on a machine that has the SDDS back-end installed. YMMV.

# META FORMAT

The YAML file `specs/meta.yaml` defines metadata about packages that is inconvenient to store
inside the pubspec or importspec files (because external tools may corrupt such extensions).

Example:

```yaml
%YAML 1.1
---
3799FB3E-808A-4F7D-AC6A-0C74F931C386:
    define-symbolic-paths: ['@flags']
D723F0FE-B74C-403D-8C17-A79DFD9D50E7#NG_REC:
    define-symbolic-paths: ['@flags']
```

The file is simply metadata organized by package.

Keys can either be:

* `line-id`

  This metadata will apply to all packages in that line-id, no matter their version.

* `line-id#importreference`

  Applies only to the specific instance of the package.

Values can be one of:

* `define-symbolic-paths` (array)

  This is a list of symbolic-paths. Each symbolic path must begin with `@`, and it
  expands to the package's decode path.

# SPECS FORMAT

The specs files in `specs/...` have some special extensions that allow them to generate
SDDS2 warehouses and SDDS3 repos.

## The `mode` attribute

The `pubspec.xml` supports a new `mode="sdds2|sdds3"` attribute on the `<component/>`
element.

Example:

```xml
<component line="3799FB3E-808A-4F7D-AC6A-0C74F931C386" importreference="4.13.16.0" mountpoint="mcsep"/>
<component line="D723F0FE-B74C-403D-8C17-A79DFD9D50E7" importreference="4.13.16.0" mountpoint="mcsarm64" mode="sdds3">
```

In the example above, `mcsarm64` will only be included in the SDDS3 repo; it will not be
included in the generated Ballista-compatible `output/specs/` for SDDS2 warehouses.

By default, every component applies to both `sdds2` and `sdds3`.

## The `flags` attribute on `<componentsuite>`

The `pubspec.xml` supports a new `flags=` attribute on `<componentsuite>` elements to
generate a flags supplement for either SDDS2 or SDDS3.

The supplement's various parameters are generated based on the parent folder name:
e.g. `cepng/pubspec.xml` --> `CEPNGFLAGS`.

The `warehousename=` attribute is generated based on the parent folder name:
e.g. `cix/pubspec.xml` --> `cix_flags`.

The `decodepath=` attribute is always `"."` for SDDS2, and `"@flags"` for SDDS3.
That's because the SDDS2 flags package decodes directly into `mcsep/flags/`; but
the SDDS3 flags package decodes in the symbolic path `@flags`, which is defined
by the MCS package.

Example:

```xml
<line id="WindowsCloudNextGen">
  <componentsuite importreference="NG_REC" flags="2020-6">
    <releasetag tag="RECOMMENDED" baseversion="11" />
  </componentsuite>
```

This is equivalent to writing the following (except that `<genericsupplement/>` does not support the `mode` attribute).

```xml
<line id="WindowsCloudNextGen">
  <componentsuite importreference="NG_REC">
    <releasetag tag="RECOMMENDED" baseversion="11" />
    <genericsupplement decodepath="." line="CEPNGFLAGS" tag="2020-6" warehousename="cepng_flags" mode="sdds2"/>
    <genericsupplement decodepath="@flags" line="CEPNGFLAGS" tag="2020-6" warehousename="cepng_flags" mode="sdds3"/>
  </componentsuite>
```

## The `relativeto` attribute on `<genericsupplement>`

The `pubspec.xml` supports adding a `relativeto` attribute on a `<genericsupplement>`
element to generate a supplement that applies to a subcomponent of the suite, rather
than to the suite itself.

In SDDS2, `relativeto` prefixes the decode path with the `relativeto`'s `mountpoint`,
and then attaches the supplement to the suite. In SDDS2, the same component version
may not target different supplements in different suites, so we attach the supplement
to the suite itself, which already has a different version for each suite anyway,
to accommodate having different marketing versions.

In SDDS3, `relativeto` causes the supplement to be attached to the `relativeto`
component. In SDDS3, a package's metadata is an attribute of the suite that contains
it, so each suite can have the same package with different supplements.

Example:

```xml
<line id="WindowsCloudHitmanProAlert">
  <componentsuite importreference="beta">
    <genericsupplement
        relativeto="12D32E6C-4CFF-435B-BC98-5E14CCB54950"
        decodepath="telemetry"
        line="995645D3-DA2A-407E-AC5D-7267B8B43975"
        tag="USER_TELEM"
        warehousename="ml_models" />
```

This is equivalent to writing the following in SDDS2:

```xml
<line id="WindowsCloudHitmanProAlert">
  <componentsuite importreference="beta">
    <genericsupplement
        decodepath="sme\telemetry"
        line="995645D3-DA2A-407E-AC5D-7267B8B43975"
        tag="USER_TELEM"
        warehousename="ml_models" />
```

... and this in SDDS3 (assuming that component `12D32E6C-4CFF-435B-BC98-5E14CCB54950`
version `1.7.0.19` belongs to suite `WindowsCloudHitmanProAlert` version `beta`).

```xml
<line id="12D32E6C-4CFF-435B-BC98-5E14CCB54950">
  <component importreference="1.7.0.19">
    <genericsupplement
        decodepath="telemetry"
        line="995645D3-DA2A-407E-AC5D-7267B8B43975"
        tag="USER_TELEM"
        warehousename="ml_models" />
```

## The `source=auto` importspec attribute

The `importspec.xml` supports defining an import using a `<componentsuite source="auto"/>`.

```xml
<imports>
  <line id="WindowsCloudAV">
    <componentsuite source="auto" id="AV_REC" version="11.6.514" longdic="10.8.10.2" shortdic="10.8.10.2" />
    <componentsuite source="auto" id="AV_BETA" version="11.6.513" longdic="10.8.10.2 BETA" shortdic="10.8.10.2 BETA" />
    <componentsuite source="auto" id="AV_BETA2" version="11.6.512" longdic="10.8.10.2 XDR BETA" shortdic="10.8.10.2 XDR BETA" />
```

When building SDDS3, this is just a shorthand for referencing `winep_suites` intermediate inputs:

```xml
<imports>
  <line id="WindowsCloudAV">
    <componentsuite id="AV_REC" version="11.6.514" longdic="10.8.10.2" shortdic="10.8.10.2"
                    fileset="./inputs/winep_suites/WindowsCloudAV/data" />
    <componentsuite id="AV_BETA" version="11.6.513" longdic="10.8.10.2 BETA" shortdic="10.8.10.2 BETA"
                    fileset="./inputs/winep_suites/WindowsCloudAV/data" />
    <componentsuite id="AV_BETA2" version="11.6.512" longdic="10.8.10.2 XDR BETA" shortdic="10.8.10.2 XDR BETA"
                    fileset="./inputs/winep_suites/WindowsCloudAV/data" />
```

When outputting SDDS2 Ballista-compatible specs, these are rewritten to refer to the build's `winep_suites` pipeline output:

```xml
<imports>
  <line id="WindowsCloudAV">
    <componentsuite id="AV_REC" version="11.6.514" longdic="10.8.10.2" shortdic="10.8.10.2"
                    artifact="esg-releasable-candidate/winep.warehouse/$branch/$buildid/winep_suites/SDDS-Ready-Packages/WindowsCloudAV/data" />
    <componentsuite id="AV_BETA" version="11.6.513" longdic="10.8.10.2 BETA" shortdic="10.8.10.2 BETA"
                    artifact="esg-releasable-candidate/winep.warehouse/$branch/$buildid/winep_suites/SDDS-Ready-Packages/WindowsCloudAV/data" />
    <componentsuite id="AV_BETA2" version="11.6.512" longdic="10.8.10.2 XDR BETA" shortdic="10.8.10.2 XDR BETA"
                    artifact="esg-releasable-candidate/winep.warehouse/$branch/$buildid/winep_suites/SDDS-Ready-Packages/WindowsCloudAV/data" />
```

# How To Install Your Warehouse

Depending on what you actually want, there are several possible answers.

For general instructions for installing a non-production warehouse, see [How to install a development warehouse](https://wiki.sophos.net/display/UG/How+to+install+a+development+warehouse). That information is useful anyway!

As a convenient way to serve your repo to virtual or physical machines with network access to your machine, try this.

```bash
$ py -3 -u ./sdds3/tools/server.py
Starting server in Mock SUS mode from C:\g\winep\warehouse\output\sus
Listening on 0.0.0.0 port 8080...
```

By default, `server.py` operates in "Mock SUS" mode: it parses AutoUpdate's or the CloudInstaller's SUS request,
extracts the `product`, and responds with the contents of the `./output/sus/mock_sus_response_$product.json` file.

You can override how the server's behaviour with extra options:

```bash
$ py -3 -u sdds3/tools/server.py --help
usage: server.py [-h] [--launchdarkly LAUNCHDARKLY] [--mock-sus MOCK_SUS]
                 [--sdds3 WWWROOT]

optional arguments:
  -h, --help            show this help message and exit
  --launchdarkly LAUNCHDARKLY
                        Path to launchdarkly flags (used to calculate suites
                        as per SUS)
  --mock-sus MOCK_SUS   Path to folder containing
                        "mock_sus_response_<product>.json" files
  --sdds3 WWWROOT       Path to (local) SDDS3 repository to serve
```


# FAQs / Gotchas

When changing a component's rigid-name aka ProductLineId:
* The ProductLineId must be unique to that platform (x86 / x64 / ARM64) of the component.
* The ProductLineCanonicalName must be unique to each ProductLineId - e.g. mcsep => mcs and shs => HEALTH, HEALTHARM64, etc.
* Self Repair Kit (repairkit.xml in SAU) must be taught to recognise the component by both its old and new ProductLineId's.
* ESH config (//winesh/configuration/config/Configuration.json) for the component's particular line (REC, BETA etc) in the warehouse must forget the component's old ProductLineId and learn the new one.

