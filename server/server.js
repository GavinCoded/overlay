const net = require("net");

const PORT = parseInt(process.argv[2], 10) || 8080;
const clients = new Map();

function broadcast(sender, message, type = "MSG") {
  const line = `${type}|${sender}|${message}\n`;
  for (const [socket] of clients) {
    try {
      socket.write(line);
    } catch {
      clients.delete(socket);
    }
  }
}

function system_message(text) {
  const line = `SYS|${text}\n`;
  for (const [socket] of clients) {
    try {
      socket.write(line);
    } catch {
      clients.delete(socket);
    }
  }
}

const server = net.createServer((socket) => {
  let nickname = null;
  let buffer = "";

  socket.write("OK|Connected to chat server\n");

  socket.on("data", (chunk) => {
    buffer += chunk.toString();

    let idx;
    while ((idx = buffer.indexOf("\n")) !== -1) {
      const line = buffer.slice(0, idx);
      buffer = buffer.slice(idx + 1);
      if (!line) continue;

      const pipe = line.indexOf("|");
      const cmd = pipe === -1 ? line : line.slice(0, pipe);
      const arg = pipe === -1 ? "" : line.slice(pipe + 1);

      if (cmd === "NICK") {
        if (nickname) {
          system_message(`${nickname} is now known as ${arg}`);
        }
        nickname = arg;
        clients.set(socket, nickname);
        system_message(`${nickname} joined the server`);
        console.log(`${nickname} connected`);
      } else if (cmd === "MSG") {
        if (!nickname) {
          socket.write("SYS|Set a nickname first with NICK|name\n");
          return;
        }
        const msg = arg;
        broadcast(nickname, msg);
        console.log(`${nickname}: ${arg}`);
      }
    }
  });

  socket.on("close", () => {
    if (nickname) {
      clients.delete(socket);
      system_message(`${nickname} left the server`);
      console.log(`${nickname} disconnected`);
    }
  });

  socket.on("error", () => {
    if (nickname) {
      clients.delete(socket);
      system_message(`${nickname} left the server`);
    }
  });
});

server.listen(PORT, () => {
  console.log(`Chat server running on port ${PORT}`);
});
