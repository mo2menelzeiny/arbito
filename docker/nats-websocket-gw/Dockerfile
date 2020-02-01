FROM golang:latest

RUN go get github.com/spf13/cobra

RUN go get github.com/spf13/viper

RUN go get github.com/orus-io/nats-websocket-gw

RUN go install github.com/orus-io/nats-websocket-gw/cmd/nats-websocket-gw

ENTRYPOINT ["bin/nats-websocket-gw"]

CMD ["--no-origin-check", "--host=0.0.0.0", "--nats=nats:4222"]