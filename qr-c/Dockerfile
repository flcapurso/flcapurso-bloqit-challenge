# ---- Build stage ----
FROM alpine:3.19 AS builder

RUN apk add --no-cache \
    build-base \
    cmake \
    linux-headers

WORKDIR /app

# Copy CMakeLists and source files
COPY CMakeLists.txt .
COPY src/ src/

# Build
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel

# ---- Runtime stage ----
FROM alpine:3.19

RUN apk add --no-cache libstdc++

WORKDIR /app

# Copy only the compiled binary from the build stage
COPY --from=builder /app/build/qr-c .

# Serial port device path, can be overridden in docker-compose
ENV SERIAL_PORT=/tmp/ttyS1
ENV BUFFER_SIZE=1024
ENV SOCKET_PATH=/tmp/qr_socket.sock

CMD ["./qr-c"]