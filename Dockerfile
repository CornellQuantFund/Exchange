########################################################################################################################
# Exchange Build Stage
########################################################################################################################

FROM alpine:3.17.0 AS build

# Install necessary build packages
RUN apk update && \
    apk add --no-cache \
    build-base \
    cmake \
    boost1.80-dev=1.80.0-r3

# Set the working directory for building
WORKDIR /Exchange

# Copy source code and CMake configuration
COPY src/ ./src/
COPY include/ ./include/
COPY CMakeLists.txt .

# Create build directory and compile the application
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make

########################################################################################################################
# Exchange Runtime Image
########################################################################################################################

FROM alpine:3.17.0

# Install runtime dependencies (Boost libraries)
RUN apk update && \
    apk add --no-cache \
    libstdc++ \
    boost1.80-program_options=1.80.0-r3

# Create a non-root user and group for running the application
RUN addgroup -S Exchange && adduser -S Exchange -G Exchange

# Set the working directory inside the container
WORKDIR /Exchange

# Copy the compiled executable from the build stage
COPY --chown=Exchange:Exchange --from=build \
    /Exchange/build/src/Exchange \
    ./Exchange/

# Copy necessary static and template files
COPY --chown=Exchange:Exchange static/ ./static/
COPY --chown=Exchange:Exchange templates/ ./templates/

# Switch to the non-root user
USER Exchange

# Expose the ports your application uses
EXPOSE 8080
EXPOSE 9090

# Define the entry point to run your application
ENTRYPOINT [ "./Exchange/main" ]
