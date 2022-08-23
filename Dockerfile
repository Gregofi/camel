FROM debian:bullseye

RUN apt-get update -y && \
    apt-get install -y gcc \
                       clang \
                       cmake \
                       python \
                       valgrind \
                       lldb \
                       git \
                       curl \
                       clangd

# rustup
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | bash -s -- -y

RUN echo 'source ${HOME}/.cargo/env' >> ${HOME}/.bashrc
