const express = require("express");
const http = require("http");
const socketIo = require("socket.io");
const cors = require("cors");

const app = express();
const server = http.createServer(app);
const io = socketIo(server);

app.use(cors());
app.use(express.json());

app.post("/audio", (req, res) => {
  const { device_id, status } = req.body;
  io.emit("update_table", { device_id, status });
  res.json({ status, device_id });
});

io.on("connection", (socket) => {
  console.log("New client connected");
  socket.on("disconnect", () => {
    console.log("Client disconnected");
  });
});

server.listen(4000, () => {
  console.log("Node.js server is running on port 4000");
});
