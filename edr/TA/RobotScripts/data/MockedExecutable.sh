#!/bin/bash
# echos the executable called + arguments
echo $(basename $0) $@ >> /tmp/mockedExecutable
echo $(basename $0) $@