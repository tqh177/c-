gcc  -g log.c main.c config.c header.c until/object.c until/json.c until/stream.c sock/server.c -o debug/main.exe -lws2_32 -D _DEBUG