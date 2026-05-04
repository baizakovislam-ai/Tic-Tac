FROM debian:bookworm-slim AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY . .

RUN cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build backend/build --config Release -j"$(nproc)"

FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=build /src/backend/build/tic_tac_server /app/tic_tac_server
COPY --from=build /src/index.html /app/index.html
COPY --from=build /src/assets /app/assets

ENV PORT=10000
EXPOSE 10000

CMD ["/app/tic_tac_server"]
