#!/usr/bin/env python

## Generate a manifest.spec file for a given directory

import generateManifests

def main(argv):
    c = generateManifests.GenerateManifests()
    return c.generateManifestSpec(argv)

if __name__ == "__main__":
    import sys
    sys.exit(main(sys.argv))
  
