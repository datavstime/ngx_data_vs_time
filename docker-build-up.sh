#!/bin/bash

docker run --rm -it -p 8000:8000 \
  -v /git/datavstime/ngx_data_vs_time/:/repo \
  -v /git/datavstime/ngx_data_vs_time/:/var/www \
  mhowlett/nginx-build-base /bin/bash
