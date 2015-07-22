FROM debian:jessie

RUN apt-get -qq update \
    && apt-get -qqy install \
    autoconf \
    automake \
    build-essential \
    curl \
    g++ \
    git \
    libc6-dev-i386 \
    libcurl4-openssl-dev \
    libpcre3-dev \
    libtool \
    libunwind8 \
    make \
    uuid-dev \
    unzip \
    zlib1g-dev

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
  && curl -sSL http://nginx.org/download/nginx-${NGINX_VERSION}.tar.gz | tar zxfv - -C . \
  && cd /root/ \
  && ./script/compile \
  && ln -sf /root/nginx.conf /root/build/nginx/conf/nginx.conf

EXPOSE 80

CMD /root/build/nginx/sbin/nginx
