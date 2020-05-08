BUILD tools:
- g++
- make

BUILD game_term:
- libgpm-dev

RUN game_term:
- gpm

BUILD asciiid:
- xorg-dev
- libgl1-mesa-dev

RUN asciiid:
- libgl1-mesa-glx

BUILD game:
- libgpm-dev
- xorg-dev
- libgl1-mesa-dev

RUN game:
- gpm
- libgl1-mesa-glx
