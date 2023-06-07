FROM gcr.io/oss-fuzz-base/base-builder:v1
RUN apt-get update && apt-get install -y make libssl-dev libbsd-dev
COPY . $SRC/app-stellar
WORKDIR $SRC/app-stellar
COPY .clusterfuzzlite/build.sh $SRC/
