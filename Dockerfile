FROM gcc:latest

WORKDIR /app

RUN apt-get update && apt-get install -y libasio-dev curl

COPY . .

RUN g++ -o server main.cpp -lpthread

EXPOSE 18080

CMD ["./server"]
