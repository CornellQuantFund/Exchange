# Stage 1: Build the application
FROM gcc:10 AS builder

# Install necessary packages
RUN apt-get update && apt-get install -y \
    cmake \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app

# Copy CMakeLists.txt and project files
COPY CMakeLists.txt main.cpp src/ include/ /app/

# Create build directory and compile the application
RUN mkdir build && cd build && \
    cmake .. && \
    make

# Stage 2: Create the final image
FROM gcc:10-slim

# Install runtime dependencies (Boost)
RUN apt-get update && apt-get install -y \
    libboost-system1.71.0 \
    libboost-thread1.71.0 \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app

# Copy the built executable from the builder stage
COPY --from=builder /app/build/main /app/main

# Copy necessary static and template files
COPY static/ /app/static/
COPY templates/ /app/templates/

# Expose the ports your application uses
EXPOSE 8080
EXPOSE 9090

# Define the entry point
CMD ["./main"]
