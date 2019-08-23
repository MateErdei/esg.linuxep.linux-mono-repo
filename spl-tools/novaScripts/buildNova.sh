#!/bin/bash
set -x
./gradlew cleanNova

az acr login -n SophosNovaHub --subscription  "CPG USA Devops"
./gradlew buildNova

