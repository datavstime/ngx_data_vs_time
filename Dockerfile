FROM mhowlett/nginx_build_base

RUN \
     mkdir /var/www \
  && mkdir /root/script

COPY predefined.json /var/www/
COPY version.json /var/www/

COPY config /root/
COPY script/compile /root/script/compile
COPY ngx_http_data_vs_time_module.c /root/
COPY nginx.docker.conf /root/nginx.conf

RUN \
     DIR=/root/ \
  && BUILDDIR=$DIR/build \
  && NGINX_DIR=nginx \
  && NGINX_VERSION=1.4.7 \
  \
  && mkdir $BUILDDIR \
  && mkdir $BUILDDIR/$NGINX_DIR \
  && mkdir $DIR/vendor \
  \
  && cd $DIR/vendor \
  && curl -sSL http://nginx.org/download/nginx-${NGINX_VERSION}.tar.gz | tar zxfv - -C . 
#  && cd nginx-${NGINX_VERSION} \
#  && ./configure \
#       --with-debug \
#       --prefix=/root/build/nginx \
#       --conf-path=conf/nginx.conf \
#       --error-log-path=logs/error.log \
#       --http-log-path=logs/access.log \
#  && make \
#  && make install \
#  && ln -sf /root/nginx.conf /root/build/nginx/conf/nginx.conf

RUN cd /root/ && ./script/compile && ln -sf /root/nginx.conf /root/build/nginx/conf/nginx.conf