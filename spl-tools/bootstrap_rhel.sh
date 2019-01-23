#!/bin/bash 

echo "Bootstrapping RedHat"
sudo subscription-manager attach
sudo subscription-manager repos --enable "rhel-*-optional-rpms" --enable "rhel-*-extras-rpms"



