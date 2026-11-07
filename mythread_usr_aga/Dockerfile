FROM debian:bookworm-slim
RUN apt-get update \
    && apt-get install -y --no-install-recommends gcc libc6-dev cmake make \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY CMakeLists.txt main.c uthread.c uthread.h test_uthread.c ./
RUN cmake -S . -B build -DCMAKE_C_FLAGS="-Wall -Wextra -Werror" \
    && cmake --build build \
    && ctest --test-dir build --output-on-failure
CMD ["./build/1_7_umythread"]
