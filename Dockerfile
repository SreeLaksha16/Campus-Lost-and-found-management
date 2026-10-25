
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends build-essential cmake libsqlite3-dev && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

WORKDIR /app/CampusFindAI
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j2

EXPOSE 10000

WORKDIR /app/CampusFindAI/backend
CMD ["sh", "-c", "../build/campusfind_server"]
