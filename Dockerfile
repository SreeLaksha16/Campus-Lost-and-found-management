
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y build-essential cmake libsqlite3-dev && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j2

EXPOSE 10000

WORKDIR /app/backend
CMD ["sh", "-c", "../build/campusfind_server"]