# Multi-stage production build for secure_telemetry_gateway
FROM ros:jazzy-ros-base-noble AS builder

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /ros2_ws

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libsqlite3-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . src/secure_telemetry_gateway/

RUN . /opt/ros/jazzy/setup.sh && \
    colcon build --packages-select secure_telemetry_gateway --cmake-args -DCMAKE_BUILD_TYPE=Release

FROM ros:jazzy-ros-core-noble AS runtime

WORKDIR /ros2_ws

RUN apt-get update && apt-get install -y \
    libsqlite3-0 \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /ros2_ws/install /ros2_ws/install

ENTRYPOINT ["/bin/bash", "-c", "source /opt/ros/jazzy/setup.bash && source /ros2_ws/install/setup.bash && ros2 launch secure_telemetry_gateway gateway.launch.py"]