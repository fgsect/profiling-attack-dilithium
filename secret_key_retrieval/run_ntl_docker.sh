#!/bin/bash
docker build . -t ntl_docker
docker run --privileged -v $(realpath ../):/current_dir -ti --rm ntl_docker /bin/bash
