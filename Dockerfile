FROM debian:bookworm

RUN apt-get update && apt-get install -y \
    build-essential g++ gcc libc6-dev make cmake \
    libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev libxext-dev \
    libgl1-mesa-dev libglu1-mesa-dev libasound2-dev \
    pkg-config sqlite3 libsqlite3-dev wget unzip git \
    nano \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/raysan5/raylib.git /raylib \
    && cd /raylib/src \
    && make PLATFORM=PLATFORM_DESKTOP -s RAYLIB_RELEASE=1 \
    && make install

WORKDIR /app

# RUN g++ main.cpp -o todo_app -lraylib -lsqlite3 -std=c++17
CMD ["bash"]