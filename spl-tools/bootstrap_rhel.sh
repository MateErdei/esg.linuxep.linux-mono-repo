#!/bin/bash 

echo "Bootstrapping RedHat"
subscription-manager attach
subscription-manager repos --enable "rhel-*-optional-rpms" --enable "rhel-*-extras-rpms"

