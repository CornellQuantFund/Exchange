FROM gcc:11.2.0

RUN apt-get update && apt-get install -y \
    cmake \
    make \
    nlohmann-json3-dev \
    libboost-system-dev \
    libboost-thread-dev \
    libboost-filesystem-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /usr/src/app

# Copy all files into the container
COPY . .

# Compile the code
RUN g++ -std=c++17 -o main main.cpp src/*.cpp -lboost_thread -lboost_system -lpthread
# Expose the ports
EXPOSE 8080
EXPOSE 9090

# Specify the default command to run the application
CMD ["./main"]


