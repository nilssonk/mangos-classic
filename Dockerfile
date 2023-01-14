FROM debian:sid-slim

ARG CLANG_VERSION=14
ARG GCC_VERSION=12

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get upgrade -y

# development environment
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    dpkg-dev wget curl \
    bash \
    gcc-$GCC_VERSION \
    g++-$GCC_VERSION \
    libc-dev \
    cmake ninja-build make autoconf automake autotools-dev libtool gettext \
    git subversion mercurial \
    python3 python3-venv python3-dev python3-pip \
    clang-$CLANG_VERSION \
    clang-tidy-$CLANG_VERSION \
    clang-format-$CLANG_VERSION \
    llvm \
    ccache valgrind

# GCC version defaults
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-$GCC_VERSION 99 \
    --slave   /usr/bin/cc cc /usr/bin/gcc-$GCC_VERSION \
    --slave   /usr/bin/c++ c++ /usr/bin/g++-$GCC_VERSION \
    --slave   /usr/bin/g++ g++ /usr/bin/g++-$GCC_VERSION \
    --slave   /usr/bin/gcov gcov /usr/bin/gcov-$GCC_VERSION \
    --slave   /usr/bin/gcov-dump gcov-dump /usr/bin/gcov-dump-$GCC_VERSION \
    --slave   /usr/bin/gcov-tool gcov-tool /usr/bin/gcov-tool-$GCC_VERSION \
    --slave   /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-$GCC_VERSION \
    --slave   /usr/bin/gcc-nm gcc-nm /usr/bin/gcc-nm-$GCC_VERSION \
    --slave   /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-$GCC_VERSION

# Clang version defaults
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-$CLANG_VERSION 99 \
    --slave /usr/bin/clang++ clang++ /usr/bin/clang++-$CLANG_VERSION \
    --slave /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-$CLANG_VERSION \
    --slave /usr/bin/clang-format clang-format /usr/bin/clang-format-$CLANG_VERSION

# dev tools
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    less \
    gdb ltrace strace linux-perf \
    openssh-client gpg neovim

# dev libs
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        libboost-dev \
        libboost-thread-dev \
        libboost-filesystem-dev \
        libboost-program-options-dev \
        libboost-regex-dev \
        default-libmysqlclient-dev

ARG USER=dev
ARG USER_UID=1000
ARG USER_GID=1000

RUN groupadd --gid $USER_GID $USER && useradd -m --gid $USER_GID --uid $USER_UID $USER

USER $USER

# bash autocompletion for git
COPY --chown=$USER:$USER profile /home/$USER/.profile
RUN curl https://raw.githubusercontent.com/git/git/master/contrib/completion/git-completion.bash \
    -o /home/$USER/.git-completion.bash

# https://code.visualstudio.com/remote/advancedcontainers/avoid-extension-reinstalls
RUN mkdir -p \
    /home/$USER/.vscode-server/extensions \
    /home/$USER/.vscode-server-insiders/extensions \
    && chown -R $USER \
    /home/$USER/.vscode-server \
    /home/$USER/.vscode-server-insiders

ENV SHELL /bin/bash
