package main

import (
    "net/http"
    "io"
    "runtime"
)

func HelloServer(w http.ResponseWriter, req *http.Request) {
    w.Header().Set("Content-Type", "text/plain")
    w.Header().Set("Connection", "keep-alive")
    w.Header().Set("Content-Length", "14")
    io.WriteString(w, "hello, world!\n")
}

func main() {
    runtime.GOMAXPROCS(4)
    http.HandleFunc("/", HelloServer)
    http.ListenAndServe(":8081", nil)
}

