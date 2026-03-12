FROM fedora:43 AS builder

RUN dnf install -y \
    cmake \
    gcc \
    gcc-c++ \
    make \
    gtk4-devel \
    libadwaita-devel \
    webkitgtk6.0-devel \
    && dnf clean all

WORKDIR /src
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel "$(nproc)"

# The built binary lives at /src/build/mdpeek
# Copy it out via a multi-stage build or volume mount.
FROM fedora:43

RUN dnf install -y gtk4 libadwaita webkitgtk6.0 && dnf clean all

COPY --from=builder /src/build/mdpeek /usr/local/bin/mdpeek

ENTRYPOINT ["mdpeek"]
